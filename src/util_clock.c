#include "util.h"
#include <stdint.h>
#if defined(_WIN32)
#include <windows.h>
#elif defined(ARDUINO_ARCH_ESP32)
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#else
#include <time.h>
#include <unistd.h>
#endif

#if UTIL_PLATFORM_P4
#include "../driver/esp32/p4/platform.h"
#endif

// The clock and sleep primitives live in their own translation unit so that
// test builds can link a virtual clock in place of this file. The clock is
// monotonic and wraps every 49.7 days; compare times by signed difference.

uint32_t UTIL_GetMilliseconds(void)
{
#ifdef ARDUINO_ARCH_ESP32
    // High resolution monotonic time since boot in microseconds
    uint32_t raw=(uint32_t)(esp_timer_get_time() / 1000ULL);
#if UTIL_PLATFORM_P4
    return P4_GameMilliseconds(raw);
#else
    return raw;
#endif
#elif defined(_WIN32)
    static LARGE_INTEGER frequency;
    LARGE_INTEGER counter;
    if (frequency.QuadPart == 0)
        QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return (uint32_t)(counter.QuadPart * 1000 / frequency.QuadPart);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)((uint64_t)ts.tv_sec * 1000U + (uint64_t)ts.tv_nsec / 1000000U);
#endif
}

void UTIL_Sleep(uint32_t ms)
{
#if defined(ARDUINO_ARCH_ESP32)
    vTaskDelay(pdMS_TO_TICKS(ms));
#elif defined(_WIN32)
    // Without a 1 ms timer period, Sleep(1) lasts a whole 15.6 ms scheduler tick.
    static int fTimerPeriodSet = 0;
    if (!fTimerPeriodSet)
    {
        timeBeginPeriod(1);
        fTimerPeriodSet = 1;
    }
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}
