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

#ifdef PATH_MAX
# define PAL_MAX_PATH  PATH_MAX
#else
# define PAL_MAX_PATH  1024
#endif

/* When porting SDLPAL to a new platform, please make a separate directory and put a file 
   named 'pal_config.h' that contains marco definitions & header includes into the directory.
   The example of this file can be found in directories of existing portings.
 */

#ifndef PAL_LARGE
# define PAL_LARGE
#endif


#define PAL_GLOBAL_BUFFER_SIZE 1024

#endif
