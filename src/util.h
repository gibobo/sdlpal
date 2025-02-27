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

#ifndef RESOURCE_PATH
#define RESOURCE_PATH "."
#endif

#ifndef SEEK_SET
#define SEEK_SET 0
#endif

#define FRAME_TIME (1000 / 10)        // 10 FPS

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

long flength(void *fp);

int RandomLong(int from, int to);

float RandomFloat(float from, float to);

void TerminateOnError(const char *fmt, ...);

void *UTIL_malloc(unsigned int buffer_size);

void *UTIL_calloc(unsigned int n, unsigned int size);

void *UTIL_realloc(void *ptr, unsigned int n, unsigned int size);

void UTIL_free(void *ptr);

// Platform-specific utilities
unsigned int UTIL_GetTicks(void);

void UTIL_Sleep(unsigned int tm);

void *UTIL_fopen(const char *_FileName, const char *_Mode);

void *UTIL_fopen_without_checking(const char *_FileName, const char *_Mode);

int UTIL_fseek(void *_Stream, long _Offset, int _Origin);

unsigned int UTIL_fread(void *_Buffer, unsigned int _ElementSize, unsigned int _ElementCount, void *_Stream);

unsigned int UTIL_fwrite(void *_Buffer, unsigned int _ElementSize, unsigned int _ElementCount, void *_Stream);

void UTIL_fclose(void *fp);

#ifdef __cplusplus
extern "C" {
#endif

void UTIL_Delay(int ms);

#ifdef __cplusplus
}
#endif

#endif
