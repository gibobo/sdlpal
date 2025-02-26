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

#include "opl.h"
#include "rix.h"
#include "src/audio.h"
#include "src/util.h"
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define PAL_MAX_SAMPLERATE 49716
#define PAL_MIX_MAXVOLUME 128
#define AUDIO_CHUNK_PER_SECOND 70

enum {
  FADE_NONE,
  FADE_IN,
  FADE_OUT
} FadeType; // fade in or fade out ?

typedef struct tagRIXPLAYER {
  AUDIOPLAYER_COMMONS;
  unsigned char *buf;
  unsigned char *pos;
  int iNextMusic; // the next music number to switch to
  unsigned int dwStartFadeTime;
  int iTotalFadeOutSamples;
  int iTotalFadeInSamples;
  int iRemainingFadeSamples;
  unsigned char FadeType;
  int fNextLoop;
  int fReady;
} RIXPLAYER;

static void
RIX_FillBuffer(
    void *object,
    unsigned char *stream,
    int len)
/*++
    Purpose:

    Fill the background music into the sound buffer. Called by the SDL sound
    callback function only (audio.c: AUDIO_FillBuffer).

    Parameters:

    [OUT] stream - pointer to the stream buffer.

    [IN]  len - Length of the buffer.

    Return value:

    None.

--*/
{
    RIXPLAYER *pRixPlayer = (RIXPLAYER *)object;

    if (pRixPlayer == NULL || !pRixPlayer->fReady)
    {
        // Not initialized
        return;
    }

    while (len > 0)
    {
        int volume, delta_samples = 0, vol_delta = 0;
        // fading in or fading out
        switch (pRixPlayer->FadeType)
        {
        case FADE_IN:
            if (pRixPlayer->iRemainingFadeSamples <= 0)
            {
                pRixPlayer->FadeType = FADE_NONE;
                volume = PAL_MIX_MAXVOLUME;
            }
            else
            {
                volume = (int)(PAL_MIX_MAXVOLUME * (1.0 - (double)pRixPlayer->iRemainingFadeSamples / pRixPlayer->iTotalFadeInSamples));
                delta_samples = (pRixPlayer->iTotalFadeInSamples / PAL_MIX_MAXVOLUME) & ~(PAL_AUDIO_CHANNEL_NUM - 1);
                vol_delta = 1;
            }
            break;
        case FADE_OUT:
            if (pRixPlayer->iTotalFadeOutSamples == pRixPlayer->iRemainingFadeSamples && pRixPlayer->iTotalFadeOutSamples > 0)
            {
                unsigned int now = UTIL_GetTicks();
                int passed_samples = (now > pRixPlayer->dwStartFadeTime) ? (int)((now - pRixPlayer->dwStartFadeTime) * PAL_AUDIO_SAMPLE_RATE / 1000) : 0;
                pRixPlayer->iRemainingFadeSamples -= passed_samples;
            }
            if (pRixPlayer->iMusic == -1 || pRixPlayer->iRemainingFadeSamples <= 0)
            {
                // There is no current playing music, or fading time has passed.
                // Start playing the next one or stop playing.
                if (pRixPlayer->iNextMusic > 0)
                {
                    pRixPlayer->iMusic = pRixPlayer->iNextMusic;
                    pRixPlayer->iNextMusic = -1;
                    pRixPlayer->fLoop = pRixPlayer->fNextLoop;
                    pRixPlayer->FadeType = FADE_IN;
                    if (pRixPlayer->iMusic > 0)
                        pRixPlayer->dwStartFadeTime += pRixPlayer->iTotalFadeOutSamples * 1000 / PAL_AUDIO_SAMPLE_RATE;
                    else
                        pRixPlayer->dwStartFadeTime = UTIL_GetTicks();
                    pRixPlayer->iTotalFadeOutSamples = 0;
                    pRixPlayer->iRemainingFadeSamples = pRixPlayer->iTotalFadeInSamples;
                    CrixPlayer_rewind(pRixPlayer->iMusic, true);
                    
                    continue;
                }
                else
                {
                    pRixPlayer->iMusic = -1;
                    pRixPlayer->FadeType = FADE_NONE;
                    return;
                }
            }
            else
            {
                volume = (int)(PAL_MIX_MAXVOLUME * ((double)pRixPlayer->iRemainingFadeSamples / pRixPlayer->iTotalFadeOutSamples));
                delta_samples = (pRixPlayer->iTotalFadeOutSamples / PAL_MIX_MAXVOLUME) & ~(PAL_AUDIO_CHANNEL_NUM - 1);
                vol_delta = -1;
            }
            break;
        default:
            if (pRixPlayer->iMusic <= 0)
                return; // No current playing music
            else
                volume = PAL_MIX_MAXVOLUME;
        }

        //
        // Fill the buffer with sound data
        //
        int buf_max_len = PAL_AUDIO_SAMPLE_RATE / AUDIO_CHUNK_PER_SECOND * PAL_AUDIO_CHANNEL_NUM * sizeof(short);
        char fContinue = true;
        while (len > 0 && fContinue)
        {
            if (pRixPlayer->pos == NULL || pRixPlayer->pos - pRixPlayer->buf >= buf_max_len)
            {
                pRixPlayer->pos = pRixPlayer->buf;
                if (!CrixPlayer_update())
                {
                    if (!pRixPlayer->fLoop)
                    {
                        //
                        // Not loop, simply terminate the music
                        //
                        pRixPlayer->iMusic = -1;
                        if (pRixPlayer->FadeType != FADE_OUT && pRixPlayer->iNextMusic == -1)
                        {
                            pRixPlayer->FadeType = FADE_NONE;
                        }
                        return;
                    }
                    CrixPlayer_rewind(pRixPlayer->iMusic, false);
                    if (!CrixPlayer_update())
                    {
                        //
                        // Something must be wrong
                        //
                        pRixPlayer->iMusic = -1;
                        pRixPlayer->FadeType = FADE_NONE;
                        return;
                    }
                }
                
                Copl_update((short *)pRixPlayer->buf, PAL_AUDIO_SAMPLE_RATE / AUDIO_CHUNK_PER_SECOND);
            }

            int l = buf_max_len - (int)(pRixPlayer->pos - pRixPlayer->buf);
            l = (l > len) ? len / sizeof(short) : l / sizeof(short);

            // Put audio data into buffer and adjust volume
            if (pRixPlayer->FadeType != FADE_NONE)
            {
                short *ptr = (short *)stream;
                for (int i = 0; i < l && pRixPlayer->iRemainingFadeSamples > 0; volume += vol_delta)
                {
                    int j = 0;
                    for (j = 0; i < l && j < delta_samples; i++, j++)
                    {
                        *ptr++ = *(short *)pRixPlayer->pos * volume / PAL_MIX_MAXVOLUME;
                        pRixPlayer->pos += sizeof(short);
                    }
                    pRixPlayer->iRemainingFadeSamples -= j;
                }
                fContinue = (pRixPlayer->iRemainingFadeSamples > 0);
                len -= (int)((unsigned char *)ptr - stream);
                stream = (unsigned char *)ptr;
            }
            else
            {
                memcpy(stream, pRixPlayer->pos, l * sizeof(short));
                pRixPlayer->pos += l * sizeof(short);
                stream += l * sizeof(short);
                len -= l * sizeof(short);
            }
        }
    }
}

