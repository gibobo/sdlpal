#include "../src/audio.h"
#include "../src/global.h"
#include "../src/util.h"
#include "mongoose.h"
#include <math.h>

static unsigned char *send_audio = NULL;
static unsigned int send_audio_size = 0;
static double duration_sum = 0.0;
static unsigned int audio_start_tick = 0;
static const double duration_ms = 1000.0 * (double)PAL_AUDIO_BUFFER_SIZE / (double)PAL_AUDIO_SAMPLE_RATE;
extern struct mg_connection *ws_conn;

void send_audio_data(void)
{
    int16_t *audio_data = (int16_t *)(send_audio + 1);
    memset(audio_data, 0, send_audio_size - 1);

    if ((unsigned int)UTIL_GetTicks() + duration_ms < audio_start_tick + duration_sum)
        return;

    AUDIO_FillBuffer(audio_data, send_audio_size - 1);
    if (ws_conn == NULL || mg_ws_send(ws_conn, send_audio, send_audio_size, WEBSOCKET_OP_BINARY) == -1)
    {
        fprintf(stderr, "Failed to send audio data\n");
        duration_sum = 0.0;
        audio_start_tick = 0;
    }
    else
    {
        duration_sum += duration_ms;
        if (audio_start_tick == 0)
            audio_start_tick = (unsigned int)UTIL_GetTicks();
    }
}

void send_audio_config()
{
    uint8_t send_config[] = {
        3,                                   // Type 3: audio format
        (PAL_AUDIO_SAMPLE_RATE >> 8) & 0xFF, // Frequency high byte
        PAL_AUDIO_SAMPLE_RATE & 0xFF,        // Frequency low byte
        PAL_AUDIO_CHANNEL_NUM,               // Number of channels (1 byte)
        PAL_AUDIO_BIT_DEPTH,                 // Bit depth (1 byte)
    };

    if (ws_conn == NULL || mg_ws_send(ws_conn, send_config, sizeof(send_config), WEBSOCKET_OP_BINARY) == -1)
    {
        fprintf(stderr, "Failed to send audio config\n");
    }
    else
    {
        fprintf(stdout, "Audio config sent: %d Hz, %d channels, %d bits\n",
                PAL_AUDIO_SAMPLE_RATE, PAL_AUDIO_CHANNEL_NUM, PAL_AUDIO_BIT_DEPTH);
    }

    duration_sum = 0.0;
    audio_start_tick = 0;
}

int DRIVER_Init_Audio(void)
{
    send_audio_size = 1 + PAL_AUDIO_BUFFER_SIZE * PAL_AUDIO_CHANNEL_NUM * PAL_AUDIO_BYTES_PER_SAMPLE;
    send_audio = (unsigned char *)UTIL_malloc(send_audio_size);
    send_audio[0] = 1;
    return 0;
}

void DRIVER_DeInit_Audio(void)
{
    UTIL_free(send_audio);
    send_audio = NULL;
    send_audio_size = 0;
}

void DRIVER_Audio_Lock(void) {}

void DRIVER_Audio_Unlock(void) {}
