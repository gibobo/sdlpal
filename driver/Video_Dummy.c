int DRIVER_Init_Video(void) { return 0; }

void DRIVER_DeInit_Video(void) {}

void DRIVER_UpdatePalette(const unsigned char *rgPalette) {}

void DRIVER_FrameShow(
    unsigned char *frame,
    const unsigned short roi_x,
    const unsigned short roi_y,
    const unsigned short roi_w,
    const unsigned short roi_h,
    const unsigned char padding_flag) {}
