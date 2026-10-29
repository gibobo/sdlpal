#ifndef _DRIVER_H_
#define _DRIVER_H_

#include <stdint.h>

int DRIVER_Init(void);
void DRIVER_DeInit(void);

void DRIVER_UpdatePalette(const uint8_t *rgPalette);
// Show the region of interest of a SCREEN_W x SCREEN_H frame. Without padding,
// screen row y shows frame row y inside the region and the rest of the screen
// keeps what it showed. With padding, screen row roi_y + k shows frame row k,
// so only roi_h rows are read from frame, and the rest of the screen is black.
void DRIVER_FrameShow(
    uint8_t *frame,
    const uint16_t roi_x,
    const uint16_t roi_y,
    const uint16_t roi_w,
    const uint16_t roi_h,
    const uint8_t padding_flag);

int DRIVER_Process_Events(void);

void DRIVER_Audio_Lock(void);
void DRIVER_Audio_Unlock(void);

#endif
