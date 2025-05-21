int DRIVER_Init(void) { return 0; }

void DRIVER_DeInit(void) {}

void DRIVER_FrameResize(unsigned int width, unsigned int height) {}

void DRIVER_UpdatePalette(const unsigned char *rgPalette) {}

void DRIVER_FrameShow(
    unsigned char *frame,
    const unsigned short roi_x,
    const unsigned short roi_y,
    const unsigned short roi_w,
    const unsigned short roi_h,
    const unsigned char padding_flag) {}

int DRIVER_Process_Events(void) { return 0; }

void DRIVER_Audio_Lock(void) {}

void DRIVER_Audio_Unlock(void) {}
