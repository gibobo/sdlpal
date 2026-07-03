#include "../backend_select.h"
#if defined(ARDUINO_ARCH_ESP32) && PAL_ESP32_BACKEND_WEB
#include "../../driver.h"
#include "../../video.h"
#include "DrvIf_internal.h"
#include "esp_heap_caps.h"
#include "utils/esp32_pins.h"
#include "utils/esp32_webserver.h"
#include <esp_attr.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <stddef.h>
#include <string.h>

#define ENABLE_LCD 0

// Double framebuffer: Core 0 writes to back_buffer[fb_write_idx],
// Core 1 (web task) reads from back_buffer[fb_read_idx].
// After each frame render, DRIVER_SwapFramebuffer() swaps indices under fb_mutex.
// If the web task is mid-transfer (fb_read_locked=1), the swap is skipped —
// Core 0 keeps writing to the same buffer next frame (acceptable frame drop).
static uint8_t *back_buffer[2] = {NULL, NULL};
static uint8_t *stable_palette = NULL;  // palette copy for web task, guarded by fb_mutex
static int fb_write_idx   = 0;          // index Core 0 renders into (Core 0 only)
static int fb_read_idx    = 0;          // index Core 1 reads from  (guarded by fb_mutex)
static int fb_read_locked = 0;          // 1 = web task is mid TCP write (guarded by fb_mutex)
SemaphoreHandle_t fb_mutex = NULL;      // exported for DrvIf_internal.h declaration

void DRIVER_FrameShow(
    uint8_t *frame,
    const uint16_t roi_x,
    const uint16_t roi_y,
    const uint16_t roi_w,
    const uint16_t roi_h,
    const uint8_t padding_flag)
{
    if (frame == NULL || back_buffer[fb_write_idx] == NULL || roi_w == 0 || roi_h == 0)
        return;

    uint16_t x, y;
    uint16_t roi_x2 = roi_x + roi_w;
    uint16_t roi_y2 = roi_y + roi_h;
    uint8_t *src = frame;
    uint8_t *dst = back_buffer[fb_write_idx];

    for (y = 0; y < SCREEN_H; y++)
    {
        if ((y >= roi_y) && (y < roi_y2))
        {
            for (x = 0; x < SCREEN_W; x++)
            {
                if ((x >= roi_x) && (x < roi_x2))
                {
                    dst[x] = src[x];
                }
                else if (padding_flag)
                {
                    dst[x] = 0;
                }
            }
            src += SCREEN_W;
        }
        else if (padding_flag)
            memset(dst, 0, SCREEN_W);
        else
            src += SCREEN_W;
        dst += SCREEN_W;
    }

    DRIVER_SwapFramebuffer();
}

void DRIVER_SwapFramebuffer(void)
{
    xSemaphoreTake(fb_mutex, portMAX_DELAY);
    if (!fb_read_locked)
    {
        int tmp      = fb_write_idx;
        fb_write_idx = fb_read_idx;
        fb_read_idx  = tmp;
    }
    xSemaphoreGive(fb_mutex);
}

void DRIVER_UpdatePalette(const unsigned char *rgPalette)
{
    if (rgPalette == NULL || stable_palette == NULL)
        return;

    xSemaphoreTake(fb_mutex, portMAX_DELAY);
    memcpy(stable_palette, rgPalette, 256 * 3);
    DRIVER_MarkPaletteUpdate();
    xSemaphoreGive(fb_mutex);
}

// Called by web task before starting TCP write.
// Snapshots fb_read_idx and stable_palette under fb_mutex, sets fb_read_locked.
// Returns the snapshotted buffer index; caller writes palette to local_pal_out (768 bytes).
int DRIVER_Web_BeginRead(uint8_t *local_pal_out)
{
    xSemaphoreTake(fb_mutex, portMAX_DELAY);
    int snap = fb_read_idx;
    fb_read_locked = 1;
    if (local_pal_out && stable_palette)
        memcpy(local_pal_out, stable_palette, 256 * 3);
    xSemaphoreGive(fb_mutex);
    return snap;
}

// Called by web task after TCP write completes.
void DRIVER_Web_EndRead(void)
{
    xSemaphoreTake(fb_mutex, portMAX_DELAY);
    fb_read_locked = 0;
    xSemaphoreGive(fb_mutex);
}

// Returns the framebuffer at given index. Use index from DRIVER_Web_BeginRead().
const uint8_t *DRIVER_Web_GetFrameBuffer(int idx)
{
    return back_buffer[idx];
}

int DRIVER_Init_Video(void)
{
    back_buffer[0] = (uint8_t *)heap_caps_malloc(SCREEN_SIZE, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    back_buffer[1] = (uint8_t *)heap_caps_malloc(SCREEN_SIZE, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);
    stable_palette = (uint8_t *)heap_caps_malloc(256 * 3, MALLOC_CAP_8BIT | MALLOC_CAP_DMA);

    if (!back_buffer[0] || !back_buffer[1] || !stable_palette)
        return -1;

    fb_mutex = xSemaphoreCreateMutex();
    if (fb_mutex == NULL)
        return -1;

    fb_write_idx   = 0;
    fb_read_idx    = 0;
    fb_read_locked = 0;

    return 0;
}

void DRIVER_DeInit_Video(void)
{
    if (fb_mutex)
    {
        vSemaphoreDelete(fb_mutex);
        fb_mutex = NULL;
    }
    heap_caps_free(back_buffer[0]);
    heap_caps_free(back_buffer[1]);
    heap_caps_free(stable_palette);
    back_buffer[0] = NULL;
    back_buffer[1] = NULL;
    stable_palette  = NULL;
}
#endif /* ARDUINO_ARCH_ESP32 && PAL_ESP32_BACKEND_WEB */
