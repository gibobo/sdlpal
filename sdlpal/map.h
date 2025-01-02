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

#ifndef _MAP_H
#define _MAP_H

#include <stdio.h>
#include "video/video.h"

//
// Map format:
//
// +----------------------------------------------> x
// | * * * * * * * * * * ... * * * * * * * * * *  (y = 0, h = 0)
// |  * * * * * * * * * * ... * * * * * * * * * * (y = 0, h = 1)
// | * * * * * * * * * * ... * * * * * * * * * *  (y = 1, h = 0)
// |  * * * * * * * * * * ... * * * * * * * * * * (y = 1, h = 1)
// | * * * * * * * * * * ... * * * * * * * * * *  (y = 2, h = 0)
// |  * * * * * * * * * * ... * * * * * * * * * * (y = 2, h = 1)
// | ............................................
// v
// y
//
// Note:
//
// Tiles are in diamond shape (32x15).
//
// Each tile is represented with a unsigned int value, which contains information
// about the tile bitmap, block flag, height, etc.
//
// Bottom layer sprite index:
//  (d & 0xFF) | ((d >> 4) & 0x100)
//
// Top layer sprite index:
//  d >>= 16;
//  ((d & 0xFF) | ((d >> 4) & 0x100)) - 1)
//
// Block flag (player cannot walk through this tile):
//  d & 0x2000
//

typedef struct tagPALMAP
{
    unsigned int Tiles[128][64][2];
    unsigned char *pTileSprite;
    int iMapNum;
} PALMAP;

typedef const PALMAP *LPCPALMAP;

#ifdef __cplusplus
extern "C"
{
#endif

    PALMAP *PAL_LoadMap(
        int iMapNum,
        FILE *fpMapMKF,
        FILE *fpGopMKF);

    void PAL_FreeMap(
        PALMAP *lpMap);

    const unsigned char *
    PAL_MapGetTileBitmap(
        unsigned char x,
        unsigned char y,
        unsigned char h,
        unsigned char ucLayer,
        LPCPALMAP lpMap);

    int
    PAL_MapTileIsBlocked(
        unsigned char x,
        unsigned char y,
        unsigned char h,
        LPCPALMAP lpMap);

    unsigned char
    PAL_MapGetTileHeight(
        unsigned char x,
        unsigned char y,
        unsigned char h,
        unsigned char ucLayer,
        LPCPALMAP lpMap);

    void
    PAL_MapBlitToSurface(
        LPCPALMAP lpMap,
        PAL_Surface *lpSurface,
        const PAL_Rect *lpSrcRect,
        unsigned char ucLayer);

#ifdef __cplusplus
}
#endif

//
// Convert map location to the real location
//
#define PAL_XYH_TO_POS(x, y, h) \
    unsigned int((x) * 32 + (h) * 16, (y) * 16 + (h) * 8)

//
// Convert real location to map location
//
#define PAL_POS_TO_XYH(pos, x, y, h)                             \
    {                                                            \
        (h) = (unsigned char)(((PAL_X(pos) % 32) != 0) ? 1 : 0); \
        (x) = (unsigned char)(PAL_X(pos) / 32);                  \
        (y) = (unsigned char)(PAL_Y(pos) / 16);                  \
    }

#endif
