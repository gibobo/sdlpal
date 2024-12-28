/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
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
#include "palcommon.h"
#include "common.h"
#include "text.h"
#include "util.h"

#define _FONT_C

#include "fontglyph.h"
#include "ascii.h"

static int _font_height = 16;

static unsigned char reverseBits(unsigned char x) {
    unsigned char y = 0;
    for (int i = 0 ; i < 8; i++){
        y <<= 1;
        y |= (x & 1);
        x >>= 1;
    }
    return y;
}

static void PAL_LoadISOFont(void)
{
    int         i, j;

    for (i = 0; i < sizeof(iso_font) / 15; i++)
    {
        for (j = 0; j < 15; j++)
        {
            unicode_font[i][j] = reverseBits(iso_font[i * 15 + j]);
        }

        unicode_font[i][15] = 0;
        font_width[i] = 16;
    }
}

int
PAL_InitFont(
   const CONFIGURATION* cfg
)
{
   PAL_LoadISOFont();
   return 0;
}

void
PAL_DrawCharOnSurface(
	unsigned short                 wChar,
	SDL_Surface             *lpSurface,
	unsigned int                  pos,
	unsigned char                  bColor,
	int                     fUse8x8Font
)
{
	int       i, j;
	int       x = PAL_X(pos), y = PAL_Y(pos);
    int       x_offset = 0;
    int       y_offset = 0;

	//
	// Check for NULL pointer & invalid char code.
	//
	if (lpSurface == NULL || (wChar >= unicode_lower_top && wChar < unicode_upper_base) ||
		wChar >= unicode_upper_top || (_font_height == 8 && wChar >= 0x100))
	{
		return;
	}

	//
	// Locate for this character in the font lib.
	//
	if (wChar >= unicode_upper_base)
	{
		wChar -= (unicode_upper_base - unicode_lower_top);
	}

	//
	// Draw the character to the surface.
	//
	unsigned char * dest = (unsigned char *)lpSurface->pixels + (int)max(y + y_offset, 0) * lpSurface->pitch + x;
	unsigned char * top = (unsigned char *)lpSurface->pixels + lpSurface->h * lpSurface->pitch;
	if (fUse8x8Font)
	{
		for (i = 0; i < 8 && dest < top; i++, dest += lpSurface->pitch)
		{
			for (j = 0; j < 8 && x + j < lpSurface->w; j++)
			{
				if (iso_font_8x8[wChar][i] & (1 << j))
				{
					dest[j] = bColor;
				}
			}
		}
	}
	else
	{
		if (font_width[wChar] == 32)
		{
			for (i = 0; i < _font_height * 2 && dest < top; i += 2, dest += lpSurface->pitch)
			{
				for (j = 0; j < 8 && x + j + x_offset < lpSurface->w && x + j + x_offset >= 0; j++)
				{
					if (unicode_font[wChar][i] & (1 << (7 - j)))
					{
						dest[j + x_offset] = bColor;
					}
				}
				for (j = 0; j < 8 && x + j + 8 + x_offset < lpSurface->w && x + j + 8 + x_offset >= 0; j++)
				{
					if (unicode_font[wChar][i + 1] & (1 << (7 - j)))
					{
						dest[j + 8 + x_offset] = bColor;
					}
				}
			}
		}
		else
		{
			for (i = 0; i < _font_height && dest < top; i++, dest += lpSurface->pitch)
			{
				for (j = 0; j < 8 && x + j + x_offset < lpSurface->w && x + j + x_offset >= 0; j++)
				{
					if (unicode_font[wChar][i] & (1 << (7 - j)))
					{
						dest[j + x_offset] = bColor;
					}
				}
			}
		}
	}
}

int
PAL_CharWidth(
	unsigned short                 wChar
)
{
	if ((wChar >= unicode_lower_top && wChar < unicode_upper_base) || wChar >= unicode_upper_top)
	{
		return 0;
	}

	//
	// Locate for this character in the font lib.
	//
	if (wChar >= unicode_upper_base)
	{
		wChar -= (unicode_upper_base - unicode_lower_top);
	}

	return font_width[wChar] >> 1;
}

int
PAL_FontHeight(
	void
)
{
	return _font_height;
}