static void RIX_Shutdown(void *object)
/*++
    Purpose:

    Shutdown the RIX player subsystem.

    Parameters:

    None.

    Return value:

    None.

--*/
{
    if (object != NULL) {
        RIXPLAYER *pRixPlayer = (RIXPLAYER *)object;
        pRixPlayer->fReady = false;
        UTIL_free(pRixPlayer->buf);
        CrixPlayer_deinit();
        Copl_deinit();
        UTIL_free(pRixPlayer);
    }
}

static int
RIX_Play(
    void *object,
    int iNumRIX,
    int fLoop,
    float flFadeTime)
/*++
    Purpose:

    Start playing the specified music.

    Parameters:

    [IN]  iNumRIX - number of the music. 0 to stop playing current music.

    [IN]  fLoop - Whether the music should be looped or not.

    [IN]  flFadeTime - the fade in/out time when switching music.

    Return value:

    None.

--*/
{
    RIXPLAYER *pRixPlayer = (RIXPLAYER *)object;

    //
    // Check for NULL pointer.
    //
    if (pRixPlayer == NULL)
    {
        return false;
    }

    if (iNumRIX == pRixPlayer->iMusic && pRixPlayer->iNextMusic == -1)
    {
        /* Will play the same music without any pending play changes,
           just change the loop attribute */
        pRixPlayer->fLoop = fLoop;
        return true;
    }

    if (pRixPlayer->FadeType != FADE_OUT)
    {
        if (pRixPlayer->FadeType == FADE_IN && pRixPlayer->iTotalFadeInSamples > 0 && pRixPlayer->iRemainingFadeSamples > 0)
        {
            pRixPlayer->dwStartFadeTime = UTIL_GetTicks() - (int)((float)pRixPlayer->iRemainingFadeSamples / pRixPlayer->iTotalFadeInSamples * flFadeTime * (1000 / 2));
        }
        else
        {
            pRixPlayer->dwStartFadeTime = UTIL_GetTicks();
        }
        pRixPlayer->iTotalFadeOutSamples = (int)round(flFadeTime / 2.0f * PAL_AUDIO_SAMPLE_RATE) * PAL_AUDIO_CHANNEL_NUM;
        pRixPlayer->iRemainingFadeSamples = pRixPlayer->iTotalFadeOutSamples;
        pRixPlayer->iTotalFadeInSamples = pRixPlayer->iTotalFadeOutSamples;
    }
    else
    {
        pRixPlayer->iTotalFadeInSamples = (int)round(flFadeTime / 2.0f * PAL_AUDIO_SAMPLE_RATE) * PAL_AUDIO_CHANNEL_NUM;
    }

    pRixPlayer->iNextMusic = iNumRIX;
    pRixPlayer->FadeType = FADE_OUT;
    pRixPlayer->fNextLoop = fLoop;
    pRixPlayer->fReady = true;

    return true;
}

