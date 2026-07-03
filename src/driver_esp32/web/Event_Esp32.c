#include "../backend_select.h"
#if defined(ARDUINO_ARCH_ESP32) && PAL_ESP32_BACKEND_WEB
#include "../../driver.h"
#include "../../input.h"
#include "DrvIf_internal.h"
#include "utils/esp32_webserver.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <string.h>
#include <stdio.h>

// ===== Key mapping from web buttons to game keys =====
typedef struct
{
    const char *name;
    PALKEY keycode;
} KeyMapEntry;

static const KeyMapEntry key_map[] = {
    // D-pad
    {"up", kKeyUp},
    {"down", kKeyDown},
    {"left", kKeyLeft},
    {"right", kKeyRight},
    // Face buttons
    {"A", kKeySearch},      // Confirm
    {"B", kKeyMenu},        // Menu/Cancel
    {"X", kKeyPgUp},        // Previous page
    {"Y", kKeyPgDn},        // Next page
    // Meta buttons
    {"Start", kKeyForce},  // Repeat
    {"Select", kKeyAuto},   // Auto
};

static PALKEY get_keycode_for_name(const char *name)
{
    if (!name)
        return kKeyNone;

    for (size_t i = 0; i < sizeof(key_map) / sizeof(key_map[0]); i++)
    {
        if (strcmp(name, key_map[i].name) == 0)
            return key_map[i].keycode;
    }
    return kKeyNone;
}

// ===== Input queue: Web task (Core 1) → Game loop (Core 0) =====
typedef struct
{
    PALKEY  keycode;
    uint8_t is_down;  // 1 = key down, 0 = key up/cancel
} WebInputEvent;

QueueHandle_t input_queue = NULL;

// Called from web task (Core 1) when a /input request arrives.
// Resolves the key name and pushes the event onto input_queue (non-blocking).
void DRIVER_WebInputQueue_Push(const char *key, const char *type)
{
    if (!key || !type)
        return;

    PALKEY keycode = get_keycode_for_name(key);
    if (keycode == kKeyNone)
    {
        printf("[WEBINPUT] unknown key: %s\n", key);
        return;
    }

    printf("[WEBINPUT] key=%s type=%s\n", key, type);

    WebInputEvent evt;
    evt.keycode = keycode;
    evt.is_down = (strcmp(type, "down") == 0) ? 1 : 0;

    // Non-blocking: if queue is full, drop the event rather than block the web task
    xQueueSend(input_queue, &evt, 0);
}

int DRIVER_Init_Event(void)
{
    // Create input queue before starting web server so it is ready when
    // the first /input request arrives.
    input_queue = xQueueCreate(16, sizeof(WebInputEvent));
    if (input_queue == NULL)
        return -1;

    DRIVER_InitWebServer();
    return 0;
}

void DRIVER_DeInit_Event(void)
{
    DRIVER_DeInitWebServer();

    if (input_queue)
    {
        vQueueDelete(input_queue);
        input_queue = NULL;
    }
}

int DRIVER_Process_Events(void)
{
    // Drain all pending web input events from the queue.
    // This runs on Core 0, so PAL_KeyDown/Up are always called from Core 0.
    WebInputEvent evt;
    while (xQueueReceive(input_queue, &evt, 0) == pdTRUE)
    {
        if (evt.is_down)
            PAL_KeyDown(evt.keycode);
        else
            PAL_KeyUp(evt.keycode);
    }
    return 0;
}
#endif /* ARDUINO_ARCH_ESP32 && PAL_ESP32_BACKEND_WEB */
