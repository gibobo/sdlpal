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

#ifdef ARDUINO_ARCH_ESP32
#define PAL_AUDIO_OUTPUT_CHANNEL_COUNT (1U)
#define PAL_AUDIO_OUTPUT_SAMPLE_RATE   (22050U)
#define PAL_AUDIO_DEVICE_PERIOD_FRAMES (128U)
#else
#define PAL_AUDIO_OUTPUT_CHANNEL_COUNT (2U)
#define PAL_AUDIO_OUTPUT_SAMPLE_RATE   (44100U)
#define PAL_AUDIO_DEVICE_PERIOD_FRAMES (256U)
#endif
#define PAL_AUDIO_MIX_BUFFER_FRAMES (1U + ((PAL_AUDIO_OUTPUT_SAMPLE_RATE / PAL_AUDIO_CHUNK_PER_SECOND) / PAL_AUDIO_DEVICE_PERIOD_FRAMES)) * PAL_AUDIO_DEVICE_PERIOD_FRAMES

#define AUDIOPLAYER_COMMONS               \
    int iMusic;                           \
    unsigned char fLoop;                  \
    void (*Shutdown)(void *);             \
    int (*Play)(void *, int, int, float); \
    void (*FillBuffer)(void *, unsigned char *, unsigned int)

typedef struct tagAUDIOPLAYER
{
    AUDIOPLAYER_COMMONS;
} AUDIOPLAYER;

int AUDIO_Startup(void);

void AUDIO_CloseDevice(void);

void AUDIO_FillBuffer(void *stream, unsigned int len);

void AUDIO_PlayMusic(int iNumRIX, unsigned char fLoop, float flFadeTime);

void AUDIO_PlaySound(int iSoundNum);

void AUDIO_EnableMusic(int fEnable);
int AUDIO_MusicEnabled(void);

void AUDIO_EnableSound(int fEnable);
int AUDIO_SoundEnabled(void);

int AUDIO_GetCurrentMusic(void);

/* RIX */
AUDIOPLAYER *RIX_Init(void);

/* SOUND */
AUDIOPLAYER *SOUND_Init(void);

#endif
