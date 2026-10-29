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

#include <stdint.h>

#define PALETTE_SIZE (256 * 3)

uint8_t *PAL_GetPalette(int32_t iPaletteNum, uint8_t fNight);

void PAL_SetPalette(int32_t iPaletteNum, uint8_t fNight);

void PAL_FadeOut(uint32_t iDelay);

void PAL_FadeIn(int32_t iPaletteNum, uint8_t fNight, uint16_t iDelay);

void PAL_SceneFade(int32_t iPaletteNum, uint8_t fNight, int iStep);

void PAL_PaletteFade(int32_t iPaletteNum, uint8_t fNight, int fUpdateScene);

void PAL_ColorFade(int32_t iDelay, uint8_t bColor, int fFrom);

void PAL_FadeToRed(void);

#endif