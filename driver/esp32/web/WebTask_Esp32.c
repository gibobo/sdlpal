#include "../backend_select.h"
#if defined(ARDUINO_ARCH_ESP32) && PAL_ESP32_BACKEND_WEB
#include "DrvIf_internal.h"
#include "utils/esp32_webserver.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include <stdbool.h>

// Web server task runs on Core 1 at lower priority than the audio task (MAX-2).
// It continuously polls the Arduino WebServer for incoming /frame and /input requests.
// This keeps the game loop on Core 0 free from TCP blocking.
#define WEB_TASK_STACK_SIZE (8192)
#define WEB_TASK_PRIORITY   (configMAX_PRIORITIES - 4)  // below audio (MAX-2)
#define WEB_TASK_CORE       (1)

static TaskHandle_t web_task_handle = NULL;
static volatile bool web_task_running = false;

static void web_server_task(void *arg)
{
    (void)arg;
    printf("Web task started on Core %d\n", xPortGetCoreID());
    web_task_running = true;

    while (web_task_running)
    {
        DRIVER_ProcessWebServer();
        // Yield for 1 ms so the higher-priority audio task can preempt freely.
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    printf("Web task exiting...\n");
    vTaskDelete(NULL);
}

int DRIVER_Init_WebTask(void)
{
    if (xTaskCreatePinnedToCore(
            web_server_task,
            "web_task",
            WEB_TASK_STACK_SIZE,
            NULL,
            WEB_TASK_PRIORITY,
            &web_task_handle,
            WEB_TASK_CORE) != pdPASS)
    {
        printf("Failed to create web task\n");
        return -1;
    }
    return 0;
}

void DRIVER_DeInit_WebTask(void)
{
    web_task_running = false;

    if (web_task_handle != NULL)
    {
        // Give the task a moment to observe web_task_running=false and exit,
        // then force-delete if still alive.
        vTaskDelay(pdMS_TO_TICKS(50));
        vTaskDelete(web_task_handle);
        web_task_handle = NULL;
    }
}
#endif /* ARDUINO_ARCH_ESP32 && PAL_ESP32_BACKEND_WEB */
