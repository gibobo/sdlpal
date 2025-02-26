#ifndef _DRIVER_H_
#define _DRIVER_H_

int DRIVER_Init(void);
void DRIVER_DeInit(void);

int DRIVER_Init_Audio(void);
void DRIVER_DeInit_Audio(void);
void DRIVER_Audio_Lock(void);
void DRIVER_Audio_Unlock(void);

int DRIVER_Init_Video(void);
void DRIVER_DeInit_Video(void);
unsigned char *DRIVER_FrameBuffer();

void DRIVER_FrameShow(
    unsigned char *frame,
    const unsigned char *palette,
    const unsigned short roi_x,
    const unsigned short roi_y,
    const unsigned short roi_w,
    const unsigned short roi_h,
    const unsigned char padding_flag);
void DRIVER_FrameResize(unsigned int width, unsigned int height);

int DRIVER_Init_Event(void);
void DRIVER_DeInit_Event(void);
int DRIVER_Process_Events(void);

#endif
