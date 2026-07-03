#include "../backend_select.h"
#if defined(ARDUINO_ARCH_ESP32) && PAL_ESP32_BACKEND_HW
#include "../../driver.h"
#include "DrvIf_internal.h"

int DRIVER_Init(void)
{
    // Open the audio device.
    if (DRIVER_Init_Audio())
        return -1;
    // Open the video device.
    if (DRIVER_Init_Video())
        return -1;
    // Open the event device.
    if (DRIVER_Init_Event())
        return -1;

    return 0;
}

void DRIVER_DeInit(void)
{
    // Close the event device.
    DRIVER_DeInit_Event();
    // Close the audio device.
    DRIVER_DeInit_Audio();
    // Close the video device.
    DRIVER_DeInit_Video();
}
#endif /* ARDUINO_ARCH_ESP32 && PAL_ESP32_BACKEND_HW */
