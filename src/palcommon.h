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

#ifndef _PALUTILS_H
#define _PALUTILS_H

#include "video.h"
#include <stdint.h>

#define PAL_XY(x, y)            (uint32_t)(((((uint32_t)(y)) << 16) & 0xFFFF0000) | (((uint32_t)(x)) & 0xFFFF))
#define PAL_X(xy)               (short)((xy) & 0xFFFF)
#define PAL_Y(xy)               (short)(((xy) >> 16) & 0xFFFF)
#define PAL_XY_OFFSET(xy, x, y) (uint32_t)(((((int)(y) << 16) & 0xFFFF0000) + ((xy) & 0xFFFF0000)) | (((int)(x) & 0xFFFF) + ((xy) & 0xFFFF)))

int PAL_RLEBlitToSurface(
    const uint8_t *lpBitmapRLE,
    VIDEO_Surface *lpDstSurface,
    uint32_t pos);

int PAL_RLEBlitToSurfaceWithShadow(
    const uint8_t *lpBitmapRLE,
    VIDEO_Surface *lpDstSurface,
    uint32_t pos,
    int bShadow);

int PAL_RLEBlitWithColorShift(
    const uint8_t *lpBitmapRLE,
    VIDEO_Surface *lpDstSurface,
    uint32_t pos,
    int iColorShift);

int PAL_RLEBlitMonoColor(
    const uint8_t *lpBitmapRLE,
    VIDEO_Surface *lpDstSurface,
    uint32_t pos,
    uint8_t bColor,
    int iColorShift);

int PAL_FBPBlitToSurface(
    uint8_t *lpBitmapFBP,
    VIDEO_Surface *lpDstSurface);

uint16_t PAL_RLEGetWidth(
    const uint8_t *lpBitmapRLE);

uint16_t PAL_RLEGetHeight(
    const uint8_t *lpBitmapRLE);

uint16_t PAL_SpriteGetNumFrames(
    const uint8_t *lpSprite);

uint8_t *PAL_SpriteGetFrame(
    uint8_t *lpSprite,
    int iFrameNum);

uint32_t PAL_MKFGetChunkCount(void *fp);

uint32_t PAL_MKFGetChunkSize(
    uint32_t uiChunkNum,
    void *fp);

uint32_t PAL_MKFReadChunk(
    void *lpBuffer,
    uint32_t uiBufferSize,
    uint32_t uiChunkNum,
    void *fp);

uint32_t PAL_MKFDecompressChunk(
    uint8_t **lpBuffer,
    uint32_t uiBufferSize,
    uint32_t uiChunkNum,
    void *fp);

// From yj1.c:
uint8_t YJ2_Decompress(
    const void *Source,
    void *Destination);

#endif // _PALUTILS_H