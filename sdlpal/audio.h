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

#ifndef AUDIO_H
#define AUDIO_H

#include <SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

INT
AUDIO_OpenDevice(
   VOID
);

VOID
AUDIO_CloseDevice(
   VOID
);

SDL_AudioSpec*
AUDIO_GetDeviceSpec(
   VOID
);

VOID
AUDIO_IncreaseVolume(
   VOID
);

VOID
AUDIO_DecreaseVolume(
   VOID
);

VOID
AUDIO_PlayMusic(
   INT       iNumRIX,
   BOOL      fLoop,
   FLOAT     flFadeTime
);

VOID
AUDIO_PlaySound(
   INT    iSoundNum
);

VOID
AUDIO_EnableMusic(
   BOOL   fEnable
);

BOOL
AUDIO_MusicEnabled(
   VOID
);

VOID
AUDIO_EnableSound(
   BOOL   fEnable
);

BOOL
AUDIO_SoundEnabled(
   VOID
);

void
AUDIO_Lock(
	void
);

void
AUDIO_Unlock(
	void
);

#ifdef __cplusplus
}
#endif

#endif
