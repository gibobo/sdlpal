
#include "mini_glloader.h"
#include "video.h"
#include "video_glsl.h"
#include <SDL.h>

static SDL_Window *gpWindow = NULL;
static SDL_Renderer *gpRenderer = NULL;

void DRIVER_FrameShow(unsigned char *frame_rgb) {
  VIDEO_GLSL_RenderCopy(frame_rgb);
  SDL_GL_SwapWindow(gpWindow);
}

void DRIVER_FrameResize(unsigned int width, unsigned int height) {
  VIDEO_GLSL_Resize(width, height);
}

int DRIVER_Init_Video(void) {
  int w = 320;
  int h = 200;
  //   int w = gConfig.dwTextureWidth;
  //   int h = gConfig.dwTextureHeight;

  SDL_RendererInfo rendererInfo;
#ifdef GLES
  SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengles2");
#if SDL_VIDEO_OPENGL_EGL && (SDL_VIDEO_DRIVER_EMSCRIPTEN || SDL_VIDEO_DRIVER_WINRT)
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif
#else
  SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
#endif

  // Before we can render anything, we need a window and a renderer.
  gpWindow = SDL_CreateWindow(NULL,
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              w, h,
                              SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
  if (gpWindow == NULL) {
    return -1;
  }

  SDL_SetWindowTitle(gpWindow, "PAL");
  gpRenderer = SDL_CreateRenderer(gpWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

  if (gpRenderer == NULL) {
    return -1;
  }

  SDL_GetRendererInfo(gpRenderer, &rendererInfo);
  SDL_GetRendererOutputSize(gpRenderer, &w, &h);
  VIDEO_GLSL_Setup(rendererInfo.name);
  VIDEO_GLSL_Resize(w, h);
  return 0;
}

void DRIVER_DeInit_Video(void) {
  if (gpRenderer) {
    SDL_DestroyRenderer(gpRenderer);
  }
  gpRenderer = NULL;

  if (gpWindow) {
    SDL_DestroyWindow(gpWindow);
  }
  gpWindow = NULL;
}