// Timing tests: run transitions and frame pacing on the virtual clock and compare
// their total duration with the original game's (upstream's) duration.
// Usage: paltiming <work ms per present>
//
// Upstream paces frames and fades by deadline: a step lasts max(duration, work),
// and the time-driven fades last the same no matter how slow presenting is.
#include "../../../src/driver.h"
#include "../../../src/global.h"
#include "../../../src/input.h"
#include "../../../src/main.h"
#include "../../../src/palette.h"
#include "../../../src/res.h"
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
static const uint8_t *g_pLastFrame = NULL;
static uint16_t g_rgLastRoi[5];

// Present: remember what was shown and charge the simulated work to the virtual clock.
void DRIVER_FrameShow(uint8_t *frame, const uint16_t roi_x, const uint16_t roi_y, const uint16_t roi_w,
                      const uint16_t roi_h, const uint8_t padding_flag)
{
    g_pLastFrame = frame;
    g_rgLastRoi[0] = roi_x;
    g_rgLastRoi[1] = roi_y;
    g_rgLastRoi[2] = roi_w;
    g_rgLastRoi[3] = roi_h;
    g_rgLastRoi[4] = padding_flag;
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

// Report a check that does not measure time.
static void Check(const char *name, bool ok)
{
    printf("%s %s\n", ok ? "PASS" : "FAIL", name);
    if (!ok)
        g_nFailures++;
}

// Whether the last present showed full-width rows of gpScreen from the given row in the given region.
static bool LastShow(uint32_t first_row, uint16_t roi_y, uint16_t roi_h, uint8_t padding_flag)
{
    return g_pLastFrame == gpScreen->pixels + first_row * SCREEN_W && g_rgLastRoi[0] == 0 &&
           g_rgLastRoi[1] == roi_y && g_rgLastRoi[2] == SCREEN_W && g_rgLastRoi[3] == roi_h &&
           g_rgLastRoi[4] == padding_flag;
}

// Run every timing case with the given simulated work per present.
int main(int argc, char *argv[])
{
    const uint32_t w = argc > 1 ? (uint32_t)strtoul(argv[1], NULL, 10) : 0;

    // Start a new game so scene-based transitions have a scene to draw.
    PAL_Init();
    PAL_ReloadInNextTick(0);
    PAL_LoadResources();
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

    // Like the original game, waiting keeps the keys pressed before the wait;
    // only an explicit clear or a key wait drops them.
    PAL_ClearKeyState();
    PAL_KeyDown(kKeyMenu);
    PAL_KeyUp(kKeyMenu);
    UTIL_Delay(FRAME_TIME);
    Check("delay keeps keys", (PAL_GetKeyInput() & kKeyMenu) != 0);
    UTIL_FrameDelay(UTIL_GetMilliseconds(), FRAME_TIME);
    Check("frame delay keeps keys", (PAL_GetKeyInput() & kKeyMenu) != 0);
    UTIL_WaitKeys(FRAME_TIME, 0);
    Check("key wait drops old keys", PAL_GetKeyInput() == kKeyNone);

    // Screen shake, like the original game: each present tests the remaining
    // count before using it up; odd counts move the picture up by the level,
    // even counts move it down.
    VIDEO_ShakeScreen(2, 3);
    VIDEO_UpdateScreen(NULL);
    Check("shake even count moves down", LastShow(0, 3, SCREEN_H - 3, 1));
    VIDEO_UpdateScreen(NULL);
    Check("shake odd count moves up", LastShow(3, 0, SCREEN_H - 3, 1));
    VIDEO_UpdateScreen(NULL);
    Check("shake ends", LastShow(0, 0, SCREEN_H, 0));

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
