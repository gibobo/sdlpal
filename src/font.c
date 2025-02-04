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
#include <stdio.h>
#include <stdlib.h>

#define ReLU(A)  ((A) > 0 ? (A) : 0)
#define unicode_lower_top	0xD800
#define unicode_upper_base  0xF900
#define unicode_upper_top	0xFFFE

static FILE *fp_font_data = NULL;
static FILE *fp_font_size = NULL;
static unsigned char *p_font = NULL;
static unsigned char *p_font_size = NULL;

void PAL_InitFont(void)
{
    p_font = (unsigned char *)UTIL_calloc(65536, 32);
    p_font_size = (unsigned char *)UTIL_calloc(65536, 1);
    fp_font_data = UTIL_fopen(SOURCE_DIR "/unicode_font.dat", "rb");
    fp_font_size = UTIL_fopen(SOURCE_DIR "/unicode_font_size.dat", "rb");
    UTIL_fread(p_font, 32, 65536, fp_font_data);
    UTIL_fread(p_font_size, 1, 65536, fp_font_size);
}

void PAL_DeInitFont(void)
{
    free(p_font);
    free(p_font_size);
    UTIL_fclose(fp_font_data);
    UTIL_fclose(fp_font_size);
}

void PAL_DrawCharOnSurface(
    unsigned short wChar,
    const unsigned int x,
    const unsigned int y,
    const unsigned char bColor)
{
    unsigned int i;
    unsigned int j;

    // Check for NULL pointer & invalid char code.
    if ((gpScreen == NULL) ||
        (wChar >= unicode_lower_top && wChar < unicode_upper_base) ||
        (wChar >= unicode_upper_top))
    {
        return;
    }

    // Locate for this character in the font lib.
    if (wChar >= unicode_upper_base)
    {
        wChar -= (unicode_upper_base - unicode_lower_top);
    }

    // Draw the character to the surface.
    unsigned char *dst = gpScreen->pixels + gpScreen->w * ReLU(y) + x;
    unsigned char *top = gpScreen->pixels + gpScreen->w * gpScreen->h;
    unsigned char *font = p_font + wChar * 32;
    unsigned char font_size = (p_font_size[wChar / 8] & (1 << (wChar % 8))) ? 32 : 16;

    for (i = 0; i < font_size && dst < top; i++, dst += gpScreen->w)
    {
        for (j = 0; j < 8 && x + j < gpScreen->w && x + j >= 0; j++)
        {
            if (font[i] & (1 << j % 8))
            {
                dst[j] = bColor;
            }
        }
        if (font_size == 32)
        {
            i++;
            for (j = 8; j < 16 && x + j < gpScreen->w && x + j >= 0; j++)
            {
                if (font[i] & (1 << j % 8))
                {
                    dst[j] = bColor;
                }
            }
        }
    }
}

int PAL_CharWidth(unsigned short wChar)
{
    if ((wChar >= unicode_lower_top && wChar < unicode_upper_base) || wChar >= unicode_upper_top)
    {
        return 0;
    }

    // Locate for this character in the font lib.
    if (wChar >= unicode_upper_base)
    {
        wChar -= (unicode_upper_base - unicode_lower_top);
    }

    return (p_font_size[wChar / 8] & (1 << (wChar % 8))) ? 16 : 8;;
}
