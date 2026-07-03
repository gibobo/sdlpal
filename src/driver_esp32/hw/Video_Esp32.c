#include "../backend_select.h"
#if defined(ARDUINO_ARCH_ESP32) && PAL_ESP32_BACKEND_HW
#include "../../driver.h"
#include "DrvIf_internal.h"
#include "esp_heap_caps.h"
#include "utils/esp32_pins.h"
#include <esp_attr.h>
#include <string.h>
#ifdef CONFIG_IDF_TARGET_ESP32S3
#include "utils/ili9341_lib.h"
#else
#include "utils/CompositeColorOutput.h"
#endif

static unsigned int pal_palette[256]; // Palette array (1 KB)
#ifdef ILI9341_LIB_H
// ILI9341 display variables
static uint16_t *framebuffer = NULL;
#else
// Composite output variables
static unsigned char *framebuffer = NULL;
static unsigned char *linebuffer[NTSC_VIDEO_HEIGHT];
#endif

// Optimized scanline rendering function for better performance
#ifdef ILI9341_LIB_H
static inline void IRAM_ATTR process_scanline_ili9341(
    unsigned char *src_line,
    uint16_t *dst_line,
    unsigned short roi_x,
    unsigned short roi_x2,
    unsigned char padding_flag)
{
    if (padding_flag)
    {
        // Left padding
        for (unsigned short x = 0; x < roi_x; x++)
            dst_line[x] = 0x0000;

        // Active region with optimized palette conversion
        for (unsigned short x = roi_x; x < roi_x2; x++)
        {
            unsigned int rgb888 = pal_palette[src_line[x]];
            // Fast RGB888 to RGB565 conversion using bit operations
            dst_line[x] = ((rgb888 & 0xF80000) >> 8) | ((rgb888 & 0xFC00) >> 5) | ((rgb888 & 0xFF) >> 3);
        }

        // Right padding
        for (unsigned short x = roi_x2; x < VIDEO_SCREEN_W; x++)
            dst_line[x] = 0x0000;
    }
    else
    {
        // Fast path - only active region
        for (unsigned short x = roi_x; x < roi_x2; x++)
        {
            unsigned int rgb888 = pal_palette[src_line[x]];
            dst_line[x] = ((rgb888 & 0xF80000) >> 8) | ((rgb888 & 0xFC00) >> 5) | ((rgb888 & 0xFF) >> 3);
        }
    }
}
#else
static inline void IRAM_ATTR process_scanline_composite(
    unsigned char *src_line,
    unsigned char *dst_line,
    unsigned short roi_x,
    unsigned short roi_x2,
    unsigned char padding_flag)
{
    if (padding_flag)
        memset(dst_line, 0, VIDEO_SCREEN_W);
    // Fast path - only active region
    memcpy(dst_line + roi_x, src_line + roi_x, roi_x2 - roi_x);
}
#endif

