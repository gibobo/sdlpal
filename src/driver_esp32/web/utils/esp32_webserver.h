#ifndef _ESP32_WEBSERVER_H_
#define _ESP32_WEBSERVER_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

int DRIVER_InitWebServer(void);
void DRIVER_DeInitWebServer(void);
void DRIVER_ProcessWebServer(void);
void DRIVER_TransmitVideoFrame(const uint8_t *frame, const uint8_t *palette);
void DRIVER_MarkPaletteUpdate(void);

// Called from web task (Core 1): snapshot read buffer index + stable palette,
// set fb_read_locked. Returns the buffer index to pass to DRIVER_Web_GetFrameBuffer().
int DRIVER_Web_BeginRead(uint8_t *local_pal_out);

// Called from web task after TCP write: release fb_read_locked.
void DRIVER_Web_EndRead(void);

// Returns pointer to framebuffer at given index (use index from DRIVER_Web_BeginRead).
const uint8_t *DRIVER_Web_GetFrameBuffer(int idx);

// Called from web task (Core 1) to push a key event onto the input queue.
void DRIVER_WebInputQueue_Push(const char *key, const char *type);

#ifdef __cplusplus
}
#endif

#endif
