#include "driver.h"
#include "src/input.h"
#include "src/util.h"
#include "src/video.h"

#ifndef NULL
#define NULL 0
#endif

int DRIVER_Audio(void) { return 0; }

void DRIVER_DeInit_Audio(void) {}

void DRIVER_Audio_Lock(void) {}

void DRIVER_Audio_Unlock(void) {}

int DRIVER_Init(void) { return 0; }

void DRIVER_DeInit(void) {}

unsigned char *DRIVER_FrameBuffer() { return NULL; }

void DRIVER_FrameShow(
    unsigned char *frame,
    const unsigned char *palette,
    const unsigned short roi_x,
    const unsigned short roi_y,
    const unsigned short roi_w,
    const unsigned short roi_h,
    const unsigned char padding_flag) {}

void DRIVER_FrameResize(unsigned int width, unsigned int height) {}

int DRIVER_Process_Events(void) { return 0; }
