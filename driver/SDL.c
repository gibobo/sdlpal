#include "driver.h"
#include "global.h"
#include "input.h"
#include "mini_glloader.h"
#include "util.h"
#include "video.h"
#include "video_glsl.h"
#include <SDL.h>
#include <stdio.h>

#define PAL_HAS_JOYSTICKS

static SDL_Window *gpWindow = NULL;
static SDL_Renderer *gpRenderer = NULL;
static const int g_KeyMap[][2] = {
    {SDLK_UP, kKeyUp},
    {SDLK_KP_8, kKeyUp},
    {SDLK_DOWN, kKeyDown},
    {SDLK_KP_2, kKeyDown},
    {SDLK_LEFT, kKeyLeft},
    {SDLK_KP_4, kKeyLeft},
    {SDLK_RIGHT, kKeyRight},
    {SDLK_KP_6, kKeyRight},
    {SDLK_ESCAPE, kKeyMenu},
    {SDLK_INSERT, kKeyMenu},
    {SDLK_LALT, kKeyMenu},
    {SDLK_RALT, kKeyMenu},
    {SDLK_KP_0, kKeyMenu},
    {SDLK_RETURN, kKeySearch},
    {SDLK_SPACE, kKeySearch},
    {SDLK_KP_ENTER, kKeySearch},
    {SDLK_LCTRL, kKeySearch},
    {SDLK_PAGEUP, kKeyPgUp},
    {SDLK_KP_9, kKeyPgUp},
    {SDLK_PAGEDOWN, kKeyPgDn},
    {SDLK_KP_3, kKeyPgDn},
    {SDLK_HOME, kKeyHome},
    {SDLK_KP_7, kKeyHome},
    {SDLK_END, kKeyEnd},
    {SDLK_KP_1, kKeyEnd},
    {SDLK_r, kKeyRepeat},
    {SDLK_a, kKeyAuto},
    {SDLK_d, kKeyDefend},
    {SDLK_e, kKeyUseItem},
    {SDLK_w, kKeyThrowItem},
    {SDLK_q, kKeyFlee},
    {SDLK_f, kKeyForce},
    {SDLK_s, kKeyStatus}};

#ifdef PAL_HAS_JOYSTICKS
static SDL_Joystick *g_pJoy = NULL;
static int joystick_axis_X = 0;
static int joystick_axis_Y = 0;
#endif

static void SDL_UpdateKeyboardState(SDL_Keycode key)
/*++
Purpose:

Poll & update keyboard state.

Parameters:

None.

Return value:

None.

--*/
{
  static unsigned int rgdwKeyLastTime[sizeof(g_KeyMap) / sizeof(g_KeyMap[0])] = {0};
  const unsigned char *keyState = (const unsigned char *)SDL_GetKeyboardState(NULL);
  int i;
  unsigned int dwCurrentTime = UTIL_GetTicks();

  for (i = 0; i < sizeof(g_KeyMap) / sizeof(g_KeyMap[0]); i++) {
    SDL_Scancode keyCode = SDL_GetScancodeFromKey(g_KeyMap[i][0]);
    unsigned char keyPress = keyState[keyCode];

    if (keyPress == SDL_PRESSED) {
      if (dwCurrentTime > rgdwKeyLastTime[i]) {
        PAL_KeyDown(g_KeyMap[i][1], (rgdwKeyLastTime[i] != 0));
        if (gConfig.fEnableKeyRepeat) {
          rgdwKeyLastTime[i] = dwCurrentTime + (rgdwKeyLastTime[i] == 0 ? 200 : 75);
        } else {
          rgdwKeyLastTime[i] = 0xFFFFFFFF;
        }
      }
    } else {
      if (rgdwKeyLastTime[i] > 0) {
        PAL_KeyUp(g_KeyMap[i][1]);
        rgdwKeyLastTime[i] = 0;
      }
    }
  }
}

#ifdef PAL_HAS_JOYSTICKS
static void PAL_DetectJoystick(void)
/*++
    Purpose:
        Detect the joystick.

    Parameters:
        None.

    Return value:
        None.
--*/
{
  if (SDL_NumJoysticks() > 0) {
    int i;
    for (i = 0; i < SDL_NumJoysticks(); i++) {
      g_pJoy = SDL_JoystickOpen(i);
      if (g_pJoy != NULL) {
        SDL_JoystickEventState(SDL_ENABLE);
        break;
      }
    }
  } else {
    g_pJoy = NULL;
  }
}

