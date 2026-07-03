#include "../backend_select.h"
#if defined(ARDUINO_ARCH_ESP32) && PAL_ESP32_BACKEND_HW
#include "../../audio.h"
#include "../../driver.h"
#include "../../global.h"
#include "DrvIf_internal.h"
#include <driver/i2s.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CONFIG_SOUND_ENA
// #define CONFIG_HW_INTERNAL_DAC
#define CONFIG_HW_EXTERNAL_DAC

// I2S port number
#define I2S_NUM           I2S_NUM_1
// I2S sample rate
#define I2S_SAMPLE_RATE   PAL_AUDIO_SAMPLING_RATE
// I2S buffer count less than 128 and more than 2
#define I2S_DMA_BUF_COUNT (2)
// I2S buffer length in samples
#define I2S_DMA_BUF_LEN   PAL_AUDIO_SAMPLES_PER_CHUNK * PAL_AUDIO_CHANNEL_COUNT * PAL_AUDIO_BYTES_PER_SAMPLE
// Please confirm PCM5102 pin configuration
#define I2S_BCK_IO        (14) // PCM5102 BCK PIN
#define I2S_DATA_IO       (12) // PCM5102 DIN PIN
#define I2S_WS_IO         (13) // PCM5102 LCK PIN

static bool audio_running = false;
static TaskHandle_t audio_task_handle = NULL;
static void *buffer = NULL;

void audio_callback(void)
{
    size_t bytes_written = 0;
    // Clear buffer and fill with audio data
    memset(buffer, 0, I2S_DMA_BUF_LEN);

    // Call the audio fill buffer function
    AUDIO_FillBuffer(buffer, I2S_DMA_BUF_LEN);

    // Write to I2S transmission, wait for transmission completion
    i2s_write(I2S_NUM, (const char *)buffer, I2S_DMA_BUF_LEN, &bytes_written, 0);
}

// Audio playback task: continuously calls AUDIO_FillBuffer() to fill audio data and writes to I2S output
void audio_task(void *arg)
{
    (void)arg;
    printf("Audio task started...\n");
    audio_running = true;
    while (audio_running)
    {
        audio_callback();
    }
    printf("Audio task exiting...\n");
    vTaskDelete(NULL);
}

// Initialize ESP32 I2S and start playback task
int DRIVER_Init_Audio(void)
{
    printf("Audio parameters:\n");
    printf("Sample rate: %d Hz\n", I2S_SAMPLE_RATE);
    printf("DMA buffer count: %d\n", I2S_DMA_BUF_COUNT);
    printf("DMA buffer length: %d\n", I2S_DMA_BUF_LEN);
    printf("Buffer size: %d bytes\n", I2S_DMA_BUF_LEN);

#if defined(CONFIG_SOUND_ENA)
    // Configure I2S parameters
    i2s_config_t i2s_config = {
#if defined(CONFIG_HW_EXTERNAL_DAC)
        .mode = I2S_MODE_MASTER | I2S_MODE_TX,
#elif defined(CONFIG_HW_INTERNAL_DAC)
        .mode = I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN,
#endif
        .communication_format = I2S_COMM_FORMAT_I2S_MSB,
        .sample_rate = I2S_SAMPLE_RATE,
        .bits_per_sample = PAL_AUDIO_BIT_DEPTH,
        .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
        .intr_alloc_flags = 0,
        .dma_buf_count = I2S_DMA_BUF_COUNT,
        .dma_buf_len = I2S_DMA_BUF_LEN,
        .use_apll = false,
        .tx_desc_auto_clear = true,
    };

    esp_err_t err;
    err = i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    if (err != ESP_OK)
    {
        printf("Failed to install I2S driver: %d\n", (int)err);
        return err;
    }

#if defined(CONFIG_HW_INTERNAL_DAC)
    i2s_set_pin(I2S_NUM, NULL);
    i2s_set_dac_mode(I2S_DAC_CHANNEL_LEFT_EN);
#elif defined(CONFIG_HW_EXTERNAL_DAC)
    // Configure I2S pins
    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCK_IO,
        .ws_io_num = I2S_WS_IO,
        .data_out_num = I2S_DATA_IO,
        .data_in_num = I2S_PIN_NO_CHANGE, // TX only, no receive needed
    };
    err = i2s_set_pin(I2S_NUM, &pin_config);
    if (err != ESP_OK)
    {
        printf("Failed to set I2S pins: %d\n", (int)err);
        i2s_driver_uninstall(I2S_NUM);
        return err;
    }
#endif
    // Bytes to write each time: depends on samples processed per iteration, using DMA buffer length as example
    buffer = heap_caps_calloc(PAL_AUDIO_SAMPLES_PER_CHUNK * PAL_AUDIO_CHANNEL_COUNT, PAL_AUDIO_BYTES_PER_SAMPLE, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    if (buffer == NULL)
    {
        printf("Failed to allocate audio buffer\n");
        return ESP_FAIL;
    }

#if 0
// Create playback task with higher priority and more stack
    if (xTaskCreatePinnedToCore(audio_task, "audio_task", 8192, NULL, configMAX_PRIORITIES - 2, &audio_task_handle, 1) != pdPASS)
    {
        printf("Failed to create playback task\n");
        i2s_driver_uninstall(I2S_NUM);
        return ESP_FAIL;
    }
#endif
#endif
    printf("Audio initialized successfully.\n");
    return ESP_OK;
}

// Terminate audio playback and uninstall I2S driver
void DRIVER_DeInit_Audio(void)
{
    audio_running = false;

    free(buffer);
    if (audio_task_handle != NULL)
    {
        vTaskDelete(audio_task_handle);
        audio_task_handle = NULL;
    }
#if defined(CONFIG_SOUND_ENA)
    i2s_driver_uninstall(I2S_NUM);
#endif
}

void DRIVER_Audio_Lock(void) {}

void DRIVER_Audio_Unlock(void) {}
#endif /* ARDUINO_ARCH_ESP32 && PAL_ESP32_BACKEND_HW */
