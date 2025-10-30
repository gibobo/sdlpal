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

#define FONT_DATA_CACHE_SIZE 256
#define ReLU(A)              ((A) > 0 ? (A) : 0)
#define unicode_lower_top    0xD800
#define unicode_upper_base   0xF900
#define unicode_upper_top    0xFFFE

typedef struct
{
    unsigned short wChar;
    unsigned char size;
    unsigned char data[32];
} font_data_cache;

static void *fp_font_data = NULL;
static void *fp_font_size = NULL;
static font_data_cache *font_cache = NULL;
static unsigned short font_cache_index = 0;
static unsigned short font_cache_filled = 0;
static font_data_cache *font_cache_last_hit = NULL;

static font_data_cache *FontCacheLookupOrLoad(unsigned short wChar)
{
    if (font_cache == NULL || fp_font_data == NULL || fp_font_size == NULL)
        return NULL;

    // Check for invalid char code.
    if ((wChar >= unicode_lower_top && wChar < unicode_upper_base) || (wChar >= unicode_upper_top))
        return NULL;

    if (font_cache_last_hit != NULL && font_cache_last_hit->size > 0 && font_cache_last_hit->wChar == wChar)
        return font_cache_last_hit;

    for (unsigned short i = 0; i < font_cache_filled; i++)
    {
        font_data_cache *entry = &font_cache[i];
        if (entry->size > 0 && entry->wChar == wChar)
        {
            font_cache_last_hit = entry;
            return entry;
        }
    }

    font_data_cache *pCache = &font_cache[font_cache_index];
    unsigned short current_index = font_cache_index;
    unsigned char was_empty = (pCache->size == 0);

    unsigned short wCharOffset = wChar;
    if (wCharOffset >= unicode_upper_base)
        wCharOffset -= (unicode_upper_base - unicode_lower_top);

    unsigned char size_bits;
    if (UTIL_fseek(fp_font_size, sizeof(unsigned char) * wCharOffset / 8, SEEK_SET) != 0)
    {
        pCache->size = 0;
        return NULL;
    }
    if (UTIL_fread(&size_bits, sizeof(unsigned char), 1, fp_font_size) != 1)
    {
        pCache->size = 0;
        return NULL;
    }
    if (UTIL_fseek(fp_font_data, sizeof(unsigned char) * wCharOffset * 32, SEEK_SET) != 0)
    {
        pCache->size = 0;
        return NULL;
    }
    if (UTIL_fread(pCache->data, sizeof(unsigned char), 32, fp_font_data) != 32)
    {
        pCache->size = 0;
        return NULL;
    }

    pCache->wChar = wChar;
    pCache->size = (size_bits & (1U << (wCharOffset % 8))) ? 2 : 1;
    font_cache_last_hit = pCache;

    if (was_empty && font_cache_filled < FONT_DATA_CACHE_SIZE)
    {
        if (current_index >= font_cache_filled)
            font_cache_filled = current_index + 1;
        else
            font_cache_filled++;
    }

    font_cache_index = (current_index + 1) % FONT_DATA_CACHE_SIZE;
    return pCache;
}

void PAL_InitFont(void)
{
    fp_font_data = UTIL_fopen(UTIL_Filename("%s/unicode_font.bin", CACHES_PATH), "rb");
    fp_font_size = UTIL_fopen(UTIL_Filename("%s/unicode_font_size.bin", CACHES_PATH), "rb");
    font_cache = (font_data_cache *)UTIL_calloc(FONT_DATA_CACHE_SIZE, sizeof(font_data_cache));
    font_cache_index = 0;
    font_cache_filled = 0;
    font_cache_last_hit = NULL;
}

void PAL_DeInitFont(void)
{
    UTIL_fclose(fp_font_data);
    UTIL_fclose(fp_font_size);
    UTIL_free(font_cache);
    fp_font_data = NULL;
    fp_font_size = NULL;
    font_cache = NULL;
    font_cache_index = 0;
    font_cache_filled = 0;
    font_cache_last_hit = NULL;
}

void PAL_DrawCharOnSurface(
    unsigned short wChar,
    const unsigned short x,
    const unsigned short y,
    const unsigned char bColor,
    const unsigned char fShadow)
{
    unsigned short i;
    unsigned short j;
    unsigned char font_size;
    font_data_cache *pCache = FontCacheLookupOrLoad(wChar);

    // Check for cache miss
    if (pCache == NULL)
        return;

    // Check for NULL screen surface.
    if (gpScreen == NULL)
        return;

    font_size = pCache->size << 4;

    // Draw the character to the surface.
    unsigned char *dst = gpScreen->pixels + gpScreen->w * ReLU(y) + x;
    unsigned char *top = gpScreen->pixels + gpScreen->w * gpScreen->h;
    for (i = 0; i < font_size && dst < top; i += pCache->size, dst += gpScreen->w)
    {
        unsigned char *shadow_row = dst + gpScreen->w;
        for (j = 0; (j < (pCache->size << 3)) && ((x + j) < gpScreen->w); j++)
        {
            if (pCache->data[i + ((pCache->size == 2 && j >= 8) ? 1 : 0)] & (1 << (j % 8)))
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
    font_data_cache *pCache = FontCacheLookupOrLoad(wChar);
    if (pCache == NULL)
        return 0;

    return pCache->size;
}
