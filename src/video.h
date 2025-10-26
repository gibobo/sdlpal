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

#ifndef VIDEO_H
#define VIDEO_H

#define SCREEN_W 320
#define SCREEN_H 200
#define SCREEN_SIZE (SCREEN_W * SCREEN_H)

typedef struct PAL_Surface {
    unsigned short w;       /**< Read-only */
    unsigned short h;       /**< Read-only */
    unsigned char *pixels;  /**< Read-write */
} PAL_Surface;

typedef struct PAL_Rect {
    int x;
    int y;
    int w;
    int h;
} PAL_Rect;

extern PAL_Surface *gpScreen;

int VIDEO_Startup(void);

void VIDEO_Shutdown(void);

void VIDEO_UpdateScreen(const PAL_Rect *lpRect);

void VIDEO_SetPalette(const unsigned char *rgPalette);

const unsigned char *VIDEO_GetPalette(void);

void VIDEO_ShakeScreen(
    unsigned short wShakeTime,
    unsigned short wShakeLevel);

void VIDEO_SwitchScreen(void);

void VIDEO_FadeScreen(unsigned short wSpeed);

PAL_Surface *VIDEO_DuplicateSurface(const PAL_Rect *pRect);

PAL_Surface *VIDEO_CreateCompatibleSizedSurface(const PAL_Rect *pSize);

void VIDEO_RenderPaused(unsigned char flag);

void VIDEO_CopySurface(
    PAL_Surface *src,
    const PAL_Rect *srcrect,
    PAL_Surface *dst,
    PAL_Rect *dstrect);

void VIDEO_CopyEntireSurface(
    PAL_Surface *src,
    PAL_Surface *dst);

void VIDEO_BackupScreen(PAL_Surface *src);

void VIDEO_RestoreScreen(PAL_Surface *dst);

void PAL_FreeSurface(PAL_Surface *surface);

void PAL_CleanScreen(void);

PAL_Surface *VIDEO_GetBackupSurface(unsigned char idx);

#endif
