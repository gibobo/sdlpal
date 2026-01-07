#include "../src/driver.h"
#include "../src/util.h"
#include "../src/video.h"
#include "DrvIf_internal.h"
#include <SDL.h>
#include <stdint.h>
#include <string.h>

static int window_width = 320;
static int window_height = 200;
static uint8_t *framebuffer = NULL; // RGB888
static uint8_t *palette = NULL;
static SDL_Window *gpWindow = NULL;
static SDL_Renderer *gpRenderer = NULL;
static SDL_Texture *gpTexture = NULL;

// Function prototypes
void DRIVER_FrameResize(uint32_t width, uint32_t height);

void DRIVER_FrameShow(
    uint8_t *frame,
    const uint16_t roi_x,
    const uint16_t roi_y,
    const uint16_t roi_w,
    const uint16_t roi_h,
    const uint8_t padding_flag)
{
    if (!gpRenderer || !gpTexture || framebuffer == NULL || palette == NULL)
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
                    dst[x * 3 + 0] = palette[src[x] * 3 + 0];
                    dst[x * 3 + 1] = palette[src[x] * 3 + 1];
                    dst[x * 3 + 2] = palette[src[x] * 3 + 2];
                }
                else if (padding_flag)
                {
                    dst[x * 3 + 0] = 0;
                    dst[x * 3 + 1] = 0;
                    dst[x * 3 + 2] = 0;
                }
            }
            src += SCREEN_W;
        }
        else if (padding_flag)
            memset(dst, 0, SCREEN_W * 3);
        else
            src += SCREEN_W;
        dst += SCREEN_W * 3;
    }

    // Update texture with framebuffer data
    SDL_UpdateTexture(gpTexture, NULL, framebuffer, SCREEN_W * 3);

    // Clear and render
    SDL_SetRenderDrawColor(gpRenderer, 0, 0, 0, 255);
    SDL_RenderClear(gpRenderer);
    SDL_RenderCopy(gpRenderer, gpTexture, NULL, NULL);
    SDL_RenderPresent(gpRenderer);
}

void DRIVER_FrameResize(uint32_t width, uint32_t height)
{
    window_width = (int)width;
    window_height = (int)height;
    // Renderer automatically handles scaling
}

void DRIVER_UpdatePalette(const uint8_t *rgPalette)
{
    if (rgPalette)
        memcpy(palette, rgPalette, 256 * 3);
}

int DRIVER_Init_Video(void)
{
    // Always use software rendering to avoid driver issues
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");

    // Create window
    gpWindow = SDL_CreateWindow(
        "SDLPAL",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        window_width, window_height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    if (gpWindow == NULL)
        return -1;

    // Create renderer
    gpRenderer = SDL_CreateRenderer(gpWindow, -1, SDL_RENDERER_SOFTWARE);
    if (gpRenderer == NULL)
    {
        SDL_DestroyWindow(gpWindow);
        gpWindow = NULL;
        return -1;
    }

    // Create texture for framebuffer
    gpTexture = SDL_CreateTexture(gpRenderer,
                                  SDL_PIXELFORMAT_RGB24,
                                  SDL_TEXTUREACCESS_STREAMING,
                                  SCREEN_W, SCREEN_H);

    if (gpTexture == NULL)
    {
        SDL_DestroyRenderer(gpRenderer);
        SDL_DestroyWindow(gpWindow);
        gpRenderer = NULL;
        gpWindow = NULL;
        return -1;
    }

    framebuffer = (uint8_t *)UTIL_malloc(SCREEN_SIZE * 3);
    palette = (uint8_t *)UTIL_malloc(256 * 3);
    return 0;
}

void DRIVER_DeInit_Video(void)
{
    UTIL_free(framebuffer);
    UTIL_free(palette);
    if (gpTexture)
        SDL_DestroyTexture(gpTexture);
    if (gpRenderer)
        SDL_DestroyRenderer(gpRenderer);
    if (gpWindow)
        SDL_DestroyWindow(gpWindow);

    framebuffer = NULL;
    palette = NULL;
    gpTexture = NULL;
    gpRenderer = NULL;
    gpWindow = NULL;
}
