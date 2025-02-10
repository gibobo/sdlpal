#ifndef _DRIVER_H_
#define _DRIVER_H_

#ifdef __cplusplus
extern "C" {
#endif

int DRIVER_Init(void);
void DRIVER_DeInit(void);

int DRIVER_Init_Audio(void);
void DRIVER_DeInit_Audio(void);
void DRIVER_Audio_Lock(void);
void DRIVER_Audio_Unlock(void);

int DRIVER_Init_Video(void);
void DRIVER_DeInit_Video(void);
void DRIVER_FrameShow(unsigned char *frame_rgb);
void DRIVER_FrameResize(unsigned int width, unsigned int height);

int DRIVER_Init_Event(void);
void DRIVER_DeInit_Event(void);
int DRIVER_Process_Events(void);

#ifdef __cplusplus
}
#endif

#endif
