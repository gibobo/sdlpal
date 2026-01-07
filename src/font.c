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
#include <stddef.h>
#include <stdlib.h>

// Font glyph cache entry structure
typedef struct
{
    uint16_t codepoint; // The Unicode codepoint of the glyph
    uint8_t glyphWidth; // The width of the glyph in pixels
    uint8_t data[32];   // The bitmap data for the glyph
} FontGlyphCacheEntry;

// Font glyph cache definitions
static const uint16_t kFontCacheCapacity = 256;
// Unicode codepoint ranges
static const uint16_t kUnicodeSurrogateRangeBegin = 0xD800;
static const uint16_t kUnicodePrivateUseBase = 0xF900;
static const uint16_t kUnicodeInvalidUpperBound = 0xFFFE;

static void *gFontDataStream = NULL;
static void *gFontSizeStream = NULL;
static FontGlyphCacheEntry *gFontCache = NULL;
static uint16_t gFontCacheNextIndex = 0;
static uint16_t gFontCachePopulation = 0;
static FontGlyphCacheEntry *gFontCacheLastHit = NULL;

static FontGlyphCacheEntry *FontCacheFetchGlyph(uint16_t codepoint)
{
    if (gFontCache == NULL || gFontDataStream == NULL || gFontSizeStream == NULL)
        return NULL;

    // Check for invalid char code.
    if ((codepoint >= kUnicodeSurrogateRangeBegin && codepoint < kUnicodePrivateUseBase) ||
        (codepoint >= kUnicodeInvalidUpperBound))
        return NULL;

    if (gFontCacheLastHit != NULL && gFontCacheLastHit->glyphWidth > 0 && gFontCacheLastHit->codepoint == codepoint)
        return gFontCacheLastHit;

    for (uint16_t i = 0; i < gFontCachePopulation; i++)
    {
        FontGlyphCacheEntry *entry = &gFontCache[i];
        if (entry->glyphWidth > 0 && entry->codepoint == codepoint)
        {
            gFontCacheLastHit = entry;
            return entry;
        }
    }

    FontGlyphCacheEntry *cacheEntry = &gFontCache[gFontCacheNextIndex];
    uint16_t writeIndex = gFontCacheNextIndex;
    uint8_t entryWasEmpty = (cacheEntry->glyphWidth == 0);

    uint16_t glyphIndex = codepoint;
    if (glyphIndex >= kUnicodePrivateUseBase)
        glyphIndex -= (kUnicodePrivateUseBase - kUnicodeSurrogateRangeBegin);

    uint8_t widthBitMask;
    if (UTIL_fseek(gFontSizeStream, sizeof(uint8_t) * glyphIndex / 8, SEEK_SET) != 0)
    {
        cacheEntry->glyphWidth = 0;
        return NULL;
    }
    if (UTIL_fread(&widthBitMask, sizeof(uint8_t), 1, gFontSizeStream) != 1)
    {
        cacheEntry->glyphWidth = 0;
        return NULL;
    }
    if (UTIL_fseek(gFontDataStream, sizeof(uint8_t) * glyphIndex * 32, SEEK_SET) != 0)
    {
        cacheEntry->glyphWidth = 0;
        return NULL;
    }
    if (UTIL_fread(cacheEntry->data, sizeof(uint8_t), 32, gFontDataStream) != 32)
    {
        cacheEntry->glyphWidth = 0;
        return NULL;
    }

    cacheEntry->codepoint = codepoint;
    cacheEntry->glyphWidth = (widthBitMask & (1U << (glyphIndex % 8))) ? 2 : 1;
    gFontCacheLastHit = cacheEntry;

    if (entryWasEmpty && gFontCachePopulation < kFontCacheCapacity)
    {
        if (writeIndex >= gFontCachePopulation)
            gFontCachePopulation = writeIndex + 1;
        else
            gFontCachePopulation++;
    }

    gFontCacheNextIndex = (writeIndex + 1) % kFontCacheCapacity;
    return cacheEntry;
}

void PAL_InitFont(void)
{
    gFontDataStream = UTIL_fopen(UTIL_Filename("%s/unicode_font.bin", CACHES_PATH), "rb");
    gFontSizeStream = UTIL_fopen(UTIL_Filename("%s/unicode_font.dat", CACHES_PATH), "rb");
    gFontCache = (FontGlyphCacheEntry *)UTIL_calloc(kFontCacheCapacity, sizeof(FontGlyphCacheEntry));
    gFontCacheNextIndex = 0;
    gFontCachePopulation = 0;
    gFontCacheLastHit = NULL;
}

void PAL_DeInitFont(void)
{
    UTIL_fclose(gFontDataStream);
    UTIL_fclose(gFontSizeStream);
    UTIL_free(gFontCache);
    gFontDataStream = NULL;
    gFontSizeStream = NULL;
    gFontCache = NULL;
    gFontCacheNextIndex = 0;
    gFontCachePopulation = 0;
    gFontCacheLastHit = NULL;
}

void PAL_DrawCharOnSurface(
    uint16_t codepoint,
    const uint16_t x,
    const uint16_t y,
    const uint8_t bColor,
    const uint8_t fShadow)
{
    uint16_t i;
    uint16_t j;
    FontGlyphCacheEntry *glyphEntry = FontCacheFetchGlyph(codepoint);

    // Check for cache miss
    if (glyphEntry == NULL)
        return;

    // Check for NULL screen surface.
    if (gpScreen == NULL)
        return;

    uint16_t screen_w = gpScreen->w;
    if (x >= screen_w)
        return;

    uint8_t glyphWidth = glyphEntry->glyphWidth;
    uint8_t rows = glyphWidth << 4;
    uint8_t bitsPerRow = glyphWidth << 3;
    uint16_t max_columns = screen_w - x;
    uint8_t column_limit = bitsPerRow;
    if (column_limit > max_columns)
        column_limit = (uint8_t)max_columns;

    if (column_limit == 0)
        return;

    uint8_t draw_shadow = fShadow ? 1 : 0;

    // Draw the character to the surface.
    uint8_t *dst = gpScreen->pixels + screen_w * y + x;
    uint8_t *top = gpScreen->pixels + screen_w * gpScreen->h;
    for (i = 0; i < rows && dst < top; i += glyphWidth, dst += screen_w)
    {
        uint8_t *shadow_row = dst + screen_w;
        uint8_t has_bottom = (shadow_row < top);
        uint8_t *glyph_row = glyphEntry->data + i;
        uint32_t glyph_bits = glyph_row[0];
        if (glyphWidth == 2)
            glyph_bits |= ((uint32_t)glyph_row[1]) << 8;

        if (glyph_bits == 0)
            continue;

        for (j = 0; j < column_limit; j++)
        {
            if ((glyph_bits >> j) & 0x1)
            {
                dst[j] = bColor;
                // Draw shadow with optimized bounds checking
                if (draw_shadow)
                {
                    uint8_t can_draw_right = (j + 1 < max_columns);

                    if (can_draw_right)
                        dst[j + 1] = 0;
                    if (has_bottom)
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

uint8_t PAL_CharWidth(uint16_t codepoint)
{
    FontGlyphCacheEntry *glyphEntry = FontCacheFetchGlyph(codepoint);
    if (glyphEntry == NULL)
        return 0;

    return glyphEntry->glyphWidth;
}
