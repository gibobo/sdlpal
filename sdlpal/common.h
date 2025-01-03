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

#include <assert.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wchar.h>
#ifndef _WIN32
#include <dirent.h>
#include <unistd.h>
#endif

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
#endif

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#ifdef PATH_MAX
#define PAL_MAX_PATH PATH_MAX
#else
#define PAL_MAX_PATH 1024
#endif

#ifndef PAL_LARGE
#define PAL_LARGE
#endif

#define PAL_GLOBAL_BUFFER_SIZE 1024

#endif
