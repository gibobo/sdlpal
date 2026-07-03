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

#include <stdint.h>

#define PAL_AUDIO_CHANNEL_COUNT     (2U)
#if defined(ESP_PLATFORM)
// ESP32: 22050 Hz / 50 chunks per second reduces Nuked OPL3 CPU load by ~50%
// while keeping PAL_AUDIO_SAMPLES_PER_CHUNK = 441 (same buffer layout).
#define PAL_AUDIO_SAMPLING_RATE     (22050U)
#define PAL_AUDIO_CHUNK_PER_SECOND  (50U)
#else
#define PAL_AUDIO_SAMPLING_RATE     (44100U)
#define PAL_AUDIO_CHUNK_PER_SECOND  (100U)
#endif
#define PAL_AUDIO_BIT_DEPTH         (16U)
#define PAL_AUDIO_BYTES_PER_SAMPLE  (PAL_AUDIO_BIT_DEPTH >> 3)
#define PAL_AUDIO_SAMPLES_PER_CHUNK ((PAL_AUDIO_CHUNK_PER_SECOND + PAL_AUDIO_SAMPLING_RATE - 1) / PAL_AUDIO_CHUNK_PER_SECOND)

#if PAL_AUDIO_BIT_DEPTH == 8U
typedef uint8_t PAL_AUDIO_SAMPLE;
#define PAL_AUDIO_SAMPLE_MIN    (-128)
#define PAL_AUDIO_SAMPLE_MAX    (127)
#define PAL_AUDIO_SAMPLE_OFFSET (128)
#elif PAL_AUDIO_BIT_DEPTH == 16U
typedef int16_t PAL_AUDIO_SAMPLE;
#define PAL_AUDIO_SAMPLE_MIN    (-32768)
#define PAL_AUDIO_SAMPLE_MAX    (32767)
#define PAL_AUDIO_SAMPLE_OFFSET (0)
#else
#error Unsupported PAL_AUDIO_BIT_DEPTH
#endif
#define PAL_AUDIO_SAMPLE_SILENCE (PAL_AUDIO_SAMPLE_OFFSET)

static inline int PAL_AudioSampleToMixValue(PAL_AUDIO_SAMPLE sample)
{
#if PAL_AUDIO_BIT_DEPTH == 8U
    return ((int)sample - PAL_AUDIO_SAMPLE_OFFSET) << 8;
#else
    return (int)sample;
#endif
}

static inline PAL_AUDIO_SAMPLE PAL_AudioMixValueToSample(int value)
{
    if (value > 32767)
        value = 32767;
    else if (value < -32768)
        value = -32768;

#if PAL_AUDIO_BIT_DEPTH == 8U
    int32_t sample = value >> 8;
    if (sample > PAL_AUDIO_SAMPLE_MAX)
        sample = PAL_AUDIO_SAMPLE_MAX;
    else if (sample < PAL_AUDIO_SAMPLE_MIN)
        sample = PAL_AUDIO_SAMPLE_MIN;
    return (PAL_AUDIO_SAMPLE)(sample + PAL_AUDIO_SAMPLE_OFFSET);
#else
    return (PAL_AUDIO_SAMPLE)value;
#endif
}

#define AUDIOPLAYER_COMMONS                   \
    int32_t iMusic;                           \
    uint8_t fLoop;                            \
    void (*Shutdown)(void *);                 \
    int (*Play)(void *, int32_t, uint8_t, float); \
    void (*FillBuffer)(void *, uint8_t *, uint32_t)

typedef struct tagAUDIOPLAYER
{
    AUDIOPLAYER_COMMONS;
} AUDIOPLAYER;

/* Music backend selection. On ESP32 the default is to stream pre-rendered PCM
   (PCMMUS), which removes real-time OPL3 FM synthesis from the audio task; on
   other targets the live RIX/OPL3 player is used. Override with -DPAL_PRERENDERED_MUSIC=0/1. */
#ifndef PAL_PRERENDERED_MUSIC
#if defined(ESP_PLATFORM)
#define PAL_PRERENDERED_MUSIC 1
#else
#define PAL_PRERENDERED_MUSIC 0
#endif
#endif

int AUDIO_Startup(void);

void AUDIO_CloseDevice(void);

void AUDIO_FillBuffer(void *stream, uint32_t len);

void AUDIO_PlayMusic(int32_t iNumRIX, uint8_t fLoop, float flFadeTime);

void AUDIO_PlaySound(int iSoundNum);

void AUDIO_EnableMusic(uint8_t fEnable);
uint8_t AUDIO_MusicEnabled(void);

void AUDIO_EnableSound(uint8_t fEnable);
uint8_t AUDIO_SoundEnabled(void);

int AUDIO_GetCurrentMusic(void);

/* RIX */
AUDIOPLAYER *RIX_Init(void);

/* PCMMUS - streams pre-rendered PCM music from disk (see pcmmus.c). Returns NULL
   if no pre-rendered tracks exist for the current rate, so the caller can fall back to RIX. */
AUDIOPLAYER *PCMMUS_Init(void);

/* SOUND */
AUDIOPLAYER *SOUND_Init(void);

#endif