AUDIOPLAYER *RIX_Init(void)
/*++
  Purpose:

    Initialize the RIX player subsystem.

  Parameters:

    [IN]  szFileName - Filename of the mus.mkf file.

  Return value:

    0 if success, -1 if cannot allocate memory, -2 if file not found.
--*/
{
    RIXPLAYER *pRixPlayer;
    pRixPlayer = (RIXPLAYER *)UTIL_malloc(sizeof(RIXPLAYER));
    pRixPlayer->FillBuffer = RIX_FillBuffer;
    pRixPlayer->Shutdown = RIX_Shutdown;
    pRixPlayer->Play = RIX_Play;
    pRixPlayer->buf = (unsigned char *)UTIL_calloc((PAL_MAX_SAMPLERATE + AUDIO_CHUNK_PER_SECOND - 1) / AUDIO_CHUNK_PER_SECOND * PAL_AUDIO_CHANNEL_NUM, sizeof(short));

    Copl_init(PAL_AUDIO_SAMPLE_RATE, PAL_AUDIO_CHANNEL_NUM == 2);

    // Load the MKF file.
    if (!CrixPlayer_load(RESOURCE_PATH "/mus.mkf")) {
      UTIL_free(pRixPlayer);
      pRixPlayer = NULL;
      return NULL;
    }

    // Success.
    pRixPlayer->FadeType = FADE_NONE;
    pRixPlayer->iMusic = pRixPlayer->iNextMusic = -1;
    pRixPlayer->pos = NULL;
    pRixPlayer->fLoop = false;
    pRixPlayer->fNextLoop = false;
    pRixPlayer->fReady = false;

    return (AUDIOPLAYER *)pRixPlayer;
}
