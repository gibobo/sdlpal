#include "../src/audio.h"
#include "../src/driver.h"
#include "../src/global.h"
#include "DrvIf_internal.h"
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio/miniaudio.h"
#include <string.h>

static ma_device device;
#ifndef MA_NO_THREADING
static ma_mutex dmutex;
#endif

static void audio_callback(ma_device *pDevice, void *stream, const void *pInput, ma_uint32 frameCount)
{
    (void)pDevice;
    (void)pInput;
    AUDIO_FillBuffer(
        stream, 
        frameCount * PAL_AUDIO_OUTPUT_CHANNEL_COUNT * PAL_AUDIO_BYTES_PER_SAMPLE
    );
}

void DRIVER_DeInit_Audio(void)
{
#ifndef MA_NO_THREADING
    ma_mutex_uninit(&dmutex);
#endif
    ma_device_uninit(&device);
    memset(&device, 0, sizeof(ma_device));
}

int DRIVER_Init_Audio(void)
{
    ma_device_config deviceConfig;
    deviceConfig = ma_device_config_init(ma_device_type_playback);
#if PAL_AUDIO_BIT_DEPTH == 8U
    deviceConfig.playback.format = ma_format_u8;
#elif PAL_AUDIO_BIT_DEPTH == 16U
    deviceConfig.playback.format = ma_format_s16;
#else
#error Unsupported PAL_AUDIO_BIT_DEPTH for miniaudio backend
#endif
    deviceConfig.playback.channels = PAL_AUDIO_OUTPUT_CHANNEL_COUNT;
    deviceConfig.sampleRate = PAL_AUDIO_OUTPUT_SAMPLE_RATE;
    deviceConfig.periodSizeInFrames = PAL_AUDIO_SAMPLES_PER_CHUNK;
    deviceConfig.dataCallback = audio_callback;
    deviceConfig.pUserData = NULL;

    if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS)
    {
        fprintf(stderr, "Failed to initialize playback device.\n");
        return -1;
    }

#ifndef MA_NO_THREADING
    if (ma_mutex_init(&dmutex) != MA_SUCCESS)
    {
        fprintf(stderr, "Failed to initialize mutex context.\n");
        return -1;
    }
#endif

    // Let the callback function run so that musics will be played.
    if (ma_device_start(&device) != MA_SUCCESS)
    {
        fprintf(stderr, "Failed to start playback device.\n");
        DRIVER_DeInit_Audio();
        return -1;
    }
    return 0;
}

void DRIVER_Audio_Lock(void)
{
#ifndef MA_NO_THREADING
    ma_mutex_lock(&dmutex);
#endif
}

void DRIVER_Audio_Unlock(void)
{
#ifndef MA_NO_THREADING
    ma_mutex_unlock(&dmutex);
#endif
}
