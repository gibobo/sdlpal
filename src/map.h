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

#include <stdint.h>

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
    uint32_t *Tiles;
    uint32_t Len_Tiles;
    uint8_t *pTileSprite;
    uint32_t Len_TileSprite;
    int32_t iMapNum;
} PALMAP;

void PAL_LoadMap(PALMAP *lpMap, int32_t iMapNum);

void PAL_FreeMap(PALMAP *lpMap);

const uint8_t *PAL_MapGetTileBitmap(uint8_t x, uint8_t y, uint8_t h, uint8_t ucLayer, PALMAP *lpMap);

int PAL_MapTileIsBlocked(uint8_t x, uint8_t y, uint8_t h, PALMAP *lpMap);

uint8_t PAL_MapGetTileHeight(uint8_t x, uint8_t y, uint8_t h, uint8_t ucLayer, PALMAP *lpMap);

void PAL_MapBlitToSurface(PALMAP *lpMap, const VIDEO_Rect *lpSrcRect, uint8_t ucLayer);

#endif