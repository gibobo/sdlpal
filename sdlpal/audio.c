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

#include "audio.h"
#include "global.h"
#include "palcfg.h"
#include "players.h"
#include "resampler.h"
#include "util.h"
#include "common.h"

/* WASAPI need fewer samples for less gapping */
#ifndef PAL_AUDIO_FORCE_BUFFER_SIZE_WASAPI
# define PAL_AUDIO_FORCE_BUFFER_SIZE_WASAPI   512
#endif

typedef void(*ResampleMixFunction)(void *, const void *, int, void *, int, int, uint8_t);

typedef struct tagAUDIODEVICE {
   SDL_AudioSpec spec; /* Actual-used sound specification */
   AUDIOPLAYER *pMusPlayer;
   AUDIOPLAYER *pSoundPlayer;
   void *pSoundBuffer; /* The output buffer for sound */
   SDL_AudioDeviceID id;
   int iMusicVolume;  /* The BGM volume ranged in [0, 128] for better performance */
   int iSoundVolume;  /* The sound effect volume ranged in [0, 128] for better performance */
   int fMusicEnabled; /* Is BGM enabled? */
   int fSoundEnabled; /* Is sound effect enabled? */
   int fOpened;       /* Is the audio device opened? */
} AUDIODEVICE;

static AUDIODEVICE gAudioDevice;

PAL_FORCE_INLINE
void
AUDIO_MixNative(
	short     *dst,
	short     *src,
	int        samples
)
{
	while (samples > 0)
	{
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

PAL_FORCE_INLINE
void
AUDIO_AdjustVolume(
	short     *srcdst,
	int        iVolume,
	int        samples
)
{
	if (iVolume == SDL_MIX_MAXVOLUME) return;
	if (iVolume == 0) { memset(srcdst, 0, samples << 1); return; }
	while (samples > 0)
	{
		*srcdst = *srcdst * iVolume / SDL_MIX_MAXVOLUME;
		samples--; srcdst++;
	}
}

static void SDLCALL
AUDIO_FillBuffer(
   void            *udata,
   unsigned char *          stream,
   int             len
)
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

   //
   // Play music
   //
   if (gAudioDevice.fMusicEnabled && gAudioDevice.iMusicVolume > 0)
   {
      if (gAudioDevice.pMusPlayer)
      {
         gAudioDevice.pMusPlayer->FillBuffer(gAudioDevice.pMusPlayer, stream, len);
      }

      //
      // Adjust volume for music
      //
      AUDIO_AdjustVolume((short *)stream, gAudioDevice.iMusicVolume, len >> 1);
   }

   //
   // Play sound
   //
   if (gAudioDevice.fSoundEnabled && gAudioDevice.pSoundPlayer && gAudioDevice.iSoundVolume > 0)
   {
	   memset(gAudioDevice.pSoundBuffer, 0, len);

	   gAudioDevice.pSoundPlayer->FillBuffer(gAudioDevice.pSoundPlayer, gAudioDevice.pSoundBuffer, len);

	   //
	   // Adjust volume for sound
	   //
	   AUDIO_AdjustVolume((short *)gAudioDevice.pSoundBuffer, gAudioDevice.iSoundVolume, len >> 1);

	   //
	   // Mix sound & music
	   //
	   AUDIO_MixNative((short *)stream, gAudioDevice.pSoundBuffer, len >> 1);
   }
}

int
AUDIO_OpenDevice(
   void
)
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

   if (gAudioDevice.fOpened)
   {
      //
      // Already opened
      //
      return -1;
   }

   gAudioDevice.fOpened = FALSE;
   gAudioDevice.fMusicEnabled = TRUE;
   gAudioDevice.fSoundEnabled = TRUE;
   gAudioDevice.iMusicVolume = gConfig.iMusicVolume * SDL_MIX_MAXVOLUME / PAL_MAX_VOLUME;
   gAudioDevice.iSoundVolume = gConfig.iSoundVolume * SDL_MIX_MAXVOLUME / PAL_MAX_VOLUME;

   //
   // Initialize the resampler module
   //
   resampler_init();

    for( int i = 0; i<SDL_GetNumAudioDrivers();i++)
    {
        UTIL_LogOutput(LOGLEVEL_VERBOSE, "Available audio driver %d:%s\n", i, SDL_GetAudioDriver(i));
    }
    const char* driver_name = SDL_GetCurrentAudioDriver();
    if (driver_name) {
        UTIL_LogOutput(LOGLEVEL_VERBOSE, "Audio subsystem initialized; current driver is %s.\n", driver_name);
        if(SDL_strncmp(driver_name, "wasapi", 6)==0)
            gConfig.wAudioBufferSize = PAL_AUDIO_FORCE_BUFFER_SIZE_WASAPI;
    } else {
        UTIL_LogOutput(LOGLEVEL_VERBOSE, "Audio subsystem not initialized.\n");
    }
    for( int i = 0; i<SDL_GetNumAudioDevices(0);i++)
    {
        UTIL_LogOutput(LOGLEVEL_VERBOSE, "Available audio device %d:%s\n", i, SDL_GetAudioDeviceName(i,0));
    }
    UTIL_LogOutput(LOGLEVEL_VERBOSE, "OpenAudio: requesting audio device: %s\n",(gConfig.iAudioDevice >= 0 ? SDL_GetAudioDeviceName(gConfig.iAudioDevice, 0) : "default"));

   //
   // Open the audio device.
   //
   gAudioDevice.spec.freq = gConfig.iSampleRate;
   gAudioDevice.spec.format = AUDIO_S16SYS;
   gAudioDevice.spec.channels = gConfig.iAudioChannels;
   gAudioDevice.spec.samples = gConfig.wAudioBufferSize;
   gAudioDevice.spec.callback = AUDIO_FillBuffer;
   const char *device = gConfig.iAudioDevice >= 0 ? SDL_GetAudioDeviceName(gConfig.iAudioDevice, 0) : NULL;
   gAudioDevice.id = SDL_OpenAudioDevice(device, 0, &gAudioDevice.spec, &spec, 0);

   UTIL_LogOutput(LOGLEVEL_VERBOSE, "OpenAudio: requesting audio spec:freq %d, format %d, channels %d, samples %d\n", gAudioDevice.spec.freq, gAudioDevice.spec.format,  gAudioDevice.spec.channels, gAudioDevice.spec.samples);

   if (gAudioDevice.id < 0)
   {
      UTIL_LogOutput(LOGLEVEL_VERBOSE, "OpenAudio ERROR: %s, got spec:freq %d, format %d, channels %d, samples %d\n", SDL_GetError(), spec.freq, spec.format, spec.channels,  spec.samples);
      //
      // Failed
      //
      return -3;
   }
   else
   {
      UTIL_LogOutput(LOGLEVEL_VERBOSE, "OpenAudio succeed, got spec:freq %d, format %d, channels %d, samples %d\n", spec.freq, spec.format, spec.channels,  spec.samples);
      gAudioDevice.pSoundBuffer = malloc(gConfig.wAudioBufferSize * gConfig.iAudioChannels * sizeof(short));
   }

   gAudioDevice.fOpened = TRUE;

   //
   // Initialize the sound subsystem.
   //
   gAudioDevice.pSoundPlayer = SOUND_Init();

   //
   // Initialize the music subsystem.
   //
   gAudioDevice.pMusPlayer = RIX_Init(UTIL_GetFullPathName(UTIL_GlobalBuffer(0), PAL_GLOBAL_BUFFER_SIZE, gConfig.pszGamePath, "mus.mkf"));

   //
   // Let the callback function run so that musics will be played.
   //
   SDL_PauseAudioDevice(gAudioDevice.id, 0);

   return 0;
}

