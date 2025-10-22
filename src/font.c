//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2024, SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 3
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "font.h"
#include "driver.h"
#include "util.h"
#include "video.h"
#include <stdlib.h>
#define ReLU(A)            ((A) > 0 ? (A) : 0)
#define unicode_lower_top  0xD800
#define unicode_upper_base 0xF900
#define unicode_upper_top  0xFFFE

// Font size constants
#define FONT_SIZE_SMALL    16U // 16x16 pixels
#define FONT_SIZE_LARGE    32U // 32x32 pixels
#define FONT_WIDTH_SMALL   8U  // 8 pixels width for small font
#define FONT_WIDTH_LARGE   16U // 16 pixels width for large font
#define FONT_DATA_SIZE     32U // Size of font data buffer
#define BITS_PER_BYTE      8U  // Number of bits per byte for bitmap

#define ESP32_PLATFORM     1
#define FONT_CACHE_ENABLED 1

/*
 * Font Size Management Design:
 * 
 * The font system supports two font sizes:
 * - Small font: 16x16 pixels (width: 8 pixels)
 * - Large font: 32x32 pixels (width: 16 pixels)
 * 
 * Font size information is stored in a bitmap format in 'unicode_font_size.bin':
 * - Each byte contains size information for 8 consecutive characters
 * - Bit 0 (LSB) corresponds to character at position (byte_index * 8 + 0)
 * - Bit 7 (MSB) corresponds to character at position (byte_index * 8 + 7)
 * - Bit value: 0 = small font (16x16), 1 = large font (32x32)
 * 
 * Font data is stored in 'unicode_font.bin':
 * - Each character occupies 32 bytes regardless of actual size
 * - Small fonts (16x16) use only the first 16 bytes
 * - Large fonts (32x32) use all 32 bytes, organized as 16 rows of 2 bytes each
 * 
 * Memory Usage (configurable for embedded systems):
 * 
 * High-memory systems (FONT_CACHE_ENABLED=1):
 * - Font cache: 16 × 36 bytes = 576 bytes
 * - Static font data buffer: 32 bytes
 * - Font size bitmap: ~8KB (loaded from file)
 * - Total: ~8.6KB
 * 
 * Low-memory systems like ESP32 (FONT_CACHE_ENABLED=0):
 * - No font cache: 0 bytes
 * - Static font data buffer: 32 bytes  
 * - Font size bitmap: ~8KB (loaded from file)
 * - Total: ~8KB
 * 
 * Ultra-low memory systems:
 * - Can define ESP32_PLATFORM to use minimal 4-character cache
 * - Cache: 4 × 36 bytes = 144 bytes
 * - Total: ~8.2KB
 */

static void *fp_font_data = NULL;
static unsigned char *font_size_bitmap = NULL;  // Bitmap storing font size info for each character
static unsigned char current_font_size = 0;     // Current character's font size (16 or 32)
static unsigned char font_data[FONT_DATA_SIZE]; // Font bitmap data for current character
static unsigned short font_wChar = 0xFFFF;      // Current cached character

// Memory configuration for different platforms
#ifndef FONT_CACHE_ENABLED
#ifdef ESP32_PLATFORM
#define FONT_CACHE_ENABLED 0 // Disable cache on ESP32
#else
#define FONT_CACHE_ENABLED 1 // Enable cache on other platforms
#endif
#endif

#if FONT_CACHE_ENABLED
// Performance optimization: small cache for embedded systems
#define FONT_CACHE_SIZE_SMALL  4  // Very small cache for memory-constrained systems
#define FONT_CACHE_SIZE_NORMAL 16 // Reduced normal cache size

// Conditional cache size based on available memory
#ifdef ESP32_PLATFORM
#define FONT_CACHE_SIZE FONT_CACHE_SIZE_SMALL
#else
#define FONT_CACHE_SIZE FONT_CACHE_SIZE_NORMAL
#endif

static struct
{
    unsigned short wChar;
    unsigned char font_size;
    unsigned char padding[1]; // Padding for alignment
    unsigned char font_data[FONT_DATA_SIZE];
} font_cache[FONT_CACHE_SIZE];
static int cache_next_slot = 0;
#endif

