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

#include <SDL_audio.h>

#ifdef __cplusplus
extern "C" {
#endif

int AUDIO_OpenDevice(void);

void AUDIO_CloseDevice(void);

void AUDIO_PlayMusic(int iNumRIX, int fLoop, float flFadeTime);

void AUDIO_PlaySound(int iSoundNum);

void AUDIO_EnableMusic(int fEnable);
int AUDIO_MusicEnabled(void);

void AUDIO_EnableSound(int fEnable);
int AUDIO_SoundEnabled(void);

SDL_AudioSpec *
AUDIO_GetDeviceSpec(
   void
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
