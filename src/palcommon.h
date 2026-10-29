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
#define PAL_XY_OFFSET(xy, x, y) PAL_XY(PAL_X(xy) + (int)(x), PAL_Y(xy) + (int)(y))

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
    const uint8_t *lpSprite,
    int iFrameNum);

// An open MKF archive with its chunk offset table kept in memory, so a chunk
// takes one seek and one read.
typedef struct tagPALMKF
{
    void *fp;              // the archive file, owned by this handle
    uint32_t uiChunkCount; // number of chunks
    uint32_t *rguiOffsets; // uiChunkCount + 1 chunk offsets from the archive header
} PALMKF;

void PAL_MKFOpen(
    PALMKF *lpMKF,
    void *fp);

void PAL_MKFClose(
    PALMKF *lpMKF);

uint32_t PAL_MKFGetChunkCount(
    const PALMKF *lpMKF);

uint32_t PAL_MKFGetChunkSize(
    uint32_t uiChunkNum,
    const PALMKF *lpMKF);

uint32_t PAL_MKFReadChunk(
    void *lpBuffer,
    uint32_t uiBufferSize,
    uint32_t uiChunkNum,
    const PALMKF *lpMKF);

uint32_t PAL_MKFPrepareBuffer(
    uint8_t **lpBuffer,
    uint32_t uiBufferSize,
    uint32_t uiSize,
    uint32_t uiChunkNum);

uint32_t PAL_MKFDecompressChunk(
    uint8_t **lpBuffer,
    uint32_t uiBufferSize,
    uint32_t uiChunkNum,
    const PALMKF *lpMKF);

// RNG archives are MKF archives whose chunks are MKF archives of frames.
int32_t PAL_RNGReadFrame(
    uint8_t **lpBuffer,
    uint32_t uiRngNum,
    uint32_t uiFrameNum,
    const PALMKF *lpRngMKF);

// From yj1.c: returns the decompressed length, or -1 if it exceeds DestSize.
int32_t YJ2_Decompress(
    const void *Source,
    void *Destination,
    uint32_t DestSize);

#endif // _PALUTILS_H