#include "../src/driver.h"
#include "DrvIf_internal.h"
#include <stdint.h>

int DRIVER_Init_Video(void) { return 0; }

void DRIVER_DeInit_Video(void) {}

void DRIVER_UpdatePalette(const uint8_t *rgPalette) { (void)rgPalette; }

void DRIVER_FrameShow(
    uint8_t *frame,
    const uint16_t roi_x,
    const uint16_t roi_y,
    const uint16_t roi_w,
    const uint16_t roi_h,
    const uint8_t padding_flag)
{
    (void)frame;
    (void)roi_x;
    (void)roi_y;
    (void)roi_w;
    (void)roi_h;
    (void)padding_flag;
}
