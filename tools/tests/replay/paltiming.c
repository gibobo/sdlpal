// Timing tests: run transitions and frame pacing on the virtual clock and compare
// their total duration with the original game's (upstream's) duration.
// Usage: paltiming <work ms per present>
//
// Upstream paces frames and fades by deadline: a step lasts max(duration, work),
// and the time-driven fades last the same no matter how slow presenting is.
#include "../../../src/driver.h"
#include "../../../src/global.h"
#include "../../../src/main.h"
#include "../../../src/palette.h"
#include "../../../src/rngplay.h"
#include "../../../src/util.h"
#include "../../../src/video.h"
#include "../../../driver/DrvIf_internal.h"
#include "replay.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define HANG_LIMIT_MS 60000u

static uint32_t g_dwWorkMs = 0;
static uint32_t g_dwTestStart = 0;
static const char *g_pszTest = "init";
static int g_nFailures = 0;

// Present: charge the simulated work to the virtual clock.
void DRIVER_FrameShow(uint8_t *frame, const uint16_t roi_x, const uint16_t roi_y, const uint16_t roi_w,
                      const uint16_t roi_h, const uint8_t padding_flag)
{
    (void)frame;
    (void)roi_x;
    (void)roi_y;
    (void)roi_w;
    (void)roi_h;
    (void)padding_flag;
    REPLAY_AdvanceClock(g_dwWorkMs);
}

void DRIVER_UpdatePalette(const uint8_t *rgPalette)
{
    (void)rgPalette;
}

int DRIVER_Init_Video(void)
{
    return 0;
}

void DRIVER_DeInit_Video(void)
{
}

int DRIVER_Init_Event(void)
{
    return 0;
}

void DRIVER_DeInit_Event(void)
{
}

// No input; stop a test that never finishes instead of spinning forever.
int DRIVER_Process_Events(void)
{
    if (UTIL_GetMilliseconds() - g_dwTestStart > HANG_LIMIT_MS)
    {
        fprintf(stderr, "FAIL %s: still running after %u virtual ms\n", g_pszTest, HANG_LIMIT_MS);
        exit(1);
    }
    return 0;
}

// Start timing a test.
static void Begin(const char *name)
{
    g_pszTest = name;
    g_dwTestStart = UTIL_GetMilliseconds();
}

// Compare the elapsed virtual time with the expected duration.
static void End(uint32_t expected, uint32_t tolerance)
{
    uint32_t elapsed = UTIL_GetMilliseconds() - g_dwTestStart;
    int ok = elapsed + tolerance >= expected && elapsed <= expected + tolerance;
    printf("%s %-24s %6u ms, expected %6u +- %u\n", ok ? "PASS" : "FAIL", g_pszTest, elapsed, expected, tolerance);
    if (!ok)
        g_nFailures++;
}

// A transition lasts its nominal length plus the work of presenting its final state.
static void EndTransition(uint32_t nominal, uint32_t step)
{
    End(nominal + g_dwWorkMs, step + g_dwWorkMs);
}

// Run every timing case with the given simulated work per present.
int main(int argc, char *argv[])
{
    const uint32_t w = argc > 1 ? (uint32_t)strtoul(argv[1], NULL, 10) : 0;

    // Start a new game so scene-based transitions have a scene to draw.
    PAL_Init();
    PAL_ReloadInNextTick(0);
    PAL_SetPalette(0, false);
    g_dwWorkMs = w;
    printf("simulated work: %u ms per present\n", w);

    // A frame lasts max(duration, work).
    Begin("frame-delay 100");
    {
        uint32_t start = UTIL_GetMilliseconds();
        REPLAY_AdvanceClock(w);
        UTIL_FrameDelay(start, FRAME_TIME);
    }
    End(w > FRAME_TIME ? w : FRAME_TIME, 1);

    // Time-driven fades: 60 steps of 10*d ms, ending one step after the last.
    Begin("fade-out d=1");
    PAL_FadeOut(1);
    EndTransition(610, 10);
    Begin("fade-out d=3");
    PAL_FadeOut(3);
    EndTransition(1830, 30);
    Begin("fade-in d=1");
    PAL_FadeIn(0, false, 1);
    EndTransition(610, 10);
    Begin("fade-out d=20");
    PAL_FadeOut(20);
    EndTransition(610 * 20, 200);

    // Stepped fades and screen transitions at their nominal step durations.
    Begin("color-fade d=1");
    PAL_ColorFade(1, 0x0F, false);
    EndTransition(64 * 10, 10);
    Begin("color-fade d=3");
    PAL_ColorFade(3, 0x0F, true);
    EndTransition(64 * 30, 30);
    Begin("palette-fade");
    PAL_PaletteFade(0, false, false);
    EndTransition(32 * (FRAME_TIME / 4), FRAME_TIME / 4);
    Begin("palette-fade scene");
    PAL_PaletteFade(0, false, true);
    EndTransition(32 * FRAME_TIME, FRAME_TIME);
    Begin("scene-fade s=1");
    PAL_SceneFade(0, false, 1);
    EndTransition(64 * FRAME_TIME, FRAME_TIME);
    Begin("scene-fade s=-4");
    PAL_SceneFade(0, false, -4);
    EndTransition(16 * FRAME_TIME, FRAME_TIME);
    Begin("fade-to-red");
    PAL_FadeToRed();
    EndTransition(32 * 75, 75);
    PAL_SetPalette(0, false);
    Begin("switch-screen");
    VIDEO_SwitchScreen();
    EndTransition(6 * 60, 60);
    Begin("fade-screen s=1");
    VIDEO_FadeScreen(1);
    EndTransition(71 * 20, 1);

    // RNG movie: frames 0..20 inclusive at 25 frames per second.
    Begin("rng-play 21 frames");
    PAL_RNGPlay(6, 0, 20, 25);
    End(21 * 40, 40 + w);

    printf("%d failure(s)\n", g_nFailures);
    return g_nFailures == 0 ? 0 : 1;
}
