#ifndef _DRIIF_INTERNAL_H_
#define _DRIIF_INTERNAL_H_

int DRIVER_Init_Video(void);
void DRIVER_DeInit_Video(void);
unsigned char *DRIVER_FrameBuffer(void);

int DRIVER_Init_Event(void);
void DRIVER_DeInit_Event(void);

int DRIVER_Init_Audio(void);
void DRIVER_DeInit_Audio(void);

#endif
