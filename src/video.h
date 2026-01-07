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

#include <stdint.h>

#define SCREEN_W    320
#define SCREEN_H    200
#define SCREEN_SIZE (SCREEN_W * SCREEN_H)

typedef struct VIDEO_Surface
{
    uint16_t w;      /**< Read-only */
    uint16_t h;      /**< Read-only */
    uint8_t *pixels; /**< Read-write */
} VIDEO_Surface;

typedef struct VIDEO_Rect
{
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
} VIDEO_Rect;

extern VIDEO_Surface *gpScreen;

int VIDEO_Startup(void);

void VIDEO_Shutdown(void);

void VIDEO_UpdateScreen(const VIDEO_Rect *lpRect);

void VIDEO_ShakeScreen(
    uint16_t wShakeTime,
    uint16_t wShakeLevel);

void VIDEO_SwitchScreen(void);

void VIDEO_FadeScreen(uint16_t wSpeed);

VIDEO_Surface *VIDEO_DuplicateSurface(const VIDEO_Rect *pRect);

VIDEO_Surface *VIDEO_CreateCompatibleSizedSurface(const VIDEO_Rect *pSize);

void VIDEO_RenderPaused(uint8_t flag);

void VIDEO_CopySurface(
    VIDEO_Surface *src,
    const VIDEO_Rect *srcrect,
    VIDEO_Surface *dst,
    VIDEO_Rect *dstrect);

void VIDEO_CopyEntireSurface(
    VIDEO_Surface *src,
    VIDEO_Surface *dst);

void VIDEO_BackupScreen(VIDEO_Surface *src);

void VIDEO_RestoreScreen(VIDEO_Surface *dst);

void VIDEO_FreeSurface(VIDEO_Surface *surface);

void VIDEO_CleanScreen(void);

VIDEO_Surface *VIDEO_GetBackupSurface(uint8_t idx);

#endif