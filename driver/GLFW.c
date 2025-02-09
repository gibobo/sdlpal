#include "driver.h"
#include <GLFW/glfw3.h>

int DRIVER_Init(void)
{
    if (!glfwInit())
        return -1;

    // Open the audio device.
    if (DRIVER_Init_Audio())
        return -1;

    if (DRIVER_Init_Video())
        return -1;

    if (DRIVER_Init_Event())
        return -1;

    return 0;
}

void DRIVER_DeInit(void)
{
    DRIVER_DeInit_Event();
    DRIVER_DeInit_Audio();
    DRIVER_DeInit_Video();
}
