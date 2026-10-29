// SDLPAL platform integration; GPL-3.0-or-later, like the game core.
#ifndef PAL_P4_PLATFORM_H
#define PAL_P4_PLATFORM_H
#include "../../../src/util.h"
#ifdef __cplusplus
extern "C" {
#endif
uint32_t P4_GameMilliseconds(uint32_t raw);
void P4_TransitionFinish(UTIL_TransitionClock *clock, const char *name);
void P4_FrameReport(uint32_t start, uint32_t duration, uint32_t work);
void P4_PrepareShutdown(void);
void P4_FinishShutdown(int exit_code);
void *P4_OpenMusicFile(const char *path, int32_t track, uint32_t rate, uint16_t channels);
#ifdef __cplusplus
}
#endif
#endif
