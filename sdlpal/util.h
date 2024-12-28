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


typedef enum tagLOGLEVEL
{
	LOGLEVEL_MIN,
	LOGLEVEL_VERBOSE = LOGLEVEL_MIN,
	LOGLEVEL_DEBUG,
	LOGLEVEL_INFO,
	LOGLEVEL_WARNING,
	LOGLEVEL_ERROR,
	LOGLEVEL_FATAL,
	LOGLEVEL_MAX = LOGLEVEL_FATAL,
} LOGLEVEL;

#define PAL_LOG_MAX_OUTPUTS   (LOGLEVEL_MAX + 1)

#if defined(DEBUG) || defined(_DEBUG)
# define PAL_DEFAULT_LOGLEVEL  LOGLEVEL_MIN
#else
# define PAL_DEFAULT_LOGLEVEL  LOGLEVEL_MAX
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

#define PAL_va(i, fmt, ...) UTIL_va(UTIL_GlobalBuffer(i), PAL_GLOBAL_BUFFER_SIZE, fmt, __VA_ARGS__)

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


/*
 * Logging utilities
 */

/*++
  Purpose:

    The pointer to callback function that produces actual log output.

  Parameters:

    [IN]  level    - The log level of this output call.
	[IN]  full_log - The full log string produced by UTIL_LogOutput.
	[IN]  user_log - The log string produced by user-provided format.

  Return value:

    None.

--*/
typedef void(*LOGCALLBACK)(LOGLEVEL level, const char *full_log, const char *user_log);

/*++
  Purpose:

    Adds a log output callback.

  Parameters:

    [IN]  callback     - The callback function to be added. Once added,
	                     it will be called by UTIL_LogOutput.
    [IN]  loglevel     - The minimal log level that the callback should
	                     be called. Any log whose level below this will
						 be ignored by the callback.

  Return value:

    The slot id (>= 0), -1 if all slots are used or callback is NULL.

--*/

/*++
  Purpose:

    Removes a log output callback.

  Parameters:

    [IN]  id           - The id of callback function to be removed.

  Return value:

    None

--*/
void
UTIL_LogRemoveOutputCallback(
	int            id
);

/*++
  Purpose:

    Set the minimal log level that could be output.
	Any level below this level will produce no output.

  Parameters:

    [IN]  minlevel - The minimal log level, must be within the
	                 range [LOGLEVEL_MIN, LOGLEVEL_MAX].

  Return value:

    None.

--*/
void
UTIL_LogOutput(
	LOGLEVEL       level,
	const char    *fmt,
	...
);

/*++
  Purpose:

    Set the minimal log level that could be output.
	Any level below this level will produce no output.

  Parameters:

    [IN]  minlevel - The minimal log level, must be within the
	                 range [LOGLEVEL_MIN, LOGLEVEL_MAX].

  Return value:

    None.

--*/
void
UTIL_LogSetLevel(
	LOGLEVEL       minlevel
);

void
UTIL_LogSetPrelude(
    const char    *prelude
);

#ifdef __cplusplus
}
#endif

#endif
