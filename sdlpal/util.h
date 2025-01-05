/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
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

#ifdef __cplusplus
extern "C" {
#endif

long
flength(
   FILE *fp
);

void
trim(
   char *str
);

char *
UTIL_GlobalBuffer(
	int         index
);

/*++
  Purpose:

    Does a varargs printf into the user-supplied buffer,
	so we don't need to have varargs versions of all text functions.

  Parameters:

    buffer - user-supplied buffer.
	buflen - size of the buffer, including null-terminator.
    format - the format string.

  Return value:

    The value of buffer if buffer is non-NULL and buflen > 0, otherwise NULL.

--*/
char *
UTIL_va(
	char       *buffer,
	int         buflen,
	const char *format,
	...
);

#define PAL_va(fmt, ...) UTIL_va(UTIL_GlobalBuffer(0), PAL_GLOBAL_BUFFER_SIZE, fmt, __VA_ARGS__)

int
RandomLong(
   int from,
   int to
);

float
RandomFloat(
   float from,
   float to
);

void
UTIL_Delay(
   unsigned int ms
);

void
TerminateOnError(
   const char *fmt,
   ...
);

void *
UTIL_malloc(
   size_t               buffer_size
);

void *
UTIL_calloc(
   size_t               n,
   size_t               size
);

FILE *
UTIL_OpenRequiredFileForMode(
   const char*               lpszFileName,
   const char*               szMode
);

FILE *
UTIL_OpenFile(
   const char*               lpszFileName
);

FILE *
UTIL_OpenFileForMode(
   const char*               lpszFileName,
   const char*               szMode
);

FILE *
UTIL_OpenFileAtPath(
	const char*              lpszPath,
	const char*              lpszFileName
);

/*++
  Purpose:

    Open a file in desired mode at the specific path.
	If fails, return NULL.

  Parameters:

    [IN]  lpszPath - path to locate the file.
    [IN]  lpszFileName - file name to open.
    [IN]  szMode - file open mode.

  Return value:

    Pointer to the file.

--*/
FILE *
UTIL_OpenFileAtPathForMode(
	const char*              lpszPath,
	const char*              lpszFileName,
	const char*              szMode
);

void
UTIL_CloseFile(
   FILE                *fp
);

/*++
  Purpose:

    Combine the 'dir' and 'file' part into a single path string.
	If 'dir' is non-NULL, then it ensures that the output string contains
	'/' between 'dir' and 'file' (no matter whether 'file' is NULL or not).

  Parameters:

    buffer - user-supplied buffer.
	buflen - size of the buffer, including null-terminator.
    dir    - the directory path.
	file   - the file path.

  Return value:

    The value of buffer if buffer is non-NULL and buflen > 0, otherwise NULL.

--*/
const char *
UTIL_CombinePath(
	char       *buffer,
	size_t      buflen,
	int         numentry,
	...
);

#define PAL_CombinePath(i, d, f) UTIL_CombinePath(UTIL_GlobalBuffer(i), PAL_GLOBAL_BUFFER_SIZE, 2, (d), (f))

int
UTIL_IsFileExist(
    const char *path
);

const char *
UTIL_GetFullPathName(
	char       *buffer,
	size_t      buflen,
	const char *basepath,
	const char *subpath
);

char *UTIL_basename(const char *path);

/*
 * Platform-specific utilities
 */

int
UTIL_IsAbsolutePath(
	const char *lpszFileName
);

void
UTIL_Platform_Quit(
	void
);

unsigned int UTIL_GetTicks(void);

void UTIL_Sleep(unsigned int tm);

void PAL_DelayUntil(unsigned int tm);

#ifdef __cplusplus
}
#endif

#endif
