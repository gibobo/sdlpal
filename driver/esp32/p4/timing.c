// SDLPAL P4 frame pacing; GPL-3.0-or-later.
#include "../backend_select.h"
#if defined(ARDUINO_ARCH_ESP32) && PAL_ESP32_BACKEND_P4
#include "platform.h"
#include "../../../src/input.h"
#include <stdio.h>

void P4_FrameDelay(uint32_t start, uint32_t duration)
{
    uint32_t work = UTIL_GetMilliseconds() - start;
    UTIL_Delay(work < duration ? duration - work : 1);
    static uint32_t last_report;
    uint32_t end = UTIL_GetMilliseconds();
    if (end - last_report >= 2000)
    {
        printf("P4 frame: work=%lu ms total=%lu ms target=%lu ms\n",
               (unsigned long)work, (unsigned long)(end - start), (unsigned long)duration);
        last_report = end;
    }
}

int P4_TransitionStep(UTIL_TransitionClock *clock, uint32_t duration)
{
    clock->deadline += duration;
    // Skip only the presentation. Callers still apply every state update.
    if ((int32_t)(UTIL_GetMilliseconds() - clock->deadline) >= 0)
    {
        clock->skipped++;
        return 0;
    }
    clock->presented++;
    return 1;
}

void P4_TransitionWait(UTIL_TransitionClock *clock)
{
    int32_t remaining = (int32_t)(clock->deadline - UTIL_GetMilliseconds());
    if (remaining > 0)
        UTIL_Delay((uint32_t)remaining);
    else
    {
        PAL_ClearKeyState();
        PAL_ProcessEvent();
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