// Lightweight font width calculation for memory-constrained systems
// Replaces the large lookup table with computation
static unsigned char PAL_CharWidthCompute(unsigned short wChar)
{
    if (font_size_bitmap == NULL)
    {
        return 0;
    }

    // Check for invalid char code
    if ((wChar >= unicode_lower_top && wChar < unicode_upper_base) || (wChar >= unicode_upper_top))
    {
        return 0;
    }

    // Normalize character code
    if (wChar >= unicode_upper_base)
    {
        wChar -= (unicode_upper_base - unicode_lower_top);
    }

    // Fast bit manipulation to get font size
    unsigned char size_byte = font_size_bitmap[wChar >> 3]; // Divide by 8 using bit shift
    unsigned char bit_position = wChar & 7;                 // Modulo 8 using bit mask

    // Return width based on font size bit
    return (size_byte & (1U << bit_position)) ? FONT_WIDTH_LARGE : FONT_WIDTH_SMALL;
}

//
// Helper function to check if a character uses large font (32x32) or small font (16x16)
// Returns: FONT_SIZE_LARGE (32) for large font, FONT_SIZE_SMALL (16) for small font
//
static unsigned char PAL_GetCharFontSize(unsigned short wChar)
{
    if (font_size_bitmap == NULL)
    {
        return FONT_SIZE_SMALL; // Default to small font if buffer not available
    }

    // Normalize character code
    if (wChar >= unicode_upper_base)
    {
        wChar -= (unicode_upper_base - unicode_lower_top);
    }

    // Read font size bitmap: each byte contains size info for 8 characters
    unsigned char size_byte = font_size_bitmap[wChar / BITS_PER_BYTE];
    unsigned char bit_position = wChar % BITS_PER_BYTE;

    // If the bit is set, use large font (32x32), otherwise small font (16x16)
    return (size_byte & (1U << bit_position)) ? FONT_SIZE_LARGE : FONT_SIZE_SMALL;
}

//
// Optimized font data retrieval - with optional caching for memory efficiency
// Returns pointer to font data and sets font_size
//
static unsigned char *PAL_GetFontData(unsigned short wChar, unsigned char *out_font_size)
{
#if FONT_CACHE_ENABLED
    // Check cache first (only if caching is enabled)
    for (int i = 0; i < FONT_CACHE_SIZE; i++)
    {
        if (font_cache[i].wChar == wChar)
        {
            *out_font_size = font_cache[i].font_size;
            return font_cache[i].font_data;
        }
    }
#endif

    // Not in cache or caching disabled, load from file
    if (fp_font_data == NULL)
    {
        *out_font_size = FONT_SIZE_SMALL;
        return NULL;
    }

    // Get font size
    *out_font_size = PAL_GetCharFontSize(wChar);

    // Read font data
    UTIL_fseek(fp_font_data, wChar * FONT_DATA_SIZE, SEEK_SET);

#if FONT_CACHE_ENABLED
    // Store in cache if caching is enabled
    int cache_slot = cache_next_slot;
    cache_next_slot = (cache_next_slot + 1) % FONT_CACHE_SIZE;

    font_cache[cache_slot].wChar = wChar;
    font_cache[cache_slot].font_size = *out_font_size;
    UTIL_fread(font_cache[cache_slot].font_data, sizeof(unsigned char), FONT_DATA_SIZE, fp_font_data);

    return font_cache[cache_slot].font_data;
#else
    // Direct read to static buffer when caching is disabled
    UTIL_fread(font_data, sizeof(unsigned char), FONT_DATA_SIZE, fp_font_data);
    return font_data;
#endif
}

/*
 * Initialize the font system by loading font data and size bitmap.
 * This function must be called before any font rendering operations.
 * 
 * Returns:
 *   0 - Success
 *  -1 - Failed to open font data file
 *  -2 - Failed to open font size file
 *  -3 - Failed to allocate memory for font size bitmap
 *  -4 - Failed to read font size bitmap
 */
