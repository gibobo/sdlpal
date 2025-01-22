#include "driver.h"
#include "input/input.h"
#include <SDL.h>

int DRIVER_Init(void) {
  // Initialize SDL
#ifdef PAL_HAS_JOYSTICKS
  return SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE | SDL_INIT_JOYSTICK);
#else
  return SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE);
#endif
}

void DRIVER_DeInit(void) {
  SDL_Quit();
}
