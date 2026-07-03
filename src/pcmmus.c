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
// pcmmus.c - Pre-rendered PCM music player.
//
// An AUDIOPLAYER backend that streams pre-rendered raw PCM music from disk
// instead of synthesizing OPL3 FM in real time. Used on memory/CPU-constrained
// targets (ESP32): the expensive Nuked-OPL3 synthesis is done once, offline, on
// the desktop build (see the palmusrender tool, tools/musrender.c) and the result is played
// back with a cheap file read + copy.
//
// Files live at:  <RESOURCE_PATH>/mus/<iMusic>_<PAL_AUDIO_SAMPLING_RATE>.pcm
// Format: raw interleaved stereo signed-16-bit little-endian samples at the
// current output rate (no header). This matches the engine's mixing buffer
// layout exactly, so FillBuffer is a direct read into the output stream.
//

#include "pcmmus.h"
#include "audio.h"
#include "util.h"
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define PCMMUS_PATH_MAX 256

typedef enum tagPCM_FadeType
{
    PCM_FADE_NONE = 0,
    PCM_FADE_IN,
    PCM_FADE_OUT
} PCM_FadeType;

typedef struct tagPCMMUSPLAYER
{
    AUDIOPLAYER_COMMONS;
    void *fp;                     /* handle of the currently open PCM file (NULL = none) */
    int32_t data_offset;          /* byte offset of PCM data in fp (past the header) */
    int32_t iNextMusic;           /* pending track to switch to after fade-out (-1 = none) */
    uint8_t fNextLoop;            /* loop flag for the pending track */
    PCM_FadeType FadeType;        /* current fade state */
    int32_t iTotalFadeSamples;    /* ramp length in int16 samples (interleaved), per half-fade */
    int32_t iRemainingFadeSamples;
    int32_t fReady;
} PCMMUSPLAYER;

/* Build the on-disk path of a track for the current output sample rate. */
static void PCMMUS_BuildPath(char *dst, size_t cap, int32_t iMusic)
{
    snprintf(dst, cap, "%s/mus/%d_%u.pcm", RESOURCE_PATH, (int)iMusic, (unsigned)PAL_AUDIO_SAMPLING_RATE);
}

/* Write the 12-byte little-endian PCM header. Returns 0 on success. */
int PCMMUS_WriteHeader(void *fp, uint32_t sample_rate, uint16_t channels)
{
    uint8_t h[PCMMUS_HEADER_SIZE];
    h[0] = 'P'; h[1] = 'C'; h[2] = 'M'; h[3] = '1';
    h[4] = (uint8_t)(PCMMUS_VERSION & 0xFF);
    h[5] = (uint8_t)((PCMMUS_VERSION >> 8) & 0xFF);
    h[6] = (uint8_t)(channels & 0xFF);
    h[7] = (uint8_t)((channels >> 8) & 0xFF);
    h[8] = (uint8_t)(sample_rate & 0xFF);
    h[9] = (uint8_t)((sample_rate >> 8) & 0xFF);
    h[10] = (uint8_t)((sample_rate >> 16) & 0xFF);
    h[11] = (uint8_t)((sample_rate >> 24) & 0xFF);
    return (UTIL_fwrite(h, 1, PCMMUS_HEADER_SIZE, fp) == PCMMUS_HEADER_SIZE) ? 0 : -1;
}

/* Read+validate the header at the current position. Returns the data offset
   (PCMMUS_HEADER_SIZE) and optionally the declared rate; for a legacy headerless
   file it rewinds to 0 and returns 0 so the whole file is treated as PCM data. */
static int32_t PCMMUS_ReadHeader(void *fp, uint32_t *out_rate)
{
    uint8_t h[PCMMUS_HEADER_SIZE];
    if (UTIL_fread(h, 1, PCMMUS_HEADER_SIZE, fp) != PCMMUS_HEADER_SIZE ||
        h[0] != 'P' || h[1] != 'C' || h[2] != 'M' || h[3] != '1')
    {
        UTIL_fseek(fp, 0, SEEK_SET);
        return 0;
    }
    if (out_rate != NULL)
        *out_rate = (uint32_t)h[8] | ((uint32_t)h[9] << 8) | ((uint32_t)h[10] << 16) | ((uint32_t)h[11] << 24);
    return PCMMUS_HEADER_SIZE;
}

/* Open a track's PCM file for streaming, or NULL if it does not exist. On
   success *data_offset receives the byte offset where PCM samples begin. */
static void *PCMMUS_OpenTrack(int32_t iMusic, int32_t *data_offset)
{
    char path[PCMMUS_PATH_MAX];
    void *fp;
    PCMMUS_BuildPath(path, sizeof(path), iMusic);
    /* Use the non-checking variant: a missing track is expected (silence), not an error. */
    fp = UTIL_fopen_without_checking(path, "rb");
    if (fp != NULL)
    {
        int32_t off = PCMMUS_ReadHeader(fp, NULL);
        if (data_offset != NULL)
            *data_offset = off;
    }
    return fp;
}

