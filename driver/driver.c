#include "driver.h"
#include "input.h"
#include "util.h"

#ifndef NULL
#define NULL 0
#endif

int DRIVER_Audio(void) { return 0; }

void DRIVER_DeInit_Audio(void) {}

void DRIVER_Audio_Lock(void) {}

void DRIVER_Audio_Unlock(void) {}

int DRIVER_Init(void) { return 0; }

void DRIVER_DeInit(void) {}

void DRIVER_FrameShow(unsigned char *frame_rgb) {}

void DRIVER_FrameResize(unsigned int width, unsigned int height) {}

int DRIVER_Process_Events(void) { return 0; }
