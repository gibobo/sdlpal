#include "video.h"
#include "video_glsl.h"
#include <SDL.h>

static SDL_Window *gpWindow = NULL;
static SDL_GLContext gpContext = NULL;
static int window_width = 320;
static int window_height = 200;

void DRIVER_FrameShow(unsigned char *frame_rgb) {
    VIDEO_GLSL_RenderCopy(frame_rgb);
    SDL_GL_SwapWindow(gpWindow);
}

void DRIVER_FrameResize(unsigned int width, unsigned int height) {
    window_width = width;
    window_height = height;
    VIDEO_GLSL_Setup(window_width, window_height);
}

int DRIVER_Init_Video(void) {
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengles2");
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

    // Before we can render anything, we need a window and a renderer.
    gpWindow = SDL_CreateWindow(
        "PAL",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        window_width, window_height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
    if (gpWindow == NULL)
        return -1;

    //Creates OpenGL context
    gpContext = SDL_GL_CreateContext(gpWindow);
    if (gpContext == NULL) {
        SDL_DestroyWindow(gpWindow);
        gpWindow = NULL;
        return -1;
    }
    SDL_GL_MakeCurrent(gpWindow, gpContext);
    SDL_GL_SetSwapInterval(1); // Enable vsync

    VIDEO_GLSL_Setup(window_width, window_height);
    return 0;
}

void DRIVER_DeInit_Video(void) {
    if (gpContext)
        SDL_GL_DeleteContext(gpContext);
    if (gpWindow)
        SDL_DestroyWindow(gpWindow);

    gpContext = NULL;
    gpWindow = NULL;
}
