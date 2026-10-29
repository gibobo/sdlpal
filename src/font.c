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
    uint8_t glyphWidth; // The width of the glyph in 8-pixel units; 0 if the entry is empty
    int16_t next;       // The next entry in the same hash bucket, or -1
    uint8_t data[32];   // The bitmap data for the glyph
} FontGlyphCacheEntry;

// Font glyph cache definitions
#define FONT_CACHE_CAPACITY 256
#define FONT_CACHE_BUCKETS 512
// Unicode codepoint ranges
static const uint16_t kUnicodeSurrogateRangeBegin = 0xD800;
static const uint16_t kUnicodePrivateUseBase = 0xF900;
static const uint16_t kUnicodeInvalidUpperBound = 0xFFFE;
// The font files skip the surrogate range, so this many glyphs follow each other.
#define FONT_GLYPH_COUNT (0xFFFE - (0xF900 - 0xD800))

static void *gFontDataStream = NULL;
static uint8_t *gFontWidthBits = NULL; // one bit per glyph, set for full-width glyphs
static FontGlyphCacheEntry *gFontCache = NULL;
static int16_t gFontCacheBuckets[FONT_CACHE_BUCKETS];
static uint16_t gFontCacheNextIndex = 0;
static FontGlyphCacheEntry *gFontCacheLastHit = NULL;

// The glyph index of a codepoint, or -1 for a codepoint the font does not cover.
static int32_t FontGlyphIndex(uint16_t codepoint)
{
    if ((codepoint >= kUnicodeSurrogateRangeBegin && codepoint < kUnicodePrivateUseBase) ||
        (codepoint >= kUnicodeInvalidUpperBound))
        return -1;
    if (codepoint >= kUnicodePrivateUseBase)
        return codepoint - (kUnicodePrivateUseBase - kUnicodeSurrogateRangeBegin);
    return codepoint;
}

// The width of a glyph in 8-pixel units.
static uint8_t FontGlyphWidth(int32_t glyphIndex)
{
    return (gFontWidthBits[glyphIndex / 8] & (1U << (glyphIndex % 8))) ? 2 : 1;
}

// Remove a cache entry from its hash bucket.
static void FontCacheUnlink(int16_t index)
{
    int16_t *link = &gFontCacheBuckets[gFontCache[index].codepoint % FONT_CACHE_BUCKETS];
    while (*link != index)
        link = &gFontCache[*link].next;
    *link = gFontCache[index].next;
}

static FontGlyphCacheEntry *FontCacheFetchGlyph(uint16_t codepoint)
{
    int32_t glyphIndex = FontGlyphIndex(codepoint);
    int16_t *bucket;
    int16_t index;

    if (gFontCache == NULL || glyphIndex < 0)
        return NULL;

    if (gFontCacheLastHit != NULL && gFontCacheLastHit->glyphWidth > 0 && gFontCacheLastHit->codepoint == codepoint)
        return gFontCacheLastHit;

    // Look the glyph up in its hash bucket.
    bucket = &gFontCacheBuckets[codepoint % FONT_CACHE_BUCKETS];
    for (index = *bucket; index >= 0; index = gFontCache[index].next)
    {
        if (gFontCache[index].codepoint == codepoint)
        {
            gFontCacheLastHit = &gFontCache[index];
            return gFontCacheLastHit;
        }
    }

    // Replace the oldest entry with the glyph read from the font file.
    index = (int16_t)gFontCacheNextIndex;
    gFontCacheNextIndex = (gFontCacheNextIndex + 1) % FONT_CACHE_CAPACITY;
    if (gFontCache[index].glyphWidth > 0)
    {
        FontCacheUnlink(index);
        gFontCache[index].glyphWidth = 0;
    }
    if (UTIL_fseek(gFontDataStream, (int64_t)glyphIndex * 32, SEEK_SET) != 0 ||
        UTIL_fread(gFontCache[index].data, sizeof(uint8_t), 32, gFontDataStream) != 32)
        return NULL;

    gFontCache[index].codepoint = codepoint;
    gFontCache[index].glyphWidth = FontGlyphWidth(glyphIndex);
    gFontCache[index].next = *bucket;
    *bucket = index;
    gFontCacheLastHit = &gFontCache[index];
    return gFontCacheLastHit;
}

void PAL_InitFont(void)
{
    const uint32_t widthBytes = (FONT_GLYPH_COUNT + 7) / 8;
    void *fpSize;
    int i;

    // Keep the glyph widths in memory; glyph bitmaps are read when first drawn.
    gFontDataStream = UTIL_fopen(UTIL_Filename("%s/unicode_font.bin", CACHES_PATH), "rb");
    if (UTIL_FileLength(gFontDataStream) < (long)FONT_GLYPH_COUNT * 32)
        TerminateOnError("%s() failed: unicode_font.bin is shorter than %d glyphs\n", __func__, FONT_GLYPH_COUNT);
    fpSize = UTIL_fopen(UTIL_Filename("%s/unicode_font.dat", CACHES_PATH), "rb");
    gFontWidthBits = (uint8_t *)UTIL_malloc(widthBytes);
    if (UTIL_fread(gFontWidthBits, 1, widthBytes, fpSize) != widthBytes)
        TerminateOnError("%s() failed: unicode_font.dat is shorter than %u bytes\n", __func__, widthBytes);
    UTIL_fclose(fpSize);

    gFontCache = (FontGlyphCacheEntry *)UTIL_calloc(FONT_CACHE_CAPACITY, sizeof(FontGlyphCacheEntry));
    for (i = 0; i < FONT_CACHE_BUCKETS; i++)
        gFontCacheBuckets[i] = -1;
    gFontCacheNextIndex = 0;
    gFontCacheLastHit = NULL;
}

void PAL_DeInitFont(void)
{
    UTIL_fclose(gFontDataStream);
    UTIL_free(gFontWidthBits);
    UTIL_free(gFontCache);
    gFontDataStream = NULL;
    gFontWidthBits = NULL;
    gFontCache = NULL;
    gFontCacheNextIndex = 0;
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
    int32_t glyphIndex = FontGlyphIndex(codepoint);
    if (gFontWidthBits == NULL || glyphIndex < 0)
        return 0;

    return FontGlyphWidth(glyphIndex);
}
