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

#define PAL_XY(x, y) (unsigned int)(((((unsigned short)(y)) << 16) & 0xFFFF0000) | (((unsigned short)(x)) & 0xFFFF))
#define PAL_X(xy) (short)((xy) & 0xFFFF)
#define PAL_Y(xy) (short)(((xy) >> 16) & 0xFFFF)
#define PAL_XY_OFFSET(xy, x, y) (unsigned int)(((((int)(y) << 16) & 0xFFFF0000) + ((xy) & 0xFFFF0000)) | (((int)(x) & 0xFFFF) + ((xy) & 0xFFFF)))

#ifdef __cplusplus
extern "C" {
#endif

int PAL_RLEBlitToSurface(
    const unsigned char *lpBitmapRLE,
    PAL_Surface *lpDstSurface,
    unsigned int pos);

int PAL_RLEBlitToSurfaceWithShadow(
    const unsigned char *lpBitmapRLE,
    PAL_Surface *lpDstSurface,
    unsigned int pos,
    int bShadow);

int PAL_RLEBlitWithColorShift(
    const unsigned char *lpBitmapRLE,
    PAL_Surface *lpDstSurface,
    unsigned int pos,
    int iColorShift);

int PAL_RLEBlitMonoColor(
    const unsigned char *lpBitmapRLE,
    PAL_Surface *lpDstSurface,
    unsigned int pos,
    unsigned char bColor,
    int iColorShift);

int PAL_FBPBlitToSurface(
    unsigned char *lpBitmapFBP,
    PAL_Surface *lpDstSurface);

int PAL_RLEGetWidth(
    const unsigned char *lpBitmapRLE);

int PAL_RLEGetHeight(
    const unsigned char *lpBitmapRLE);

unsigned short PAL_SpriteGetNumFrames(
    const unsigned char *lpSprite);

const unsigned char *PAL_SpriteGetFrame(
    const unsigned char *lpSprite,
    int iFrameNum);

int PAL_MKFGetChunkCount(void *fp);

int PAL_MKFGetChunkSize(
    unsigned int uiChunkNum,
    void *fp);

int PAL_MKFReadChunk(
    void *lpBuffer,
    unsigned int uiBufferSize,
    unsigned int uiChunkNum,
    void *fp);

int PAL_MKFGetDecompressedSize(
    unsigned int uiChunkNum,
    void *fp);

int PAL_MKFDecompressChunk(
    unsigned char **lpBuffer,
    unsigned int uiBufferSize,
    unsigned int uiChunkNum,
    void *fp);

// From yj1.c:
extern int (*Decompress)(
    const void *Source,
    void *Destination,
    int DestSize);

int YJ2_Decompress(
    const void *Source,
    void *Destination,
    int DestSize);

#ifdef __cplusplus
}
#endif

#endif // _PALUTILS_H
