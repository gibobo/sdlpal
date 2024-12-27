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

#include "common.h"
#include <SDL_render.h>

#define TOUCHOVERLAY_ALPHAMOD           120

#define VIDEO_CopySurface(s, sr, t, tr) SDL_BlitSurface((s), (sr), (t), (tr))
#define VIDEO_CopyEntireSurface(s, t)   SDL_BlitSurface((s), NULL, (t), NULL)
#define VIDEO_BackupScreen(s)           SDL_BlitSurface((s), NULL, gpScreenBak, NULL)
#define VIDEO_RestoreScreen(t)          SDL_BlitSurface(gpScreenBak, NULL, (t), NULL)
#define VIDEO_FreeSurface(s)            SDL_FreeSurface(s)

#ifdef __cplusplus
extern "C" {
#endif

extern SDL_Surface *gpScreen;
extern SDL_Surface *gpScreenBak;
extern volatile BOOL g_bRenderPaused;
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
   const SDL_Rect  *lpRect
);

void
VIDEO_SetPalette(
   SDL_Color        rgPalette[256]
);

void
VIDEO_Resize(
   int             w,
   int             h
);

SDL_Color *
VIDEO_GetPalette(
   void
);

void
VIDEO_ToggleFullscreen(
   void
);

void
VIDEO_SaveScreenshot(
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

SDL_Surface *
VIDEO_DuplicateSurface(
	SDL_Surface    *pSource,
	const SDL_Rect *pRect
);

SDL_Surface *
VIDEO_CreateCompatibleSurface(
	SDL_Surface    *pSource
);

SDL_Surface *
VIDEO_CreateCompatibleSizedSurface(
	SDL_Surface    *pSource,
	const SDL_Rect *pSize
);

void
VIDEO_UpdateSurfacePalette(
	SDL_Surface    *pSurface
);

void
VIDEO_DrawSurfaceToScreen(
    SDL_Surface    *pSurface
);

#ifdef __cplusplus
}
#endif

#endif
