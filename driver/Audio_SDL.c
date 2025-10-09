#include "../src/audio.h"
#include "../src/driver.h"
#include "../src/global.h"
#include "DrvIf_internal.h"
#include <SDL_audio.h>
#include <string.h>

static unsigned int AudioDeviceId = 0;

static void SDLCALL audio_callback(void *udata, unsigned char *stream, int len)
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
    memset(stream, 0, (size_t)len);
    AUDIO_FillBuffer(stream, (unsigned int)len);
}

int DRIVER_Init_Audio(void)
{
    SDL_AudioSpec audio_spec;
    // Open the audio device.
    audio_spec.freq = PAL_AUDIO_SAMPLE_RATE;
    audio_spec.format = AUDIO_S16SYS;
    audio_spec.channels = PAL_AUDIO_CHANNEL_NUM;
    audio_spec.samples = PAL_AUDIO_BUFFER_SIZE;
    audio_spec.callback = audio_callback;
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
