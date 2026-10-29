// Development functions that are file-static in src/, sliced out of the working
// tree so the tests exercise the exact code in the checkout.
#include "global.h"
#include "util.h"
#include "video.h"

#define PAL_CalcPhysicalAttackDamage dev_slice_PAL_CalcPhysicalAttackDamage

#include "dev/fight_damage.inc"
#include "dev/rngplay_blit.inc"

// Call the development base damage formula.
short dev_CalcBaseDamage(uint16_t wAttackStrength, uint16_t wDefense)
{
    return PAL_CalcBaseDamage(wAttackStrength, wDefense);
}

// Call the development physical attack damage formula.
short dev_CalcPhysicalAttackDamage(uint16_t wAttackStrength, uint16_t wDefense, uint16_t wAttackResistance)
{
    return PAL_CalcPhysicalAttackDamage(wAttackStrength, wDefense, wAttackResistance);
}

// Call the development magic damage formula.
short dev_CalcMagicDamage(uint16_t wMagicStrength, uint16_t wDefense, const uint16_t rgwElementalResistance[],
                          uint16_t wPoisonResistance, uint16_t wResistanceMultiplier, uint16_t wMagicID)
{
    return PAL_CalcMagicDamage(wMagicStrength, wDefense, rgwElementalResistance, wPoisonResistance,
                               wResistanceMultiplier, wMagicID);
}

// Call the development RNG frame blitter.
int dev_RNGBlitToSurface(const uint8_t *rng, int32_t length, uint8_t *dstSurface)
{
    return PAL_RNGBlitToSurface(rng, length, dstSurface);
}
