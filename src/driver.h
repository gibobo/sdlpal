#ifndef _DRIVER_H_
#define _DRIVER_H_

#include <stdint.h>

int DRIVER_Init(void);
void DRIVER_DeInit(void);

void DRIVER_UpdatePalette(const uint8_t *rgPalette);
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
