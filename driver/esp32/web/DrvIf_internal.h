#ifndef _DRVIF_INTERNAL_WEB_H_
#define _DRVIF_INTERNAL_WEB_H_

int DRIVER_Init_Video(void);
void DRIVER_DeInit_Video(void);
void DRIVER_SwapFramebuffer(void);

int DRIVER_Init_Event(void);
void DRIVER_DeInit_Event(void);

int DRIVER_Init_Audio(void);
void DRIVER_DeInit_Audio(void);

int DRIVER_Init_WebTask(void);
void DRIVER_DeInit_WebTask(void);

#endif
