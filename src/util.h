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

// Resource identifiers
typedef enum
{
    Res_ABC = 0, // enemy sprites in battle
    Res_BALL,    // item bitmaps
    Res_DATA,    // misc data
    Res_F,       // player sprites during battle
    Res_FBP,     // battlefield background images
    Res_FIRE,    // fire effect sprites
    Res_GOP,     // map objects
    Res_MAP,     // map data
    Res_MGO,     // sprites in scenes
    Res_MUS,     // music data
    Res_PAT,     // palette data
    Res_RGM,     // character face bitmaps
    Res_RNG,     // RNG animation data
    Res_SOUNDS,  // sound data,
    Res_SSS,     // special scene sprites
    Res_Count    // total number of resources
} PALRES;

long UTIL_FileLength(void *fp);

int RandomLong(int from, int to);

float RandomFloat(float from, float to);

void TerminateOnError(const char *fmt, ...);

void *UTIL_malloc(unsigned int buffer_size);

void *UTIL_calloc(unsigned int n, unsigned int size);

void *UTIL_realloc(void *ptr, unsigned int n, unsigned int size);

void UTIL_free(void *ptr);

// Platform-specific utilities
unsigned long UTIL_GetMilliseconds(void);

void *UTIL_fopen(const char *_FileName, const char *_Mode);

void *UTIL_fopen_without_checking(const char *_FileName, const char *_Mode);

int UTIL_fseek(void *_Stream, long _Offset, int _Origin);

unsigned int UTIL_fread(void *_Buffer, unsigned int _ElementSize, unsigned int _ElementCount, void *_Stream);

unsigned int UTIL_fwrite(void *_Buffer, unsigned int _ElementSize, unsigned int _ElementCount, void *_Stream);

void UTIL_fclose(void *fp);

void *UTIL_Open(const PALRES res, const char *_Mode);

void *UTIL_Open_without_checking(const PALRES res, const char *_Mode);

unsigned char UTIL_Close(const PALRES res);

#ifdef __cplusplus
extern "C" {
#endif

void UTIL_Sleep(unsigned int ms);

unsigned int UTIL_Delay(unsigned int ms);

unsigned int UTIL_WaitKeys(unsigned int ms, unsigned int wait_keys);

#ifdef __cplusplus
}
#endif

#endif
