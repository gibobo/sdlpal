#include "../src/audio.h"
#include "../src/driver.h"
#include "../src/global.h"
#include "../src/util.h"
#include "DrvIf_internal.h"
#include "mongoose.h"
#include <math.h>

static unsigned char *audio_package = NULL;
static unsigned int audio_package_size = 0;
static void *audio_data = NULL;
static unsigned int audio_data_size = 0;
static double duration_sum = 0.0;
static unsigned long audio_start_tick = 0;
static const double duration_ms = 1000.0 * (double)PAL_AUDIO_SAMPLES_PER_CHUNK / (double)PAL_AUDIO_OUTPUT_SAMPLE_RATE;
extern struct mg_connection *ws_conn;

/* Function prototypes */
void send_audio_data(void);
void send_audio_config(void);

void send_audio_data(void)
{
    if (UTIL_GetMilliseconds() + duration_ms < audio_start_tick + duration_sum)
        return;

    memset(audio_data, PAL_AUDIO_SAMPLE_SILENCE, audio_data_size);
    AUDIO_FillBuffer(audio_data, audio_data_size);

    if (ws_conn == NULL || (size_t)mg_ws_send(ws_conn, audio_package, audio_package_size, WEBSOCKET_OP_BINARY) == (size_t)-1)
    {
        fprintf(stderr, "Failed to send audio data\n");
        duration_sum = 0.0;
        audio_start_tick = 0U;
    }
    else
    {
        duration_sum += duration_ms;
        if (audio_start_tick == 0U)
            audio_start_tick = UTIL_GetMilliseconds();
    }
}

void send_audio_config(void)
{
    uint8_t send_config[] = {
        3,                                          // Type 3: audio format
        (PAL_AUDIO_OUTPUT_SAMPLE_RATE >> 8) & 0xFF, // Frequency high byte
        PAL_AUDIO_OUTPUT_SAMPLE_RATE & 0xFF,        // Frequency low byte
        PAL_AUDIO_OUTPUT_CHANNEL_COUNT,             // Number of channels (1 byte)
        PAL_AUDIO_BIT_DEPTH,                        // Bit depth (1 byte)
    };

    if (ws_conn == NULL || (size_t)mg_ws_send(ws_conn, send_config, sizeof(send_config), WEBSOCKET_OP_BINARY) == (size_t)-1)
    {
        fprintf(stderr, "Failed to send audio config\n");
    }
    else
    {
        fprintf(stdout, "Audio config sent: %d Hz, %d channels, %d bits\n",
                PAL_AUDIO_OUTPUT_SAMPLE_RATE, PAL_AUDIO_OUTPUT_CHANNEL_COUNT, PAL_AUDIO_BIT_DEPTH);
    }

    duration_sum = 0.0;
    audio_start_tick = 0;
}

int DRIVER_Init_Audio(void)
{
    audio_data_size = PAL_AUDIO_SAMPLES_PER_CHUNK * PAL_AUDIO_OUTPUT_CHANNEL_COUNT * PAL_AUDIO_BYTES_PER_SAMPLE;
    audio_package_size = 1 + audio_data_size;
    audio_package = (unsigned char *)UTIL_malloc(audio_package_size);
    audio_package[0] = 1;
    audio_data = (void *)(audio_package + 1);
    return 0;
}

void DRIVER_DeInit_Audio(void)
{
    UTIL_free(audio_package);
    audio_package = NULL;
    audio_package_size = 0;
}

void DRIVER_Audio_Lock(void) {}

void DRIVER_Audio_Unlock(void) {}
