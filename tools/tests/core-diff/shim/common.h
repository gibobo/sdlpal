#ifndef CORE_DIFF_SHIM_COMMON_H
#define CORE_DIFF_SHIM_COMMON_H

// Minimal replacement for upstream common.h: the Windows-style types and the
// few SDL facilities that the upstream files under test depend on.

#include "master_shared.h"
#include "master_types.h"
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif
#define VOID void

#define PAL_C_LINKAGE_BEGIN
#define PAL_C_LINKAGE_END
#define PAL_LARGE
#if defined(_MSC_VER)
#define PAL_FORCE_INLINE static __forceinline
#else
#define PAL_FORCE_INLINE static inline __attribute__((always_inline))
#endif

#define PAL_fread(buf, elem, num, fp) if (fread((buf), (elem), (num), (fp)) < (num)) return -1

// Little-endian hosts only, like the ESP32 and x86 targets.
#define SDL_SwapLE16(x) (x)
#define SDL_SwapLE32(x) (x)
#define SDL_VERSION_ATLEAST(x, y, z) 1
typedef MasterSurface SDL_Surface;

#endif
