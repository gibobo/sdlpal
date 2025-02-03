#include "audio/audio.h"
#include "global.h"
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include <string.h>

static ma_device device;

static void audio_callback(ma_device *pDevice, void *stream, const void *pInput, ma_uint32 frameCount) {
  int len = frameCount * gConfig.iAudioChannels * sizeof(short);
  memset(stream, 0, len);
  AUDIO_FillBuffer(stream, len);
}

int DRIVER_Init_Audio(void) {
  ma_device_config deviceConfig;
  deviceConfig = ma_device_config_init(ma_device_type_playback);
  deviceConfig.playback.format = ma_format_s16;
  deviceConfig.playback.channels = gConfig.iAudioChannels;
  deviceConfig.sampleRate = gConfig.iSampleRate;
  deviceConfig.dataCallback = audio_callback;
  deviceConfig.pUserData = NULL;
  //  deviceConfig.periodSizeInFrames = gConfig.wAudioBufferSize;
  //  deviceConfig.periods = 2;

  if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
    printf("Failed to initialize playback device.\n");
    return -1;
  }

  // Let the callback function run so that musics will be played.
  if (ma_device_start(&device) != MA_SUCCESS) {
    printf("Failed to start playback device.\n");
    ma_device_uninit(&device);
    return -1;
  }
  return 0;
}

void DRIVER_DeInit_Audio(void) {
  ma_device_uninit(&device);
}