int PAL_InitFont(void)
{
    font_wChar = 0xFFFF; // Reset cached character

#if FONT_CACHE_ENABLED
    cache_next_slot = 0; // Reset cache

    // Initialize font cache (only if enabled)
    for (int i = 0; i < FONT_CACHE_SIZE; i++)
    {
        font_cache[i].wChar = 0xFFFF; // Invalid character code
        font_cache[i].font_size = 0;
    }
#endif

    // Open font data file containing actual bitmap data
    fp_font_data = UTIL_fopen(CACHES_PATH "/unicode_font.bin", "rb");
    if (fp_font_data == NULL)
    {
        return -1; // Failed to open font data file
    }

    // Load font size bitmap if not already loaded
    // This bitmap determines whether each character uses 16x16 or 32x32 font
    if (font_size_bitmap == NULL)
    {
        void *fp_font_size = UTIL_fopen(CACHES_PATH "/unicode_font_size.bin", "rb");
        if (fp_font_size == NULL)
        {
            UTIL_fclose(fp_font_data);
            fp_font_data = NULL;
            return -2; // Failed to open font size file
        }

        unsigned int bitmap_size = UTIL_FileLength(fp_font_size);
        if (bitmap_size == 0)
        {
            UTIL_fclose(fp_font_size);
            UTIL_fclose(fp_font_data);
            fp_font_data = NULL;
            return -2; // Invalid font size file
        }

        font_size_bitmap = (unsigned char *)UTIL_calloc(bitmap_size, sizeof(unsigned char));
        if (font_size_bitmap == NULL)
        {
            UTIL_fclose(fp_font_size);
            UTIL_fclose(fp_font_data);
            fp_font_data = NULL;
            return -3; // Failed to allocate memory for font size bitmap
        }

        size_t read_size = UTIL_fread(font_size_bitmap, sizeof(unsigned char), bitmap_size, fp_font_size);
        UTIL_fclose(fp_font_size);

        if (read_size != bitmap_size)
        {
            UTIL_free(font_size_bitmap);
            font_size_bitmap = NULL;
            UTIL_fclose(fp_font_data);
            fp_font_data = NULL;
            return -4; // Failed to read font size bitmap
        }
    }

    return 0; // Success
}

void PAL_DeInitFont(void)
{
    UTIL_fclose(fp_font_data);
    UTIL_free(font_size_bitmap);
    fp_font_data = NULL;
    font_size_bitmap = NULL;

#if FONT_CACHE_ENABLED
    // Reset cache state (only if enabled)
    cache_next_slot = 0;

    // Clear font cache
    for (int i = 0; i < FONT_CACHE_SIZE; i++)
    {
        font_cache[i].wChar = 0xFFFF;
        font_cache[i].font_size = 0;
    }
#endif
}

void PAL_DrawCharOnSurface(
    unsigned short wChar,
    const unsigned short x,
    const unsigned short y,
    const unsigned char bColor,
    const unsigned char fShadow)
{
    // Ensure font system is initialized
    if ((gpScreen == NULL) || (fp_font_data == NULL) || (font_size_bitmap == NULL))
    {
        return;
    }

    // Check for invalid char code.
    if ((wChar >= unicode_lower_top && wChar < unicode_upper_base) || (wChar >= unicode_upper_top))
    {
        return;
    }

    // Locate for this character in the font lib.
    if (wChar >= unicode_upper_base)
    {
        wChar -= (unicode_upper_base - unicode_lower_top);
    }

    // Get optimized font data with caching
    unsigned char font_size;
    unsigned char *char_font_data = PAL_GetFontData(wChar, &font_size);

    // Prepare for drawing on the screen surface
    unsigned char *dst = gpScreen->pixels + gpScreen->w * ReLU(y) + x;
    unsigned char *top = gpScreen->pixels + gpScreen->w * gpScreen->h;

    // Pre-calculate font metrics to avoid repeated calculations
    const unsigned char font_height = font_size;
    const unsigned char font_width = font_size >> 1; // Division by 2 using bit shift
    const unsigned char row_increment = (font_size == FONT_SIZE_LARGE) ? 2 : 1;
    const unsigned char is_large_font = (font_size == FONT_SIZE_LARGE);

    for (unsigned short i = 0; i < font_height && dst < top; i += row_increment, dst += gpScreen->w)
    {
        unsigned char *shadow_row = dst + gpScreen->w;
        for (unsigned short j = 0; j < font_width && (x + j) < gpScreen->w; j++)
        {
            unsigned char byte_offset = (is_large_font && j >= BITS_PER_BYTE) ? 1 : 0;
            unsigned char bit_mask = 1 << (j & 7);
            if (char_font_data[i + byte_offset] & bit_mask)
            {
                dst[j] = bColor;
                // Draw shadow with optimized bounds checking
                if (fShadow)
                {
                    unsigned char can_draw_right = (x + j + 1 < gpScreen->w);
                    unsigned char can_draw_bottom = (shadow_row + j < top);

                    if (can_draw_right)
                        dst[j + 1] = 0;
                    if (can_draw_bottom)
                    {
                        shadow_row[j] = 0;
                        if (can_draw_right)
                            shadow_row[j + 1] = 0;
                    }
                }
            }
        }
    }
}

unsigned char PAL_CharWidth(unsigned short wChar)
{
    // Direct computation - no cache needed for memory-constrained systems
    return PAL_CharWidthCompute(wChar);
}
