#include "../src/driver.h"
#include "../src/util.h"
#include "../src/video.h"
#include "DrvIf_internal.h"
#include "caca.h"
#include <stdint.h>
#include <string.h>

static uint8_t *framebuffer = NULL;
static caca_canvas_t *cv = NULL;
caca_display_t *dp = NULL;
static caca_dither_t *caca_dither = NULL;

void DRIVER_FrameShow(
    uint8_t *frame,
    const uint16_t roi_x,
    const uint16_t roi_y,
    const uint16_t roi_w,
    const uint16_t roi_h,
    const uint8_t padding_flag)
{
    if (framebuffer == NULL)
        return;

    uint16_t x, y;
    uint16_t roi_x2 = roi_x + roi_w;
    uint16_t roi_y2 = roi_y + roi_h;
    uint8_t *src = frame;
    uint8_t *dst = framebuffer;

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
    if (cv && caca_dither)
        caca_dither_bitmap(
            cv, 0, 0,
            caca_get_canvas_width(cv),
            caca_get_canvas_height(cv),
            caca_dither, framebuffer);
    if (dp)
        caca_refresh_display(dp);
}

void DRIVER_UpdatePalette(const uint8_t *rgPalette)
{
    if (rgPalette == NULL)
        return;

    uint32_t r[256] = {0};
    uint32_t g[256] = {0};
    uint32_t b[256] = {0};
    uint32_t a[256] = {0};
    uint32_t i;
    for (i = 0; i < 256; i++)
    {
        r[i] = (uint32_t)(rgPalette[i * 3 + 0]) * 0xfff / 255;
        g[i] = (uint32_t)(rgPalette[i * 3 + 1]) * 0xfff / 255;
        b[i] = (uint32_t)(rgPalette[i * 3 + 2]) * 0xfff / 255;
        a[i] = 0;
    }
    if (caca_dither)
        caca_set_dither_palette(caca_dither, r, g, b, a);
}

int DRIVER_Init_Video(void)
{
    cv = caca_create_canvas(80, 24);
    if (cv)
        dp = caca_create_display(cv);
    if (dp)
        caca_set_display_title(dp, "PAL_CACA");
    caca_dither = caca_create_dither(8, SCREEN_W, SCREEN_H, SCREEN_W, 0, 0, 0, 0);
    // caca_set_dither_algorithm(caca_dither, caca_get_dither_algorithm_list(NULL)[0]);
    framebuffer = (uint8_t *)UTIL_malloc(SCREEN_SIZE);
    return 0;
}

void DRIVER_DeInit_Video(void)
{
    caca_free_display(dp);
    caca_free_canvas(cv);
    caca_free_dither(caca_dither);
    UTIL_free(framebuffer);
    framebuffer = NULL;
}
