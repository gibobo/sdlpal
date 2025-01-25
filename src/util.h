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

#include <stdio.h>

#ifndef RESOURCE_PATH
#define RESOURCE_PATH "."
#endif

#ifndef SOURCE_DIR
#define SOURCE_DIR "."
#endif

#ifdef __cplusplus
extern "C" {
#endif

long flength(FILE *fp);

int RandomLong(int from, int to);

float RandomFloat(float from, float to);

void TerminateOnError(const char *fmt, ...);

void *UTIL_malloc(size_t buffer_size);

void *UTIL_calloc(size_t n, size_t size);

FILE *PAL_fopen(const char *_FileName, const char *_Mode);

int PAL_fseek(FILE *_Stream, long _Offset, int _Origin);

unsigned int PAL_fread(void *_Buffer, unsigned int _ElementSize, unsigned int _ElementCount, FILE *_Stream);

unsigned int PAL_fwrite(void *_Buffer, unsigned int _ElementSize, unsigned int _ElementCount, FILE *_Stream);

void PAL_fclose(FILE *fp);

// Platform-specific utilities
void UTIL_Delay(unsigned int ms);

unsigned int UTIL_GetTicks(void);

void UTIL_Sleep(unsigned int tm);

void PAL_DelayUntil(unsigned int tm);

#ifdef __cplusplus
}
#endif

#endif
