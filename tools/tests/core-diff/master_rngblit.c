// Upstream RNG frame blitter, sliced out of rngplay.c.
#include "common.h"

#include "master/rngplay_blit.inc"

// Call the upstream (file-static) blitter.
int master_RNGBlitToSurface(const uint8_t *rng, int length, MasterSurface *lpDstSurface)
{
    return PAL_RNGBlitToSurface(rng, length, lpDstSurface);
}
