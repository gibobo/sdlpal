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

#define ReLU(A)  ((A) > 0 ? (A) : 0)
#define unicode_lower_top	0xD800
#define unicode_upper_base  0xF900
#define unicode_upper_top	0xFFFE

static void *fp_font_data = NULL;
static void *fp_font_size = NULL;

void PAL_InitFont(void)
{
    fp_font_data = UTIL_fopen("unicode_font.dat", "rb");
    fp_font_size = UTIL_fopen("unicode_font_size.dat", "rb");
}

void PAL_DeInitFont(void)
{
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
    if (gpScreen == NULL)
        return;

    if ((wChar >= unicode_lower_top && wChar < unicode_upper_base) ||
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
    unsigned char font_size;
    unsigned char font_data[32];
    unsigned char *dst = gpScreen->pixels + gpScreen->w * ReLU(y) + x;
    unsigned char *top = gpScreen->pixels + gpScreen->w * gpScreen->h;

    UTIL_fseek(fp_font_size, sizeof(unsigned char) * wChar / 8, SEEK_SET);
    UTIL_fread(&font_size, sizeof(unsigned char), 1, fp_font_size);

    UTIL_fseek(fp_font_data, sizeof(unsigned char) * wChar * 32, SEEK_SET);
    UTIL_fread(font_data, sizeof(unsigned char), 32, fp_font_data);

    font_size = (font_size & (1 << (wChar % 8))) ? 32 : 16;

    for (i = 0; i < font_size && dst < top; i++, dst += gpScreen->w)
    {
        for (j = 0; j < 8 && x + j < gpScreen->w && x + j >= 0; j++)
        {
            if (font_data[i] & (1 << j % 8))
            {
                dst[j] = bColor;
            }
        }
        if (font_size == 32)
        {
            i++;
            for (j = 8; j < 16 && x + j < gpScreen->w && x + j >= 0; j++)
            {
                if (font_data[i] & (1 << j % 8))
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

    unsigned char font_size;
    UTIL_fseek(fp_font_size, sizeof(unsigned char) * wChar / 8, SEEK_SET);
    UTIL_fread(&font_size, sizeof(unsigned char), 1, fp_font_size);

    return (font_size & (1 << (wChar % 8))) ? 16 : 8;
}
