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

#define PAL_AUDIO_CHUNK_PER_SECOND (70U)
#define PAL_AUDIO_BIT_DEPTH        (16U)
#define PAL_AUDIO_BYTES_PER_SAMPLE (PAL_AUDIO_BIT_DEPTH >> 3)
#define PAL_AUDIO_CHANNEL_NUM      (2U)
#define PAL_AUDIO_SAMPLE_RATE      (44100U)
#define PAL_AUDIO_BUFFER_SIZE      (1U + ((PAL_AUDIO_SAMPLE_RATE / PAL_AUDIO_CHUNK_PER_SECOND) / 128U)) * 128U

#define AUDIOPLAYER_COMMONS               \
    int iMusic;                           \
    int fLoop;                            \
    void (*Shutdown)(void *);             \
    int (*Play)(void *, int, int, float); \
    void (*FillBuffer)(void *, unsigned char *, unsigned int)

typedef struct tagAUDIOPLAYER
{
    AUDIOPLAYER_COMMONS;
} AUDIOPLAYER;

int AUDIO_OpenDevice(void);

void AUDIO_CloseDevice(void);

void AUDIO_FillBuffer(void *stream, unsigned int len);

void AUDIO_PlayMusic(int iNumRIX, int fLoop, float flFadeTime);

void AUDIO_PlaySound(int iSoundNum);

void AUDIO_EnableMusic(int fEnable);
int AUDIO_MusicEnabled(void);

void AUDIO_EnableSound(int fEnable);
int AUDIO_SoundEnabled(void);

/* RIX */
AUDIOPLAYER *RIX_Init(void);

/* SOUND */
AUDIOPLAYER *SOUND_Init(void);

#endif
