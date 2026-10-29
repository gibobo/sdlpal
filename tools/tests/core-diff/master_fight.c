// Upstream damage formulas, sliced out of fight.c. They are compiled against the
// development game data structures (identical field names), so both versions read
// the same loaded data and only the formulas differ.
#include "global.h"
#include "master_types.h"
#include "util.h"

#define PAL_CalcPhysicalAttackDamage master_slice_PAL_CalcPhysicalAttackDamage

#include "master/fight_damage.inc"

// Call the upstream base damage formula.
short master_CalcBaseDamage(uint16_t wAttackStrength, uint16_t wDefense)
{
    return PAL_CalcBaseDamage(wAttackStrength, wDefense);
}

// Call the upstream physical attack damage formula.
short master_CalcPhysicalAttackDamage(uint16_t wAttackStrength, uint16_t wDefense, uint16_t wAttackResistance)
{
    return PAL_CalcPhysicalAttackDamage(wAttackStrength, wDefense, wAttackResistance);
}

// Call the upstream magic damage formula.
short master_CalcMagicDamage(uint16_t wMagicStrength, uint16_t wDefense, const uint16_t rgwElementalResistance[],
                             uint16_t wPoisonResistance, uint16_t wResistanceMultiplier, uint16_t wMagicID)
{
    return PAL_CalcMagicDamage(wMagicStrength, wDefense, rgwElementalResistance, wPoisonResistance,
                               wResistanceMultiplier, wMagicID);
}