/* Apply a linear fade ramp to `samples` int16 values in place, advancing the
   remaining-fade counter. FADE_IN ramps gain 0->1, FADE_OUT ramps 1->0. */
static void PCMMUS_ApplyRamp(PCMMUSPLAYER *p, PAL_AUDIO_SAMPLE *buf, uint32_t samples)
{
    uint32_t i;
    if (p->iTotalFadeSamples <= 0)
        return;
    for (i = 0; i < samples; i++)
    {
        int32_t rem = p->iRemainingFadeSamples - (int32_t)i;
        int32_t gain_num;
        if (rem < 0)
            rem = 0;
        gain_num = (p->FadeType == PCM_FADE_OUT) ? rem : (p->iTotalFadeSamples - rem);
        if (gain_num < 0)
            gain_num = 0;
        if (gain_num > p->iTotalFadeSamples)
            gain_num = p->iTotalFadeSamples;
        buf[i] = PAL_AudioMixValueToSample(((int32_t)buf[i] * gain_num) / p->iTotalFadeSamples);
    }
    p->iRemainingFadeSamples -= (int32_t)samples;
    if (p->iRemainingFadeSamples < 0)
        p->iRemainingFadeSamples = 0;
}

static void
PCMMUS_FillBuffer(
    void *object,
    uint8_t *stream,
    uint32_t len)
{
    PCMMUSPLAYER *p = (PCMMUSPLAYER *)object;
    PAL_AUDIO_SAMPLE *out = (PAL_AUDIO_SAMPLE *)stream;
    uint32_t remaining = len / PAL_AUDIO_BYTES_PER_SAMPLE; /* int16 samples still to produce */

    if (p == NULL || !p->fReady)
    {
        memset(stream, PAL_AUDIO_SAMPLE_SILENCE, len);
        return;
    }

    while (remaining > 0)
    {
        /* Complete a pending fade-out: close current, then switch to the next track. */
        if (p->FadeType == PCM_FADE_OUT && p->iRemainingFadeSamples <= 0)
        {
            if (p->fp != NULL) { UTIL_fclose(p->fp); p->fp = NULL; }
            if (p->iNextMusic > 0)
            {
                p->iMusic = p->iNextMusic;
                p->iNextMusic = -1;
                p->fLoop = p->fNextLoop;
                p->fp = PCMMUS_OpenTrack(p->iMusic, &p->data_offset);
                if (p->fp == NULL)
                {
                    p->iMusic = -1;
                    p->FadeType = PCM_FADE_NONE;
                }
                else if (p->iTotalFadeSamples > 0)
                {
                    p->FadeType = PCM_FADE_IN;
                    p->iRemainingFadeSamples = p->iTotalFadeSamples;
                }
                else
                {
                    p->FadeType = PCM_FADE_NONE;
                }
            }
            else
            {
                p->iMusic = -1;
                p->FadeType = PCM_FADE_NONE;
            }
            continue;
        }

        /* Nothing to play: fill the rest with silence and stop. */
        if (p->iMusic <= 0 || p->fp == NULL)
        {
            memset(out, PAL_AUDIO_SAMPLE_SILENCE, remaining * PAL_AUDIO_BYTES_PER_SAMPLE);
            return;
        }

        /* When fading, stop the read at the fade boundary so the state transition
           (handled at the top of the loop) lands exactly on a buffer edge. */
        uint32_t want = remaining;
        if ((p->FadeType == PCM_FADE_IN || p->FadeType == PCM_FADE_OUT) &&
            p->iRemainingFadeSamples > 0 && (uint32_t)p->iRemainingFadeSamples < want)
        {
            want = (uint32_t)p->iRemainingFadeSamples;
        }

        /* Stream raw PCM directly into the output (int16 LE == in-memory layout). */
        uint32_t got = UTIL_fread(out, PAL_AUDIO_BYTES_PER_SAMPLE, want, p->fp);
        if (got < want)
        {
            if (p->fLoop)
            {
                /* Seamless loop: rewind to the PCM data (past the header) and read the remainder. */
                UTIL_fseek(p->fp, p->data_offset, SEEK_SET);
                got += UTIL_fread(out + got, PAL_AUDIO_BYTES_PER_SAMPLE, want - got, p->fp);
                if (got < want)
                    memset(out + got, PAL_AUDIO_SAMPLE_SILENCE, (want - got) * PAL_AUDIO_BYTES_PER_SAMPLE);
            }
            else
            {
                /* End of a non-looping track: silence the tail and stop. */
                memset(out + got, PAL_AUDIO_SAMPLE_SILENCE, (want - got) * PAL_AUDIO_BYTES_PER_SAMPLE);
                if (p->FadeType != PCM_FADE_OUT)
                {
                    UTIL_fclose(p->fp);
                    p->fp = NULL;
                    p->iMusic = -1;
                }
            }
        }

        /* Apply the fade ramp (no-op when not fading). */
        if (p->FadeType == PCM_FADE_IN || p->FadeType == PCM_FADE_OUT)
        {
            PCMMUS_ApplyRamp(p, out, want);
            if (p->FadeType == PCM_FADE_IN && p->iRemainingFadeSamples <= 0)
                p->FadeType = PCM_FADE_NONE;
        }

        out += want;
        remaining -= want;
    }
}

