// Virtual clock linked in place of src/util_clock.c: time advances only when the
// game sleeps or a driver simulates work, so runs do not depend on host speed.
#include "replay.h"
#include "util.h"

static uint32_t g_dwVirtualTime = REPLAY_CLOCK_START;

// Report the virtual time in milliseconds.
uint32_t UTIL_GetMilliseconds(void)
{
    return g_dwVirtualTime;
}

// Sleeping only moves the virtual clock forward.
void UTIL_Sleep(uint32_t ms)
{
    g_dwVirtualTime += ms;
}

// Move the virtual clock forward on behalf of a driver.
void REPLAY_AdvanceClock(uint32_t ms)
{
    g_dwVirtualTime += ms;
}