static void SDL_UpdateJoyStickState(void)
/*++
    Purpose:
        Poll & update joystick state.

    Parameters:
        None.

    Return value:
        None.
--*/
{
  if (joystick_axis_X == 1 && joystick_axis_Y >= 0) {
    PAL_SetDirInput(kDirEast);
    PAL_SetKeyInput(PAL_GetKeyInput() | kKeyRight);
  } else if (joystick_axis_X == -1 && joystick_axis_Y <= 0) {
    PAL_SetDirInput(kDirWest);
    PAL_SetKeyInput(PAL_GetKeyInput() | kKeyLeft);
  } else if (joystick_axis_Y == 1 && joystick_axis_X <= 0) {
    PAL_SetDirInput(kDirSouth);
    PAL_SetKeyInput(PAL_GetKeyInput() | kKeyDown);
  } else if (joystick_axis_Y == -1 && joystick_axis_X >= 0) {
    PAL_SetDirInput(kDirNorth);
    PAL_SetKeyInput(PAL_GetKeyInput() | kKeyUp);
  } else {
    PAL_SetDirInput(kDirUnknown);
    PAL_SetKeyInput(kKeyNone);
  }
}
#endif

static int SDLCALL SDL_Event_Filter(const SDL_Event *lpEvent)

/*++
    Purpose:
        SDL event filter function. A filter to process all events.

    Parameters:
        [IN]  lpEvent - pointer to the event.

    Return value:
        1 = the event will be added to the internal queue.
        0 = the event will be dropped from the queue.
--*/
{
  switch (lpEvent->type) {
  case SDL_WINDOWEVENT:
    if (lpEvent->window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
      // resized the window
      VIDEO_Resize(lpEvent->window.data1, lpEvent->window.data2);
    }
    break;

  case SDL_APP_WILLENTERBACKGROUND:
    VIDEO_RenderPaused(TRUE);
    break;

  case SDL_APP_DIDENTERFOREGROUND:
    VIDEO_RenderPaused(FALSE);
    VIDEO_UpdateScreen(NULL);
    break;

  case SDL_KEYDOWN:
    // Pressed a key
    if ((lpEvent->key.keysym.mod & KMOD_ALT) && (lpEvent->key.keysym.sym == SDLK_F4)) {
      // Pressed Alt+F4 (Exit program)...
      return -1;
    }
  case SDL_KEYUP:
    SDL_UpdateKeyboardState(lpEvent->key.keysym.sym);
    break;

#ifdef PAL_HAS_JOYSTICKS
  case SDL_JOYDEVICEADDED:
  case SDL_JOYDEVICEREMOVED:
    PAL_DetectJoystick();
    break;
  case SDL_JOYAXISMOTION:
    // Moved an axis on joystick
    switch (lpEvent->jaxis.axis) {
    case 0:
      joystick_axis_X = (lpEvent->jaxis.value > 3200) ? (1) : ((lpEvent->jaxis.value < -3200) ? (-1) : (0));
      break;

    case 1:
      joystick_axis_Y = (lpEvent->jaxis.value > 3200) ? (1) : ((lpEvent->jaxis.value < -3200) ? (-1) : (0));
      break;
    }
    SDL_UpdateJoyStickState();
    break;

  case SDL_JOYHATMOTION:
    // Pressed the joystick hat button
    switch (lpEvent->jhat.value) {
    case SDL_HAT_LEFT:
    case SDL_HAT_LEFTUP:
      PAL_SetDirInput(kDirWest);
      PAL_SetKeyInput(kKeyLeft);
      break;

    case SDL_HAT_RIGHT:
    case SDL_HAT_RIGHTDOWN:
      PAL_SetDirInput(kDirEast);
      PAL_SetKeyInput(kKeyRight);
      break;

    case SDL_HAT_UP:
    case SDL_HAT_RIGHTUP:
      PAL_SetDirInput(kDirNorth);
      PAL_SetKeyInput(kKeyUp);
      break;

    case SDL_HAT_DOWN:
    case SDL_HAT_LEFTDOWN:
      PAL_SetDirInput(kDirSouth);
      PAL_SetKeyInput(kKeyDown);
      break;

    case SDL_HAT_CENTERED:
      PAL_SetDirInput(kDirUnknown);
      PAL_SetKeyInput(kKeyNone);
      break;
    }
    break;

  case SDL_JOYBUTTONDOWN:
    // Pressed the joystick button
    switch (lpEvent->jbutton.button & 1) {
    case 0:
      PAL_SetKeyInput(PAL_GetKeyInput() | kKeyMenu);
      break;

    case 1:
      PAL_SetKeyInput(PAL_GetKeyInput() | kKeySearch);
      break;
    }
    break;
#endif

  case SDL_QUIT:
    // clicked on the close button of the window. Quit immediately.
    return -1;
  }
  // All events are handled here; don't put anything to the internal queue
  return 0;
}

