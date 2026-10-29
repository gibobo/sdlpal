// Upstream random number generator, built from the functions sliced out of util.c.
#include "common.h"

#include "master/util_rng.inc"

// Seed the upstream generator the same way development's UTIL_SetRandomSeed does.
void master_SeedRandom(unsigned int iSeed)
{
    lsrand(iSeed);
}

// Expose the upstream generator state to compare consumption with development.
unsigned int master_GetRandomState(void)
{
    return (unsigned int)glSeed;
}
