#ifndef CORE_DIFF_API_H
#define CORE_DIFF_API_H

// Entry points of the upstream code (master_ prefix) and of the sliced development
// functions (dev_ prefix), declared with plain C types so the test driver can use
// them next to the development headers.

#include "master_shared.h"
#include <stdint.h>

// Upstream palcommon.c and yj1.c, renamed by master_rename.h.
int master_PAL_RLEBlitToSurface(const uint8_t *lpBitmapRLE, MasterSurface *lpDstSurface, uint32_t pos);
int master_PAL_RLEBlitToSurfaceWithShadow(const uint8_t *lpBitmapRLE, MasterSurface *lpDstSurface, uint32_t pos,
                                          int bShadow);
int master_PAL_RLEBlitWithColorShift(const uint8_t *lpBitmapRLE, MasterSurface *lpDstSurface, uint32_t pos,
                                     int iColorShift);
int master_PAL_RLEBlitMonoColor(const uint8_t *lpBitmapRLE, MasterSurface *lpDstSurface, uint32_t pos,
                                uint8_t bColor, int iColorShift);
int master_PAL_FBPBlitToSurface(uint8_t *lpBitmapFBP, MasterSurface *lpDstSurface);
int master_PAL_RLEGetWidth(const uint8_t *lpBitmapRLE);
int master_PAL_RLEGetHeight(const uint8_t *lpBitmapRLE);
uint16_t master_PAL_SpriteGetNumFrames(const uint8_t *lpSprite);
const uint8_t *master_PAL_SpriteGetFrame(const uint8_t *lpSprite, int iFrameNum);
int master_PAL_MKFDecompressChunk(uint8_t *lpBuffer, unsigned int uiBufferSize, unsigned int uiChunkNum, void *fp);
int master_YJ2_Decompress(const void *Source, void *Destination, int DestSize);
extern int (*master_Decompress)(const void *, void *, int);
extern MasterConfig master_gConfig;

// Upstream util.c random number generator.
void master_SeedRandom(unsigned int iSeed);
unsigned int master_GetRandomState(void);
int master_RandomLong(int from, int to);
float master_RandomFloat(float from, float to);

// Upstream fight.c and rngplay.c functions.
short master_CalcBaseDamage(uint16_t wAttackStrength, uint16_t wDefense);
short master_CalcPhysicalAttackDamage(uint16_t wAttackStrength, uint16_t wDefense, uint16_t wAttackResistance);
short master_CalcMagicDamage(uint16_t wMagicStrength, uint16_t wDefense, const uint16_t rgwElementalResistance[],
                             uint16_t wPoisonResistance, uint16_t wResistanceMultiplier, uint16_t wMagicID);
int master_RNGBlitToSurface(const uint8_t *rng, int length, MasterSurface *lpDstSurface);

// Development functions that are file-static in src/.
short dev_CalcBaseDamage(uint16_t wAttackStrength, uint16_t wDefense);
short dev_CalcPhysicalAttackDamage(uint16_t wAttackStrength, uint16_t wDefense, uint16_t wAttackResistance);
short dev_CalcMagicDamage(uint16_t wMagicStrength, uint16_t wDefense, const uint16_t rgwElementalResistance[],
                          uint16_t wPoisonResistance, uint16_t wResistanceMultiplier, uint16_t wMagicID);
int dev_RNGBlitToSurface(const uint8_t *rng, int32_t length, uint8_t *dstSurface);

#endif
