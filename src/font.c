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
#include "fontglyph.h"
#include "palcommon.h"
#include "text.h"
#include "util.h"

#define ReLU(A)  ((A) > 0 ? (A) : 0)

#define unicode_lower_top	0xD800
#define unicode_upper_base  0xF900
#define unicode_upper_top	0xFFFE

static unsigned char *p_font = NULL;

void PAL_InitFont(void)
{
    p_font = (unsigned char *)UTIL_calloc(65535, 32);
#if 1
    FILE *fp = fopen(SOURCE_DIR "/unicode_font.dat", "rb");
    fread(p_font, 32, 65535, fp);
    fclose(fp);
#endif
#if 0
    FILE *fp = fopen("unicode_font.dat", "wb");
    for (size_t i = 0; i < 65535; i++) {
      if (font_width[i] == 16)
        memset(p_font + i * 32, 0, 16);
      if (font_width[i] == 0)
        memset(p_font + i * 32, 0, 32);
    }
    fwrite(p_font, 1, 65535 * 32, fp);
    fclose(fp);

    // fp = fopen("font_width.txt", "w");
    // fprintf(fp, "unsigned char font_width[] = {\r");
    // for (size_t i = 0; i < 65535; i++)
    // {
    //     fprintf(fp, "%d ,", font_width[i]);
    //     if (font_width[i] != 16 && font_width[i] != 32)
    //         printf("?");
    //     else
    //         printf(".");
    // }
    // fprintf(fp, "};\n");
    // fclose(fp);
    #endif
}

void PAL_DrawCharOnSurface(
    unsigned short wChar,
    PAL_Surface *lpSurface,
    unsigned int x,
    unsigned int y,
    unsigned char bColor)
{
    int       i;
    int       j;

    // Check for NULL pointer & invalid char code.
    if ((lpSurface == NULL) ||
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
    unsigned char *dst = lpSurface->pixels + lpSurface->w * ReLU(y) + x;
    unsigned char *top = lpSurface->pixels + lpSurface->w * lpSurface->h;
    unsigned char *font = p_font + wChar * 32;

    for (i = 0; i < font_width[wChar] && dst < top; i++, dst += lpSurface->w)
    {
        for (j = 0; j < 8 && x + j < lpSurface->w && x + j >= 0; j++)
        {
            if ((font[i] << j) & 0x80)
            {
                dst[j] = bColor;
            }
        }
        if (font_width[wChar] == 32)
        {
            i++;
            for (j = 8; j < 16 && x + j < lpSurface->w && x + j >= 0; j++)
            {
                if (font[i] & (1 << (15 - j)))
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

    return font_width[wChar] >> 1;
}
