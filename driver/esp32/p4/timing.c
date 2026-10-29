// SDLPAL P4 frame pacing reports; GPL-3.0-or-later.
// The core paces frames and transitions by deadline (src/util.c); these hooks
// only print what the budgeted frames and transitions actually took.
#include "../backend_select.h"
#if defined(ARDUINO_ARCH_ESP32) && PAL_ESP32_BACKEND_P4
#include "platform.h"
#include <stdio.h>

void P4_FrameReport(uint32_t start, uint32_t duration, uint32_t work)
{
    static uint32_t last_report;
    uint32_t end = UTIL_GetMilliseconds();
    if (end - last_report >= 2000)
    {
        printf("P4 frame: work=%lu ms total=%lu ms target=%lu ms\n",
               (unsigned long)work, (unsigned long)(end - start), (unsigned long)duration);
        last_report = end;
    }
}

void P4_TransitionFinish(UTIL_TransitionClock *clock, const char *name)
{
    printf("P4 transition %s: total=%lu ms target=%lu ms shown=%lu skipped=%lu\n", name,
           (unsigned long)(UTIL_GetMilliseconds() - clock->start),
           (unsigned long)(clock->deadline - clock->start),
           (unsigned long)clock->presented, (unsigned long)clock->skipped);
}
#endif
