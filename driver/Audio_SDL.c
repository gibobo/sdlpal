#include "../src/audio.h"
#include "../src/driver.h"
#include "../src/global.h"
#include "DrvIf_internal.h"
#include <SDL_audio.h>
#include <stdint.h>
#include <string.h>

static uint32_t AudioDeviceId = 0;

static void SDLCALL audio_callback(void *udata, uint8_t *stream, int len)
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
    (void)udata;
    memset(stream, PAL_AUDIO_SAMPLE_SILENCE, (size_t)len);
    AUDIO_FillBuffer(stream, (uint32_t)len);
}

int DRIVER_Init_Audio(void)
{
    SDL_AudioSpec audio_spec;
    // Open the audio device.
    audio_spec.freq = PAL_AUDIO_SAMPLING_RATE;
#if PAL_AUDIO_BIT_DEPTH == 8U
    audio_spec.format = AUDIO_U8;
#elif PAL_AUDIO_BIT_DEPTH == 16U
    audio_spec.format = AUDIO_S16SYS;
#else
#error Unsupported PAL_AUDIO_BIT_DEPTH for SDL audio backend
#endif
    audio_spec.channels = PAL_AUDIO_CHANNEL_COUNT;
    audio_spec.samples = PAL_AUDIO_SAMPLES_PER_CHUNK;
    audio_spec.callback = audio_callback;
    audio_spec.silence = (Uint8)(PAL_AUDIO_SAMPLE_SILENCE & 0xFF);
    AudioDeviceId = SDL_OpenAudioDevice(NULL, 0, &audio_spec, NULL, 0);
    if (AudioDeviceId == 0)
        return -3; // Failed

    // Let the callback function run so that musics will be played.
    SDL_PauseAudioDevice(AudioDeviceId, 0);
    return 0;
}

void DRIVER_DeInit_Audio(void)
{
    SDL_CloseAudioDevice(AudioDeviceId);
    AudioDeviceId = 0;
}

void DRIVER_Audio_Lock(void)
{
    SDL_LockAudioDevice(AudioDeviceId);
}

void DRIVER_Audio_Unlock(void)
{
    SDL_UnlockAudioDevice(AudioDeviceId);
}
