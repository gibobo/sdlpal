#include "../src/util.h"
#include "../src/video.h"
#include "utils/video_glsl.h"
#include <SDL.h>
#include <string.h>

static int window_width = 320;
static int window_height = 200;
static unsigned char *framebuffer = NULL; // RGB888
static const unsigned char *palette = NULL;
static SDL_Window *gpWindow = NULL;
static SDL_GLContext gpContext = NULL;

unsigned char *DRIVER_FrameBuffer()
{
    return framebuffer;
}

void DRIVER_FrameShow(
    unsigned char *frame,
    const unsigned short roi_x,
    const unsigned short roi_y,
    const unsigned short roi_w,
    const unsigned short roi_h,
    const unsigned char padding_flag)
{

    unsigned short x, y;
    unsigned short roi_x2 = roi_x + roi_w;
    unsigned short roi_y2 = roi_y + roi_h;
    unsigned char *src = frame;
    unsigned char *dst = DRIVER_FrameBuffer();

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

    VIDEO_GLSL_RenderCopy(framebuffer);
    SDL_GL_SwapWindow(gpWindow);
}

void DRIVER_FrameResize(unsigned int width, unsigned int height)
{
    window_width = width;
    window_height = height;
    VIDEO_GLSL_Initialize(window_width, window_height);
}

void DRIVER_UpdatePalette(const unsigned char *rgPalette) { palette = rgPalette; }

int DRIVER_Init_Video(void)
{
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengles2");
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    // Before we can render anything, we need a window and a renderer.
    gpWindow = SDL_CreateWindow(
        "SDLPAL",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        window_width, window_height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
    if (gpWindow == NULL)
        return -1;

    //Creates OpenGL context
    gpContext = SDL_GL_CreateContext(gpWindow);
    if (gpContext == NULL)
    {
        SDL_DestroyWindow(gpWindow);
        gpWindow = NULL;
        return -1;
    }
    SDL_GL_MakeCurrent(gpWindow, gpContext);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    framebuffer = (unsigned char *)UTIL_malloc(SCREEN_SIZE * 3);
    VIDEO_GLSL_Initialize(window_width, window_height);
    return 0;
}

void DRIVER_DeInit_Video(void)
{
    VIDEO_GLSL_Destroy();
    UTIL_free(framebuffer);
    if (gpContext)
        SDL_GL_DeleteContext(gpContext);
    if (gpWindow)
        SDL_DestroyWindow(gpWindow);

    gpContext = NULL;
    gpWindow = NULL;
    framebuffer = NULL;
}
