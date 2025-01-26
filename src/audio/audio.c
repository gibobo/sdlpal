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
#include "audio_internal.h"
#include "common.h"
#include "global.h"
#include "players.h"
#include "resampler.h"
#include "util.h"
#include <SDL_audio.h>

#define     PAL_MAX_VOLUME               100

typedef struct tagAUDIODEVICE {
    AUDIOPLAYER *pMusPlayer;
    AUDIOPLAYER *pSoundPlayer;
    void *pSoundBuffer; /* The output buffer for sound */
    unsigned int id;
    int fMusicEnabled; /* Is BGM enabled? */
    int fSoundEnabled; /* Is sound effect enabled? */
    int fOpened;       /* Is the audio device opened? */
} AUDIODEVICE;

static AUDIODEVICE gAudioDevice;

PAL_FORCE_INLINE void AUDIO_MixNative(short *dst, short *src, int samples) {
    while (samples > 0) {
        int val = *src++ + *dst;
        if (val > SHRT_MAX)
            *dst++ = SHRT_MAX;
        else if (val < SHRT_MIN)
            *dst++ = SHRT_MIN;
        else
            *dst++ = (short)val;
        samples--;
    }
}

static void SDLCALL AUDIO_FillBuffer(void *udata, unsigned char *stream, int len)
/*++
  Purpose:

    SDL sound callback function.

  Parameters:

    [IN]  udata - pointer to user-defined parameters (Not used).

    [OUT] stream - pointer to the stream buffer.

    [IN]  len - Length of the buffer.

  Return value:

    None.

--*/
{
    memset(stream, 0, len);

    // Play music
    if (gAudioDevice.fMusicEnabled && gAudioDevice.pMusPlayer) {
        gAudioDevice.pMusPlayer->FillBuffer(gAudioDevice.pMusPlayer, stream, len);
    }

    // Play sound
    if (gAudioDevice.fSoundEnabled && gAudioDevice.pSoundPlayer) {
        memset(gAudioDevice.pSoundBuffer, 0, len);
        gAudioDevice.pSoundPlayer->FillBuffer(gAudioDevice.pSoundPlayer, gAudioDevice.pSoundBuffer, len);

        // Mix sound & music
        AUDIO_MixNative((short *)stream, gAudioDevice.pSoundBuffer, len >> 1);
    }
}

int AUDIO_OpenDevice(void)
/*++
  Purpose:

    Initialize the audio subsystem.

  Parameters:

    None.

  Return value:

    0 if succeed, others if failed.

--*/
{
    SDL_AudioSpec spec;

    if (gAudioDevice.fOpened) {
        // Already opened
        return -1;
    }

    gAudioDevice.fOpened = FALSE;
    gAudioDevice.fMusicEnabled = TRUE;
    gAudioDevice.fSoundEnabled = TRUE;

    // Initialize the resampler module
    resampler_init();

    // Open the audio device.
    spec.freq = gConfig.iSampleRate;
    spec.format = AUDIO_S16SYS;
    spec.channels = gConfig.iAudioChannels;
    spec.samples = gConfig.wAudioBufferSize;
    spec.callback = AUDIO_FillBuffer;
    gAudioDevice.id = SDL_OpenAudioDevice(NULL, 0, &spec, NULL, 0);

    if (gAudioDevice.id == 0)
        return -3; // Failed

    gAudioDevice.pSoundBuffer = UTIL_calloc(gConfig.wAudioBufferSize * gConfig.iAudioChannels, sizeof(short));

    gAudioDevice.fOpened = TRUE;

    // Initialize the sound subsystem.
    gAudioDevice.pSoundPlayer = SOUND_Init();

    // Initialize the music subsystem.
    gAudioDevice.pMusPlayer = RIX_Init();

    // Let the callback function run so that musics will be played.
    SDL_PauseAudioDevice(gAudioDevice.id, 0);

    return 0;
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
    SDL_CloseAudioDevice(gAudioDevice.id);

    if (gAudioDevice.pSoundPlayer != NULL) {
        gAudioDevice.pSoundPlayer->Shutdown(gAudioDevice.pSoundPlayer);
        gAudioDevice.pSoundPlayer = NULL;
    }

    if (gAudioDevice.pMusPlayer) {
        gAudioDevice.pMusPlayer->Shutdown(gAudioDevice.pMusPlayer);
        gAudioDevice.pMusPlayer = NULL;
    }

    if (gAudioDevice.pSoundBuffer != NULL) {
        free(gAudioDevice.pSoundBuffer);
        gAudioDevice.pSoundBuffer = NULL;
    }

    gAudioDevice.fOpened = FALSE;
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
    if (gAudioDevice.pSoundPlayer) {
        gAudioDevice.pSoundPlayer->Play(gAudioDevice.pSoundPlayer, abs(iSoundNum), FALSE, 0.0f);
    }
}

void AUDIO_PlayMusic(int iNumRIX, int fLoop, float flFadeTime) {
    AUDIO_Lock();
    if (gAudioDevice.pMusPlayer) {
        gAudioDevice.pMusPlayer->Play(gAudioDevice.pMusPlayer, iNumRIX, fLoop, flFadeTime);
    }
    AUDIO_Unlock();
}

void AUDIO_EnableMusic(int fEnable) {
  gAudioDevice.fMusicEnabled = fEnable;
}

int AUDIO_MusicEnabled(void) {
    return gAudioDevice.fMusicEnabled;
}

void AUDIO_EnableSound(int fEnable) {
    gAudioDevice.fSoundEnabled = fEnable;
}

int AUDIO_SoundEnabled(void) {
    return gAudioDevice.fSoundEnabled;
}

void AUDIO_Lock(void) {
    SDL_LockAudioDevice(gAudioDevice.id);
}

void AUDIO_Unlock(void) {
    SDL_UnlockAudioDevice(gAudioDevice.id);
}
