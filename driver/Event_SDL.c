#include "../src/global.h"
#include "../src/input.h"
#include "../src/util.h"
#include "../src/video.h"
#include "DrvIf_internal.h"
#include <SDL_events.h>
#include <stdio.h>

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
extern void DRIVER_FrameResize(unsigned int width, unsigned int height);

static void SDL_UpdateKeyboardState(SDL_Keycode key)
{
    static unsigned int rgdwKeyLastTime[sizeof(g_KeyMap) / sizeof(g_KeyMap[0])] = {0};
    const unsigned char *keyState = (const unsigned char *)SDL_GetKeyboardState(NULL);
    int i;
    unsigned int dwCurrentTime = UTIL_GetTicks();

    for (i = 0; i < sizeof(g_KeyMap) / sizeof(g_KeyMap[0]); i++)
    {
        SDL_Scancode keyCode = SDL_GetScancodeFromKey(g_KeyMap[i][0]);
        unsigned char keyPress = keyState[keyCode];

        if (keyPress == SDL_PRESSED)
        {
            if (dwCurrentTime > rgdwKeyLastTime[i])
            {
                PAL_KeyDown(g_KeyMap[i][1], (rgdwKeyLastTime[i] != 0));
                rgdwKeyLastTime[i] = 0xFFFFFFFF;
            }
        }
        else
        {
            if (rgdwKeyLastTime[i] > 0)
                PAL_KeyUp(g_KeyMap[i][1]);
            rgdwKeyLastTime[i] = 0;
        }
    }
}

#ifdef PAL_HAS_JOYSTICKS
static void PAL_DetectJoystick(void)
{
    if (SDL_NumJoysticks() > 0)
    {
        int i;
        for (i = 0; i < SDL_NumJoysticks(); i++)
        {
            g_pJoy = SDL_JoystickOpen(i);
            if (g_pJoy != NULL)
            {
                SDL_JoystickEventState(SDL_ENABLE);
                break;
            }
        }
    }
    else
    {
        g_pJoy = NULL;
    }
}

static void SDL_UpdateJoyStickState(void)
{
    if (joystick_axis_X == 1 && joystick_axis_Y >= 0)
    {
        PAL_SetDirInput(kDirEast);
        PAL_SetKeyInput(PAL_GetKeyInput() | kKeyRight);
    }
    else if (joystick_axis_X == -1 && joystick_axis_Y <= 0)
    {
        PAL_SetDirInput(kDirWest);
        PAL_SetKeyInput(PAL_GetKeyInput() | kKeyLeft);
    }
    else if (joystick_axis_Y == 1 && joystick_axis_X <= 0)
    {
        PAL_SetDirInput(kDirSouth);
        PAL_SetKeyInput(PAL_GetKeyInput() | kKeyDown);
    }
    else if (joystick_axis_Y == -1 && joystick_axis_X >= 0)
    {
        PAL_SetDirInput(kDirNorth);
        PAL_SetKeyInput(PAL_GetKeyInput() | kKeyUp);
    }
    else
    {
        PAL_SetDirInput(kDirUnknown);
        PAL_SetKeyInput(kKeyNone);
    }
}
#endif

static int SDLCALL SDL_Event_Filter(const SDL_Event *lpEvent)
{
    switch (lpEvent->type)
    {
        case SDL_WINDOWEVENT:
            if (lpEvent->window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
            {
                // resized the window
                DRIVER_FrameResize(lpEvent->window.data1, lpEvent->window.data2);
            }
            break;

        case SDL_APP_WILLENTERBACKGROUND:
            VIDEO_RenderPaused(1);
            break;

        case SDL_APP_DIDENTERFOREGROUND:
            VIDEO_RenderPaused(0);
            VIDEO_UpdateScreen(NULL);
            break;

        case SDL_KEYDOWN:
            // Pressed a key
            if ((lpEvent->key.keysym.mod & KMOD_ALT) && (lpEvent->key.keysym.sym == SDLK_F4))
            {
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
            switch (lpEvent->jaxis.axis)
            {
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
            switch (lpEvent->jhat.value)
            {
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
            switch (lpEvent->jbutton.button & 1)
            {
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

int DRIVER_Process_Events(void)
{
    int res = 0;
    SDL_Event evt;
    while (SDL_PollEvent(&evt))
    {
        res = SDL_Event_Filter(&evt);
    }
    return res;
}

int DRIVER_Init_Event(void)
{
    // Check for joystick
    // MUST FOR PLATFORMS THAT DOES NOT SUPPORT JOYSTICKS HOTPLUG
#ifdef PAL_HAS_JOYSTICKS
    PAL_DetectJoystick();
#endif
    return 0;
}

void DRIVER_DeInit_Event(void)
{
#ifdef PAL_HAS_JOYSTICKS
    if (g_pJoy != NULL)
    {
        SDL_JoystickClose(g_pJoy);
        g_pJoy = NULL;
    }
#endif
}
