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

#ifndef FONT_H
#define FONT_H

#include "palcfg.h"

#define FONT_HEIGHT (16)

#ifdef __cplusplus
extern "C" {
#endif

/*++
  Purpose:

    Initialize the font subsystem.

  Parameters:

    [IN]  cfg - Pointer to the configuration object.

  Return value:

    None.

--*/
void PAL_InitFont(void);

/*++
  Purpose:

    Draw a Unicode character on a surface.

  Parameters:

    [IN]  wChar - the unicode character to be drawn.

    [OUT] lpSurface - the destination surface.

    [IN]  pos - the destination location of the surface.

    [IN]  bColor - the color of the character.

  Return value:

    None.

--*/
void
PAL_DrawCharOnSurface(
    unsigned short                 wChar,
    PAL_Surface              *lpSurface,
    unsigned int             pos,
    unsigned char                  bColor
);

/*++
  Purpose:

    Get the text width of a character.

  Parameters:

    [IN]  wChar - the unicode character for width calculation.

  Return value:

    The width of the character in pixels, 16 for full-width char and 8 for half-width char.

--*/
int PAL_CharWidth(unsigned short wChar);

#ifdef __cplusplus
}
#endif

#endif
