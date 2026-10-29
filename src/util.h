//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2024, SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 3
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>

#if defined(ARDUINO_ARCH_ESP32) && defined(PAL_ESP32_BACKEND_P4) && PAL_ESP32_BACKEND_P4
#define UTIL_PLATFORM_P4 1
#else
#define UTIL_PLATFORM_P4 0
#endif

#ifdef ARDUINO_ARCH_ESP32
#define RESOURCE_PATH "/sdcard"
#define CACHES_PATH   "/sdcard"
#else
#ifndef RESOURCE_PATH
#define RESOURCE_PATH "."
#endif

#ifndef CACHES_PATH
#define CACHES_PATH "."
#endif
#endif

#ifndef SEEK_SET
#define SEEK_SET 0
#endif
#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif
#ifndef SEEK_END
#define SEEK_END 2
#endif

#define FRAME_TIME (100U) // 10 FPS

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

long UTIL_FileLength(void *fp);

int RandomLong(int32_t from, int32_t to);

float RandomFloat(float from, float to);

void UTIL_SetRandomSeed(uint32_t iSeed);

uint32_t UTIL_GetRandomState(void);

char *UTIL_Filename(const char *fmt, ...);

void TerminateOnError(const char *fmt, ...);

void *UTIL_malloc(uint32_t buffer_size);

void *UTIL_calloc(uint32_t n, uint32_t size);

void *UTIL_realloc(void *ptr, uint32_t n, uint32_t size);

void UTIL_free(void *ptr);

// Allocate without zeroing, from PSRAM only on the ESP32; returns NULL instead
// of terminating on failure. For memory that only saves work, such as caches.
void *UTIL_TryMalloc(uint32_t buffer_size);

// Whether a block from UTIL_malloc lies in plentiful memory (PSRAM on the
// boards, any memory on the desktop), so it may be kept between calls; a block
// in a board's internal RAM should be freed after use instead.
int UTIL_IsPlentiful(const void *ptr);

// Set the function that frees rebuildable memory (caches) when an allocation
// fails; it returns the number of bytes freed, and the allocation is retried once.
void UTIL_SetMemoryReleaseHandler(uint32_t (*handler)(void));

// Platform-specific utilities
uint32_t UTIL_GetMilliseconds(void);

// A transition keeps its original length however slow presenting is: each step
// adds its duration to a deadline, every state update still runs, and a step
// that is already late is not presented. Callers always present the final state.
typedef struct UTIL_TransitionClock {
    uint32_t start, deadline, presented, skipped;
} UTIL_TransitionClock;
void UTIL_TransitionBegin(UTIL_TransitionClock *clock);
int UTIL_TransitionStep(UTIL_TransitionClock *clock, uint32_t duration);
void UTIL_TransitionWait(UTIL_TransitionClock *clock);
void UTIL_TransitionFinish(UTIL_TransitionClock *clock, const char *name);
// Wait until a frame that began at 'start' has lasted 'duration' ms; a frame
// whose work took longer only polls input, so it lasts max(duration, work).
void UTIL_FrameDelay(uint32_t start, uint32_t duration);
// Returns nonzero when the driver was stopped before shared resource cleanup.
int UTIL_PrepareShutdown(void);
void UTIL_FinishShutdown(int exit_code);
void *UTIL_OpenMusicFile(const char *path, int32_t track, uint32_t rate, uint16_t channels);

void *UTIL_fopen(const char *_FileName, const char *_Mode);

void *UTIL_fopen_without_checking(const char *_FileName, const char *_Mode);

int UTIL_fseek(void *_Stream, int64_t _Offset, int _Origin);

uint32_t UTIL_fread(void *_Buffer, uint32_t _ElementSize, uint32_t _ElementCount, void *_Stream);

uint32_t UTIL_fwrite(void *_Buffer, uint32_t _ElementSize, uint32_t _ElementCount, void *_Stream);

void UTIL_fclose(void *fp);

#ifdef __cplusplus
extern "C" {
#endif

void UTIL_Sleep(uint32_t ms);

void UTIL_DelayUntil(uint32_t dwDeadline);

uint32_t UTIL_Delay(uint32_t ms);

uint32_t UTIL_WaitKeys(uint32_t ms, uint32_t wait_keys);

#ifdef __cplusplus
}
#endif

#endif
