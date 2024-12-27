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

#ifndef PALETTE_H
#define PALETTE_H

#include "common.h"
#include <SDL_pixels.h>

#ifdef __cplusplus
extern "C" {
#endif

SDL_Color *
PAL_GetPalette(
   int         iPaletteNum,
   BOOL        fNight
);

void
PAL_SetPalette(
   int         iPaletteNum,
   BOOL        fNight
);

void
PAL_FadeOut(
   int         iDelay
);

void
PAL_FadeIn(
   int         iPaletteNum,
   BOOL        fNight,
   int         iDelay
);

void
PAL_SceneFade(
   int         iPaletteNum,
   BOOL        fNight,
   int         iStep
);

void
PAL_PaletteFade(
   int         iPaletteNum,
   BOOL        fNight,
   BOOL        fUpdateScene
);

void
PAL_ColorFade(
   int        iDelay,
   BYTE       bColor,
   BOOL       fFrom
);

void
PAL_FadeToRed(
   void
);

#ifdef __cplusplus
}
#endif

#endif
