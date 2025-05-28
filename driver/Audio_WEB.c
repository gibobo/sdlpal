#include "../src/audio.h"
#include "../src/global.h"
#include "../src/util.h"
#include "mongoose.h"
#include <math.h>

static unsigned char *send_audio = NULL;
static unsigned int send_audio_size = 0;
static double duration_sum = 0.0;
static long last_tick = 0;
static const double duration_ms = 1000.0 * (double)PAL_AUDIO_BUFFER_SIZE / (double)PAL_AUDIO_SAMPLE_RATE;
extern struct mg_connection *ws_conn;

void clear_audio(void)
{
    duration_sum = 0.0;
    last_tick = 0;
}

void generate_audio(void)
{
    long ticks_diff = UTIL_GetTicks() - last_tick;

    int16_t *audio_data = (int16_t *)(send_audio + 1);
    memset(audio_data, 0, send_audio_size - 1);

    if (ws_conn == NULL)
    {
        clear_audio();
        return;
    }

    if (ticks_diff + duration_ms < duration_sum)
    {
        return;
    }

    AUDIO_FillBuffer(audio_data, send_audio_size - 1);
    mg_ws_send(ws_conn, send_audio, send_audio_size, WEBSOCKET_OP_BINARY);
    duration_sum += duration_ms;
    if (last_tick == 0)
        last_tick = UTIL_GetTicks();
}

void send_audio_config()
{
    if (ws_conn == NULL)
        return;
    uint8_t send_config[13];
    send_config[0] = 3; // 類型 3：音頻格式
    send_config[1] = (PAL_AUDIO_SAMPLE_RATE >> 24) & 0xFF;
    send_config[2] = (PAL_AUDIO_SAMPLE_RATE >> 16) & 0xFF;
    send_config[3] = (PAL_AUDIO_SAMPLE_RATE >> 8) & 0xFF;
    send_config[4] = PAL_AUDIO_SAMPLE_RATE & 0xFF;
    send_config[5] = (PAL_AUDIO_CHANNEL_NUM >> 24) & 0xFF;
    send_config[6] = (PAL_AUDIO_CHANNEL_NUM >> 16) & 0xFF;
    send_config[7] = (PAL_AUDIO_CHANNEL_NUM >> 8) & 0xFF;
    send_config[8] = PAL_AUDIO_CHANNEL_NUM & 0xFF;
    send_config[9] = (PAL_AUDIO_BIT_DEPTH >> 24) & 0xFF;
    send_config[10] = (PAL_AUDIO_BIT_DEPTH >> 16) & 0xFF;
    send_config[11] = (PAL_AUDIO_BIT_DEPTH >> 8) & 0xFF;
    send_config[12] = PAL_AUDIO_BIT_DEPTH & 0xFF;
    if (mg_ws_send(ws_conn, send_config, sizeof(send_config), WEBSOCKET_OP_BINARY) == -1)
    {
        printf("Failed to send audio config\n");
    }
    else
    {
        printf("Audio config sent: %d Hz, %d channels, %d bits\n",
               PAL_AUDIO_SAMPLE_RATE, PAL_AUDIO_CHANNEL_NUM, PAL_AUDIO_BIT_DEPTH);
    }
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
