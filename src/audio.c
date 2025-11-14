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

#include "audio.h"
#include "driver.h"
#include "global.h"
#include "util.h"
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifndef PAL_FORCE_INLINE
#if defined(_MSC_VER)
#define PAL_FORCE_INLINE static __forceinline
#else
#define PAL_FORCE_INLINE __attribute__((always_inline)) static __inline__
#endif
#endif

#define PAL_MAX_VOLUME 100

typedef struct tagAUDIODEVICE
{
    AUDIOPLAYER *pMusPlayer;
    AUDIOPLAYER *pSoundPlayer;
    void *pSoundBuffer; /* The output buffer for sound */
    unsigned int id;
    int fMusicEnabled; /* Is BGM enabled? */
    int fSoundEnabled; /* Is sound effect enabled? */
    int fOpened;       /* Is the audio device opened? */
} AUDIODEVICE;

static AUDIODEVICE gAudioDevice;

PAL_FORCE_INLINE void AUDIO_MixNative(PAL_AUDIO_SAMPLE *dst, const PAL_AUDIO_SAMPLE *src, unsigned int samples)
{
    while (samples--)
    {
        int mixed = PAL_AudioSampleToMixValue(*dst) + PAL_AudioSampleToMixValue(*src++);
        *dst++ = PAL_AudioMixValueToSample(mixed);
    }
}

void AUDIO_FillBuffer(void *stream, unsigned int len)
{
    if (gAudioDevice.fOpened == false)
        return;

    // Play music
    if (gAudioDevice.fMusicEnabled && gAudioDevice.pMusPlayer)
    {
        gAudioDevice.pMusPlayer->FillBuffer(gAudioDevice.pMusPlayer, stream, len);
    }

    // Play sound
    if (gAudioDevice.fSoundEnabled && gAudioDevice.pSoundPlayer && gAudioDevice.pSoundBuffer)
    {
        // Prevent buffer overflow by limiting the size to the allocated buffer size
        unsigned int buffer_size = PAL_AUDIO_SAMPLES_PER_CHUNK * PAL_AUDIO_CHANNEL_COUNT * PAL_AUDIO_BYTES_PER_SAMPLE;
        unsigned int safe_len = min(len, buffer_size);
        memset(gAudioDevice.pSoundBuffer, PAL_AUDIO_SAMPLE_SILENCE, buffer_size);
        gAudioDevice.pSoundPlayer->FillBuffer(gAudioDevice.pSoundPlayer, gAudioDevice.pSoundBuffer, safe_len);

        // Mix sound & music
        AUDIO_MixNative((PAL_AUDIO_SAMPLE *)stream,
                        (const PAL_AUDIO_SAMPLE *)gAudioDevice.pSoundBuffer,
                        safe_len / PAL_AUDIO_BYTES_PER_SAMPLE);
    }
}

int AUDIO_Startup(void)
/*++
  Purpose:

    Initialize the audio subsystem.

  Parameters:

    None.

  Return value:

    0 - Success
   -1 - Already opened

--*/
{
    if (gAudioDevice.fOpened == true)
        return -1; // Already opened

    memset(&gAudioDevice, 0, sizeof(AUDIODEVICE));

    // Initialize the music subsystem.
    gAudioDevice.pMusPlayer = RIX_Init();
    if (gAudioDevice.pMusPlayer == NULL)
    {
        // Music initialization failed, but continue with sound only
        gAudioDevice.fMusicEnabled = false;
    }
    else
    {
        gAudioDevice.fMusicEnabled = true;
    }

    // Initialize the sound subsystem.
    gAudioDevice.pSoundPlayer = SOUND_Init();
    if (gAudioDevice.pSoundPlayer == NULL)
    {
        // Sound initialization failed
        gAudioDevice.fSoundEnabled = false;
    }
    else
    {
        // Allocate sound buffer
        gAudioDevice.pSoundBuffer = UTIL_calloc(PAL_AUDIO_SAMPLES_PER_CHUNK * PAL_AUDIO_CHANNEL_COUNT, PAL_AUDIO_BYTES_PER_SAMPLE);
        if (gAudioDevice.pSoundBuffer == NULL)
        {
            // Sound buffer allocation failed
            gAudioDevice.pSoundPlayer->Shutdown(gAudioDevice.pSoundPlayer);
            gAudioDevice.pSoundPlayer = NULL;
            gAudioDevice.fSoundEnabled = false;
        }
        else
        {
            gAudioDevice.fSoundEnabled = true;
        }
    }

    gAudioDevice.fOpened = true;
    return 0; // Success (at least one audio subsystem is working)
}

void AUDIO_CloseDevice(void)
/*++
  Purpose:

    Close the audio subsystem.

  Parameters:

    None.

  Return value:

    None.

--*/
{
    if (gAudioDevice.fOpened == false)
        return;

    if (gAudioDevice.pSoundPlayer != NULL)
        gAudioDevice.pSoundPlayer->Shutdown(gAudioDevice.pSoundPlayer);

    if (gAudioDevice.pMusPlayer)
        gAudioDevice.pMusPlayer->Shutdown(gAudioDevice.pMusPlayer);

    if (gAudioDevice.pSoundBuffer != NULL)
        UTIL_free(gAudioDevice.pSoundBuffer);

    memset(&gAudioDevice, 0, sizeof(AUDIODEVICE));
}

void AUDIO_PlaySound(int iSoundNum)
/*++
  Purpose:

    Play a sound in voc.mkf/sounds.mkf file.

  Parameters:

    [IN]  iSoundNum - number of the sound; the absolute value is used.

  Return value:

    None.

--*/
{
    // Unlike musics that use the 'load as required' strategy, sound player
    // load the entire sound file at once, which may cause about 0.5s or longer
    // latency for large sound files. To prevent this latency affects audio playing,
    // the mutex lock is obtained inside the SOUND_Play function rather than here.
    if (gAudioDevice.pSoundPlayer)
    {
        gAudioDevice.pSoundPlayer->Play(gAudioDevice.pSoundPlayer, abs(iSoundNum), false, 0.0f);
    }
}

void AUDIO_PlayMusic(int iNumRIX, unsigned char fLoop, float flFadeTime)
{
    if (gAudioDevice.pMusPlayer)
    {
        DRIVER_Audio_Lock();
        gAudioDevice.pMusPlayer->Play(gAudioDevice.pMusPlayer, iNumRIX, fLoop, flFadeTime);
        DRIVER_Audio_Unlock();
    }
}

void AUDIO_EnableMusic(int fEnable)
{
    gAudioDevice.fMusicEnabled = fEnable;
}

int AUDIO_MusicEnabled(void)
{
    return gAudioDevice.fMusicEnabled;
}

void AUDIO_EnableSound(int fEnable)
{
    gAudioDevice.fSoundEnabled = fEnable;
}

int AUDIO_SoundEnabled(void)
{
    return gAudioDevice.fSoundEnabled;
}

int AUDIO_GetCurrentMusic(void)
/*++
  Purpose:

    Get the current playing music number.

  Parameters:

    None.

  Return value:

    Current music number, or -1 if no music is playing.

--*/
{
    int result = -1;

    if (gAudioDevice.pMusPlayer)
    {
        DRIVER_Audio_Lock();
        result = gAudioDevice.pMusPlayer->iMusic;
        DRIVER_Audio_Unlock();
    }

    return result;
}
