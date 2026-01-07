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

char *UTIL_Filename(const char *fmt, ...);

void TerminateOnError(const char *fmt, ...);

void *UTIL_malloc(uint32_t buffer_size);

void *UTIL_calloc(uint32_t n, uint32_t size);

void *UTIL_realloc(void *ptr, uint32_t n, uint32_t size);

void UTIL_free(void *ptr);

// Platform-specific utilities
uint32_t UTIL_GetMilliseconds(void);

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

uint32_t UTIL_Delay(uint32_t ms);

uint32_t UTIL_WaitKeys(uint32_t ms, uint32_t wait_keys);

#ifdef __cplusplus
}
#endif

#endif
