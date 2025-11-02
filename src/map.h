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

#include "video.h"

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

#define PALMAP_Y 128
#define PALMAP_X 64
#define PALMAP_Z 2

typedef struct tagPALMAP
{
    unsigned int *Tiles;
    unsigned int Len_Tiles;
    unsigned char *pTileSprite;
    unsigned int Len_TileSprite;
    int iMapNum;
} PALMAP;

void PAL_LoadMap(PALMAP *lpMap, int iMapNum);

void PAL_FreeMap(PALMAP *lpMap);

const unsigned char *PAL_MapGetTileBitmap(unsigned char x, unsigned char y, unsigned char h, unsigned char ucLayer, PALMAP *lpMap);

int PAL_MapTileIsBlocked(unsigned char x, unsigned char y, unsigned char h, PALMAP *lpMap);

unsigned char PAL_MapGetTileHeight(unsigned char x, unsigned char y, unsigned char h, unsigned char ucLayer, PALMAP *lpMap);

void PAL_MapBlitToSurface(PALMAP *lpMap, const VIDEO_Rect *lpSrcRect, unsigned char ucLayer);

#endif
