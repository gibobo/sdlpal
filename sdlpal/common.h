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

#ifndef _COMMON_H
#define _COMMON_H

#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <stdarg.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#define __WIDETEXT(quote) L##quote
#define WIDETEXT(quote) __WIDETEXT(quote)

#define STR_INDIR(x)                    #x
#define STR(x)                          STR_INDIR(x)

#if !defined(fmax) || !defined(fmin)
# include <math.h>
#endif

#include <float.h>

#ifndef max
# define max fmax
#endif

#ifndef min
# define min fmin
#endif

#ifndef PAL_FORCE_INLINE
#if defined(_MSC_VER)
#define PAL_FORCE_INLINE static __forceinline
#else
#define PAL_FORCE_INLINE __attribute__((always_inline)) static __inline__
#endif
#endif /* PAL_FORCE_INLINE not defined */

#ifdef _WIN32
// # include <windows.h>
// # include <io.h>
#else
# include <unistd.h>
# include <dirent.h>
#endif

# ifndef FALSE
#  define FALSE               0
# endif
# ifndef TRUE
#  define TRUE                1
# endif

typedef long LONG;
typedef int BOOL;
typedef unsigned char BYTE, *LPBYTE;

#ifdef PATH_MAX
# define PAL_MAX_PATH  PATH_MAX
#else
# define PAL_MAX_PATH  1024
#endif

/* When porting SDLPAL to a new platform, please make a separate directory and put a file 
   named 'pal_config.h' that contains marco definitions & header includes into the directory.
   The example of this file can be found in directories of existing portings.
 */

#include "pal_config.h"

#ifndef PAL_DEFAULT_FULLSCREEN_HEIGHT
# define PAL_DEFAULT_FULLSCREEN_HEIGHT PAL_DEFAULT_WINDOW_HEIGHT
#endif

#ifndef PAL_DEFAULT_TEXTURE_WIDTH
# define PAL_DEFAULT_TEXTURE_WIDTH     PAL_DEFAULT_WINDOW_WIDTH
#endif

#ifndef PAL_DEFAULT_TEXTURE_HEIGHT
# define PAL_DEFAULT_TEXTURE_HEIGHT    PAL_DEFAULT_WINDOW_HEIGHT
#endif

/* Default for 1024 samples */
#ifndef PAL_AUDIO_DEFAULT_BUFFER_SIZE
# define PAL_AUDIO_DEFAULT_BUFFER_SIZE   1024
#endif

#ifndef PAL_CONFIG_PREFIX
# define PAL_CONFIG_PREFIX "./"
#endif

#ifndef PAL_LARGE
# define PAL_LARGE
#endif

# define PAL_SCALE_SCREEN   TRUE

# define PAL_IS_VALID_JOYSTICK(s)  TRUE

#ifndef PAL_NATIVE_PATH_SEPARATOR
# define PAL_NATIVE_PATH_SEPARATOR "/"
#endif

#define PAL_fread(buf, elem, num, fp) if (fread((buf), (elem), (num), (fp)) < (num)) return -1

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

#define PAL_GLOBAL_BUFFER_SIZE 1024

#endif
