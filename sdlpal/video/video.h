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

#ifndef VIDEO_H
#define VIDEO_H

#include <SDL_pixels.h>

#define VIDEO_CopySurface(s, sr, t, tr) PAL_UpperBlit((s), (sr), (t), (tr))
#define VIDEO_CopyEntireSurface(s, t)   PAL_UpperBlit((s), NULL, (t), NULL)
#define VIDEO_BackupScreen(s)           PAL_UpperBlit((s), NULL, gpScreenBak, NULL)
#define VIDEO_RestoreScreen(t)          PAL_UpperBlit(gpScreenBak, NULL, (t), NULL)

typedef struct PAL_Surface
{
    unsigned int flags;         /**< Read-only */
    SDL_PixelFormat *format;    /**< Read-only */
    int w, h;                   /**< Read-only */
    int pitch;                  /**< Read-only */
    void *pixels;               /**< Read-write */
} PAL_Surface;

typedef struct PAL_Rect {
  int x;
  int y;
  int w;
  int h;
} PAL_Rect;

typedef struct PAL_Color {
  unsigned char r;
  unsigned char g;
  unsigned char b;
  unsigned char a;
} PAL_Color;

#ifdef __cplusplus
extern "C" {
#endif

extern PAL_Surface *gpScreen;
extern PAL_Surface *gpScreenBak;

void Filter_StepParamSlot(int step);
void Filter_StepCurrentParam(int step);

int
VIDEO_Startup(
   void
);

void
VIDEO_Shutdown(
   void
);

void
VIDEO_UpdateScreen(
   const PAL_Rect *lpRect
);

void
VIDEO_SetPalette(
   PAL_Color       *rgPalette
);

void
VIDEO_Resize(
   int             w,
   int             h
);

PAL_Color *
VIDEO_GetPalette(
   void
);

void
VIDEO_ToggleFullscreen(
   void
);

void
VIDEO_ShakeScreen(
   unsigned short           wShakeTime,
   unsigned short           wShakeLevel
);

void
VIDEO_SwitchScreen(
   unsigned short           wSpeed
);

void
VIDEO_FadeScreen(
   unsigned short           wSpeed
);

void
VIDEO_SetWindowTitle(
	const char*   pszTitle
);

PAL_Surface *
VIDEO_DuplicateSurface(
	PAL_Surface    *pSource,
	const PAL_Rect *pRect
);

PAL_Surface *
VIDEO_CreateCompatibleSizedSurface(
	PAL_Surface    *pSource,
	const PAL_Rect *pSize
);

void
VIDEO_UpdateSurfacePalette(
	PAL_Surface    *pSurface
);

void
VIDEO_RenderPaused(
	unsigned char flag
);

int PAL_UpperBlit(
    PAL_Surface *src,
    const PAL_Rect *srcrect,
    PAL_Surface *dst,
    PAL_Rect *dstrect);

void PAL_FreeSurface(
    PAL_Surface *surface);

void PAL_CleanScreen(void);

#ifdef __cplusplus
}
#endif

#endif
