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

int DRIVER_Init_Audio(void);
void DRIVER_DeInit_Audio(void);

// Platform-specific utilities
void *DRIVER_fopen(const char *_FileName, const char *_Mode);
int DRIVER_fseek(void *_Stream, long _Offset, int _Origin);
unsigned int DRIVER_fread(void *_Buffer, unsigned int _ElementSize, unsigned int _ElementCount, void *_Stream);
unsigned int DRIVER_fwrite(void *_Buffer, unsigned int _ElementSize, unsigned int _ElementCount, void *_Stream);
void DRIVER_fclose(void *fp);

#ifdef __cplusplus
}
#endif

#endif
