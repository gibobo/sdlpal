#include "../backend_select.h"
#if defined(ARDUINO_ARCH_ESP32) && PAL_ESP32_BACKEND_WEB
#include "../../driver.h"
#include "DrvIf_internal.h"

int DRIVER_Init(void)
{
    // Open the audio device (starts audio_task on Core 1, MAX-2 priority).
    if (DRIVER_Init_Audio())
        return -1;
    // Open the video device (allocates double framebuffer + fb_mutex).
    if (DRIVER_Init_Video())
        return -1;
    // Open the event device (creates input_queue, starts WebServer, waits for client).
    if (DRIVER_Init_Event())
        return -1;
    // Start web server task on Core 1 (lower priority than audio).
    // Must be called after DRIVER_Init_Event() so WebServer::begin() has run.
    if (DRIVER_Init_WebTask())
        return -1;

    return 0;
}

void DRIVER_DeInit(void)
{
    // Stop web task before tearing down the web server and buffers.
    DRIVER_DeInit_WebTask();
    // Close the event device (stops WebServer, deletes input_queue).
    DRIVER_DeInit_Event();
    // Close the audio device.
    DRIVER_DeInit_Audio();
    // Close the video device (frees buffers, deletes fb_mutex).
    DRIVER_DeInit_Video();
}
#endif /* ARDUINO_ARCH_ESP32 && PAL_ESP32_BACKEND_WEB */
