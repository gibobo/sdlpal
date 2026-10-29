#include "util.h"
#include <stdint.h>
#include <time.h>
#if defined(_WIN32)
#include <windows.h>
#elif defined(ARDUINO_ARCH_ESP32)
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

#if UTIL_PLATFORM_P4
#include "../driver/esp32/p4/platform.h"
#endif

// The wall clock and sleep primitives live in their own translation unit so that
// test builds can link a virtual clock in place of this file.

#ifdef _WIN32
int gettimeofday(struct timeval *tp, void *tzp)
{
    (void)tzp; // unused parameter
    time_t clock;
    struct tm tm = {0};
    SYSTEMTIME wtm;
    GetLocalTime(&wtm);
    tm.tm_year = wtm.wYear - 1900;
    tm.tm_mon = wtm.wMonth - 1;
    tm.tm_mday = wtm.wDay;
    tm.tm_hour = wtm.wHour;
    tm.tm_min = wtm.wMinute;
    tm.tm_sec = wtm.wSecond;
    tm.tm_isdst = -1;
    clock = mktime(&tm);
    tp->tv_sec = (long)clock;
    tp->tv_usec = wtm.wMilliseconds * 1000;
    return 0;
}
#endif

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
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000LL + (tv.tv_usec / 1000LL));
#endif
}

void UTIL_Sleep(uint32_t ms)
{
#if defined(ARDUINO_ARCH_ESP32)
    vTaskDelay(pdMS_TO_TICKS(ms));
#elif defined(_WIN32)
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}