void
AUDIO_CloseDevice(
   void
)
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

   if (gAudioDevice.pSoundPlayer != NULL)
   {
      gAudioDevice.pSoundPlayer->Shutdown(gAudioDevice.pSoundPlayer);
      gAudioDevice.pSoundPlayer = NULL;
   }

   if (gAudioDevice.pMusPlayer)
   {
	   gAudioDevice.pMusPlayer->Shutdown(gAudioDevice.pMusPlayer);
	   gAudioDevice.pMusPlayer = NULL;
   }

   if (gAudioDevice.pSoundBuffer != NULL)
   {
      free(gAudioDevice.pSoundBuffer);
	  gAudioDevice.pSoundBuffer = NULL;
   }

   gAudioDevice.fOpened = FALSE;
}

SDL_AudioSpec*
AUDIO_GetDeviceSpec(
	void
)
{
	return &gAudioDevice.spec;
}

static int
AUDIO_ChangeVolumeByValue(
   int   *iVolume,
   int    iValue
)
{
   *iVolume += iValue;
   if (*iVolume > PAL_MAX_VOLUME)
      *iVolume = PAL_MAX_VOLUME;
   else if (*iVolume < 0)
      *iVolume = 0;
   return *iVolume;
}

void
AUDIO_PlaySound(
   int    iSoundNum
)
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
      gAudioDevice.pSoundPlayer->Play(gAudioDevice.pSoundPlayer, abs(iSoundNum), FALSE, 0.0f);
   }
}

void
AUDIO_PlayMusic(
   int       iNumRIX,
   int      fLoop,
   float     flFadeTime
)
{
   AUDIO_Lock();
   if (gAudioDevice.pMusPlayer)
   {
      gAudioDevice.pMusPlayer->Play(gAudioDevice.pMusPlayer, iNumRIX, fLoop, flFadeTime);
   }
   AUDIO_Unlock();
}

void
AUDIO_EnableMusic(
   int   fEnable
)
{
   gAudioDevice.fMusicEnabled = fEnable;
}

int
AUDIO_MusicEnabled(
   void
)
{
   return gAudioDevice.fMusicEnabled;
}

void
AUDIO_EnableSound(
   int   fEnable
)
{
	gAudioDevice.fSoundEnabled = fEnable;
}

int
AUDIO_SoundEnabled(
   void
)
{
   return gAudioDevice.fSoundEnabled;
}

void
AUDIO_Lock(
	void
)
{
	SDL_LockAudioDevice(gAudioDevice.id);
}

void
AUDIO_Unlock(
	void
)
{
	SDL_UnlockAudioDevice(gAudioDevice.id);
}
