#ifndef _DRIVER_H_
#define _DRIVER_H_

#ifdef __cplusplus
extern "C" {
#endif

int DRIVER_Init(void);
void DRIVER_DeInit(void);
void DRIVER_FrameShow(unsigned char *frame_rgb);
void DRIVER_FrameResize(unsigned int width, unsigned int height);
int DRIVER_ProcessEvent(void);

#ifdef __cplusplus
}
#endif

#endif