void DRIVER_FrameShow(
    unsigned char *frame,
    const unsigned short roi_x,
    const unsigned short roi_y,
    const unsigned short roi_w,
    const unsigned short roi_h,
    const unsigned char padding_flag)
{
    if(framebuffer == NULL || frame == NULL || roi_w == 0 || roi_h == 0)
        return;

    unsigned short roi_x2 = roi_x + roi_w;
    unsigned short roi_y2 = roi_y + roi_h;
    unsigned char *src = frame;
#ifdef ILI9341_LIB_H
    // Highly optimized ILI9341 implementation using scanline processing
    uint16_t *dst = framebuffer;

    if (padding_flag)
    {
        // Process entire screen with padding
        for (unsigned short y = 0; y < VIDEO_SCREEN_H; y++)
        {
            if (y >= roi_y && y < roi_y2)
            {
                // Active ROI scanline
                process_scanline_ili9341(src, dst, roi_x, roi_x2, padding_flag);
                src += VIDEO_SCREEN_W;
            }
            else
            {
                // Non-ROI scanline - fast memset
                memset(dst, 0, VIDEO_SCREEN_W * sizeof(uint16_t));
                if (y < roi_y2)
                    src += VIDEO_SCREEN_W;
            }
            dst += VIDEO_SCREEN_W;
        }
    }
    else
    {
        // Fast path - process only ROI region
        dst += roi_y * VIDEO_SCREEN_W;
        src += roi_y * VIDEO_SCREEN_W;

        for (unsigned short y = roi_y; y < roi_y2; y++)
        {
            process_scanline_ili9341(src, dst, roi_x, roi_x2, padding_flag);
            src += VIDEO_SCREEN_W;
            dst += VIDEO_SCREEN_W;
        }
    }

    ILI9341_showFrameBuffer(framebuffer);
#else
    // Highly optimized Composite output using scanline processing
    unsigned char *dst = framebuffer;

    if (padding_flag)
    {
        // Process entire screen with padding
        for (unsigned short y = 0; y < VIDEO_SCREEN_H; y++)
        {
            memset(dst, 0, VIDEO_SCREEN_W);
            if (y >= roi_y && y < roi_y2)
            {
                // Fast path - only active region
                memcpy(dst + roi_x, src + roi_x, roi_x2 - roi_x);
                src += VIDEO_SCREEN_W;
            }
            else
            {
                // Non-ROI scanline - fast memset for entire line
                if (y < roi_y2)
                    src += VIDEO_SCREEN_W;
            }
            dst += VIDEO_SCREEN_W;
        }
    }
    else
    {
        // Ultra-fast path - skip to ROI region and process only needed scanlines
        dst += roi_y * VIDEO_SCREEN_W;
        src += roi_y * VIDEO_SCREEN_W;

        for (unsigned short y = roi_y; y < roi_y2; y++)
        {
            // Active region
            memcpy(dst + roi_x, src + roi_x, roi_x2 - roi_x);
            src += VIDEO_SCREEN_W;
            dst += VIDEO_SCREEN_W;
        }
    }
#endif
}

void DRIVER_UpdatePalette(const unsigned char *rgPalette)
{
#ifdef ILI9341_LIB_H
    for (size_t i = 0; i < 256; i++)
    {
        unsigned char r = rgPalette[i * 3 + 0];
        unsigned char g = rgPalette[i * 3 + 1];
        unsigned char b = rgPalette[i * 3 + 2];
        pal_palette[i] = (r << 16) | (g << 8) | b;
    }
#else
    for (size_t i = 0; i < 256; i++)
        pal_palette[i] = convertRGBtoPalette(rgPalette[i * 3 + 0], rgPalette[i * 3 + 1], rgPalette[i * 3 + 2]);
#endif
}

int DRIVER_Init_Video(void)
{
#ifdef ILI9341_LIB_H
    // ILI9341 display initialization
    ILI9341_init();

    // Use unified UTIL API for framebuffer allocation with memory optimization
    framebuffer = (uint16_t *)UTIL_calloc(VIDEO_SCREEN_W * VIDEO_SCREEN_H, sizeof(uint16_t));
    if (!framebuffer)
    {
        printf("Error: Failed to allocate ILI9341 framebuffer memory\n");
        return -1;
    }

    // Fill buffer with blue color (RGB565: 0x001F)
    for (uint32_t i = 0; i < VIDEO_SCREEN_W * VIDEO_SCREEN_H; i++)
        framebuffer[i] = 0x001F;

    ILI9341_showFrameBuffer(framebuffer);
#else
    // Use unified UTIL API for framebuffer allocation with memory optimization
    framebuffer = heap_caps_calloc(NTSC_VIDEO_HEIGHT * VIDEO_SCREEN_W, sizeof(unsigned char), MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    if (!framebuffer)
    {
        printf("Error: Failed to allocate framebuffer memory\n");
        return -1;
    }

    for (unsigned short h = 0; h < NTSC_VIDEO_HEIGHT; h++)
    {
        // Set linebuffer pointers
        linebuffer[h] = framebuffer + h * VIDEO_SCREEN_W;
        // Clear framebuffer to black
        memset(linebuffer[h], 0, VIDEO_SCREEN_W);
    }

    video_init();
    sendFrameHalfResolution(linebuffer);
#endif
    sendPalette(pal_palette);
    return 0;
}

void DRIVER_DeInit_Video(void)
{
    if (framebuffer)
    {
        free(framebuffer);
        framebuffer = NULL;
    }
}
#endif /* ARDUINO_ARCH_ESP32 && PAL_ESP32_BACKEND_HW */
