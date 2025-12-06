#include "../src/driver.h"
#include "DrvIf_internal.h"

int DRIVER_Init_Video(void) { return 0; }

void DRIVER_DeInit_Video(void) {}

void DRIVER_UpdatePalette(const unsigned char *rgPalette) { (void)rgPalette; }

void DRIVER_FrameShow(
    unsigned char *frame,
    const unsigned short roi_x,
    const unsigned short roi_y,
    const unsigned short roi_w,
    const unsigned short roi_h,
    const unsigned char padding_flag)
{
    (void)frame;
    (void)roi_x;
    (void)roi_y;
    (void)roi_w;
    (void)roi_h;
    (void)padding_flag;
}
