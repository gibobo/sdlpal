#ifndef CORE_DIFF_MASTER_SHARED_H
#define CORE_DIFF_MASTER_SHARED_H

// Layouts shared by the upstream shim headers and the test driver, which cannot
// include the upstream headers directly.

// Stands in for SDL_Surface: upstream blitters only touch w, h, pitch and pixels.
typedef struct MasterSurface
{
    int w;
    int h;
    int pitch;
    void *pixels;
} MasterSurface;

// Stands in for upstream CONFIGURATION: the code under test only reads fIsWIN95.
typedef struct MasterConfig
{
    int fIsWIN95;
} MasterConfig;

#endif