int DRIVER_Init(void) {
  int w = 320;
  int h = 200;
//   int w = gConfig.dwTextureWidth;
//   int h = gConfig.dwTextureHeight;

// Initialize SDL
#ifdef PAL_HAS_JOYSTICKS
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE | SDL_INIT_JOYSTICK))
#else
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE))
#endif
    return -1;
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

  // Check for joystick
  // MUST FOR PLATFORMS THAT DOES NOT SUPPORT JOYSTICKS HOTPLUG
#ifdef PAL_HAS_JOYSTICKS
  PAL_DetectJoystick();
#endif

  // Open the audio device.
  if (DRIVER_Init_Audio())
    return -1;

  return 0;
}

void DRIVER_DeInit(void) {
  if (gpRenderer) {
    SDL_DestroyRenderer(gpRenderer);
  }
  gpRenderer = NULL;

  if (gpWindow) {
    SDL_DestroyWindow(gpWindow);
  }
  gpWindow = NULL;

#ifdef PAL_HAS_JOYSTICKS
  if (g_pJoy != NULL) {
    SDL_JoystickClose(g_pJoy);
    g_pJoy = NULL;
  }
#endif

  DRIVER_DeInit_Audio();
  SDL_Quit();
}

void DRIVER_FrameShow(unsigned char *frame_rgb) {
  VIDEO_GLSL_RenderCopy(frame_rgb);
  SDL_GL_SwapWindow(gpWindow);
}

void DRIVER_FrameResize(unsigned int width, unsigned int height) {
  VIDEO_GLSL_Resize(width, height);
}

int DRIVER_ProcessEvent(void)
/*++
Purpose:

Process all events.

Parameters:

None.

Return value:

None.

--*/
{
  int res = 0;
  SDL_Event evt;
  while (SDL_PollEvent(&evt)) {
    res = SDL_Event_Filter(&evt);
  }
  return res;
}

void *DRIVER_fopen(const char *_FileName, const char *_Mode) {
  if (_FileName == NULL || _Mode == NULL)
    TerminateOnError("DRIVER_fopen() called with invalid parameters\n");

  return fopen(_FileName, _Mode);
}

int DRIVER_fseek(void *_Stream, long _Offset, int _Origin) {
  if (_Stream == NULL)
    TerminateOnError("DRIVER_fseek() called with invalid parameters\n");

  return fseek(_Stream, _Offset, _Origin);
}

unsigned int DRIVER_fread(void *_Buffer, unsigned int _ElementSize, unsigned int _ElementCount, void *_Stream) {
  if (_Buffer == NULL || _Stream == NULL)
    TerminateOnError("DRIVER_fread() called with invalid parameters\n");

  return (unsigned int)fread(_Buffer, _ElementSize, _ElementCount, _Stream);
}

unsigned int DRIVER_fwrite(void *_Buffer, unsigned int _ElementSize, unsigned int _ElementCount, void *_Stream) {
  if (_Buffer == NULL || _Stream == NULL)
    TerminateOnError("DRIVER_fread() called with invalid parameters\n");

  return (unsigned int)fwrite(_Buffer, _ElementSize, _ElementCount, _Stream);
}

void DRIVER_fclose(void *fp) {
  if (fp != NULL) {
    fclose(fp);
  }
}
