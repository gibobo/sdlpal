#include "DrvIf_internal.h"
#include <SDL.h>

int DRIVER_Init(void)
{
// Initialize SDL
#ifdef PAL_HAS_JOYSTICKS
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE | SDL_INIT_JOYSTICK) < 0)
#else
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE) < 0)
#endif
        return -1;

    // Open the audio device.
    if (DRIVER_Init_Audio())
        return -1;

    if (DRIVER_Init_Video())
        return -1;

    if (DRIVER_Init_Event())
        return -1;

    return 0;
}

void DRIVER_DeInit(void)
{
    DRIVER_DeInit_Event();
    DRIVER_DeInit_Audio();
    DRIVER_DeInit_Video();
    SDL_Quit();
}