static int
PCMMUS_Play(
    void *object,
    int32_t iNumRIX,
    uint8_t fLoop,
    float flFadeTime)
{
    PCMMUSPLAYER *p = (PCMMUSPLAYER *)object;
    int32_t fadeSamples;

    if (p == NULL)
        return false;

    p->fReady = true;

    /* Same track already playing with nothing pending: just update the loop flag. */
    if (iNumRIX == p->iMusic && p->iNextMusic == -1)
    {
        p->fLoop = fLoop;
        return true;
    }

    /* Half-fade length (RIX uses fade/2 for fade-out and fade-in each). */
    fadeSamples = (int32_t)round(flFadeTime / 2.0f * PAL_AUDIO_SAMPLING_RATE) * PAL_AUDIO_CHANNEL_COUNT;
    p->iTotalFadeSamples = fadeSamples;
    p->iNextMusic = iNumRIX;
    p->fNextLoop = fLoop;

    if (p->iMusic > 0 && p->fp != NULL && fadeSamples > 0)
    {
        /* Fade the current track out; the switch happens in FillBuffer. */
        p->FadeType = PCM_FADE_OUT;
        p->iRemainingFadeSamples = fadeSamples;
    }
    else
    {
        /* No current track (or no fade): switch immediately. */
        if (p->fp != NULL) { UTIL_fclose(p->fp); p->fp = NULL; }
        p->iNextMusic = -1;
        if (iNumRIX > 0)
        {
            p->iMusic = iNumRIX;
            p->fLoop = fLoop;
            p->fp = PCMMUS_OpenTrack(iNumRIX, &p->data_offset);
            if (p->fp == NULL)
            {
                p->iMusic = -1;
                p->FadeType = PCM_FADE_NONE;
            }
            else if (fadeSamples > 0)
            {
                p->FadeType = PCM_FADE_IN;
                p->iRemainingFadeSamples = fadeSamples;
            }
            else
            {
                p->FadeType = PCM_FADE_NONE;
            }
        }
        else
        {
            p->iMusic = -1;
            p->FadeType = PCM_FADE_NONE;
        }
    }

    return true;
}

static void PCMMUS_Shutdown(void *object)
{
    if (object != NULL)
    {
        PCMMUSPLAYER *p = (PCMMUSPLAYER *)object;
        p->fReady = false;
        if (p->fp != NULL)
            UTIL_fclose(p->fp);
        UTIL_free(p);
    }
}

AUDIOPLAYER *PCMMUS_Init(void)
/*++
  Purpose:

    Initialize the pre-rendered PCM music player. Returns NULL if no pre-rendered
    PCM tracks exist for the current sample rate, so the caller can fall back to
    the live RIX/OPL3 player.

--*/
{
    /* Probe a few common tracks (1 = first BGM, 5 = title, 0 = menu). If none are
       present, this target has no pre-rendered music -> let the caller use RIX. */
    static const int32_t probe[] = {1, 5, 0, 2, 3};
    PCMMUSPLAYER *p;
    void *test = NULL;
    size_t i;

    for (i = 0; i < sizeof(probe) / sizeof(probe[0]); i++)
    {
        test = PCMMUS_OpenTrack(probe[i], NULL);
        if (test != NULL)
            break;
    }
    if (test == NULL)
        return NULL;
    UTIL_fclose(test);

    p = (PCMMUSPLAYER *)UTIL_malloc(sizeof(PCMMUSPLAYER));
    p->FillBuffer = PCMMUS_FillBuffer;
    p->Shutdown = PCMMUS_Shutdown;
    p->Play = PCMMUS_Play;
    p->iMusic = -1;
    p->iNextMusic = -1;
    p->fLoop = false;
    p->fNextLoop = false;
    p->fp = NULL;
    p->data_offset = 0;
    p->FadeType = PCM_FADE_NONE;
    p->iTotalFadeSamples = 0;
    p->iRemainingFadeSamples = 0;
    p->fReady = false;

    return (AUDIOPLAYER *)p;
}
