// Canonical hash of the gameplay state. Structs that mirror the data files hold
// only 16-bit fields and are hashed whole; everything else is hashed field by
// field so pointers, padding and the unused time-charging floats never matter.
#include "battle.h"
#include "global.h"
#include "replay.h"
#include "util.h"

extern BATTLE *g_Battle;

// FNV-1a over a byte range.
uint64_t REPLAY_HashBytes(uint64_t hash, const void *data, uint32_t size)
{
    const uint8_t *p = (const uint8_t *)data;
    uint32_t i;
    for (i = 0; i < size; i++)
    {
        hash ^= p[i];
        hash *= 0x100000001b3ull;
    }
    return hash;
}

// Hash one integer value independently of its declared width.
static uint64_t HashValue(uint64_t hash, int64_t value)
{
    return REPLAY_HashBytes(hash, &value, sizeof(value));
}

#define HASH_FIELD(h, field) HashValue((h), (int64_t)(field))
#define HASH_ARRAY(h, array) REPLAY_HashBytes((h), (array), (uint32_t)sizeof(array))

// Hash the battle fields that drive gameplay.
static uint64_t HashBattle(uint64_t h)
{
    int i;
    for (i = 0; i < MAX_PLAYERS_IN_PARTY; i++)
    {
        const BATTLEPLAYER *p = &g_Battle->rgPlayer[i];
        h = HASH_FIELD(h, p->iColorShift);
        h = HASH_FIELD(h, p->wHidingTime);
        h = HASH_FIELD(h, p->pos);
        h = HASH_FIELD(h, p->posOriginal);
        h = HASH_FIELD(h, p->wCurrentFrame);
        h = HASH_FIELD(h, p->state);
        h = HASH_FIELD(h, p->action.ActionType);
        h = HASH_FIELD(h, p->action.wActionID);
        h = HASH_FIELD(h, p->action.sTarget);
        h = HASH_FIELD(h, p->prevAction.ActionType);
        h = HASH_FIELD(h, p->prevAction.wActionID);
        h = HASH_FIELD(h, p->prevAction.sTarget);
        h = HASH_FIELD(h, p->fDefending);
        h = HASH_FIELD(h, p->fSecondAttack);
        h = HASH_FIELD(h, p->wPrevHP);
        h = HASH_FIELD(h, p->wPrevMP);
    }
    for (i = 0; i < MAX_ENEMIES_IN_TEAM; i++)
    {
        const BATTLEENEMY *e = &g_Battle->rgEnemy[i];
        h = HASH_FIELD(h, e->wObjectID);
        h = REPLAY_HashBytes(h, &e->e, (uint32_t)sizeof(e->e));
        h = HASH_ARRAY(h, e->rgwStatus);
        h = HASH_ARRAY(h, e->rgPoisons);
        h = HASH_FIELD(h, e->pos);
        h = HASH_FIELD(h, e->posOriginal);
        h = HASH_FIELD(h, e->wCurrentFrame);
        h = HASH_FIELD(h, e->state);
        h = HASH_FIELD(h, e->wScriptOnTurnStart);
        h = HASH_FIELD(h, e->wScriptOnBattleEnd);
        h = HASH_FIELD(h, e->wScriptOnReady);
        h = HASH_FIELD(h, e->wPrevHP);
        h = HASH_FIELD(h, e->iColorShift);
    }
    h = HASH_FIELD(h, g_Battle->wMaxEnemyIndex);
    h = HASH_FIELD(h, g_Battle->sBackgroundColorShift);
    h = HASH_FIELD(h, g_Battle->posSummon);
    h = HASH_FIELD(h, g_Battle->iSummonFrame);
    h = HASH_FIELD(h, g_Battle->fSummonColorShift);
    h = HASH_FIELD(h, g_Battle->iExpGained);
    h = HASH_FIELD(h, g_Battle->iCashGained);
    h = HASH_FIELD(h, g_Battle->fIsBoss);
    h = HASH_FIELD(h, g_Battle->fEnemyCleared);
    h = HASH_FIELD(h, g_Battle->BattleResult);
    h = HASH_FIELD(h, g_Battle->fEnemyMoving);
    h = HASH_FIELD(h, g_Battle->iHidingTime);
    h = HASH_FIELD(h, g_Battle->wMovingPlayerIndex);
    h = HASH_FIELD(h, g_Battle->iBlow);
    h = HASH_FIELD(h, g_Battle->Phase);
    for (i = 0; i < MAX_ACTIONQUEUE_ITEMS; i++)
    {
        const ACTIONQUEUE *q = &g_Battle->ActionQueue[i];
        h = HASH_FIELD(h, q->fIsEnemy);
        h = HASH_FIELD(h, q->wDexterity);
        h = HASH_FIELD(h, q->wIndex);
        h = HASH_FIELD(h, q->fIsSecond);
    }
    h = HASH_FIELD(h, g_Battle->iCurAction);
    h = HASH_FIELD(h, g_Battle->fRepeat);
    h = HASH_FIELD(h, g_Battle->fForce);
    h = HASH_FIELD(h, g_Battle->fFlee);
    h = HASH_FIELD(h, g_Battle->fPrevAutoAtk);
    h = HASH_FIELD(h, g_Battle->fPrevPlayerAutoAtk);
    h = HASH_ARRAY(h, g_Battle->coopContributors);
    h = HASH_FIELD(h, g_Battle->fThisTurnCoop);
    return h;
}

// Hash the game data and the session state, plus the random generator.
uint64_t REPLAY_StateHash(void)
{
    uint64_t h = REPLAY_FNV_OFFSET;
    if (gpGlobals == NULL)
        return h;

    // Game data that scripts modify.
    h = HASH_ARRAY(h, gpGlobals->g.rgScene);
    h = HASH_ARRAY(h, gpGlobals->g.rgObject);
    if (gpGlobals->g.lprgEventObject != NULL)
        h = REPLAY_HashBytes(h, gpGlobals->g.lprgEventObject,
                             (uint32_t)(gpGlobals->g.nEventObject * sizeof(EVENTOBJECT)));
    if (gpGlobals->g.PlayerRoles != NULL)
        h = REPLAY_HashBytes(h, gpGlobals->g.PlayerRoles, (uint32_t)sizeof(PLAYERROLES));

    // Session state.
    h = HASH_FIELD(h, gpGlobals->iCurMainMenuItem);
    h = HASH_FIELD(h, gpGlobals->iCurSystemMenuItem);
    h = HASH_FIELD(h, gpGlobals->iCurInvMenuItem);
    h = HASH_FIELD(h, gpGlobals->iCurPlayingRNG);
    h = HASH_FIELD(h, gpGlobals->bCurrentSaveSlot);
    h = HASH_FIELD(h, gpGlobals->fEnteringScene);
    h = HASH_FIELD(h, gpGlobals->fNeedToFadeIn);
    h = HASH_FIELD(h, gpGlobals->fInBattle);
    h = HASH_FIELD(h, gpGlobals->fAutoBattle);
    h = HASH_FIELD(h, gpGlobals->wLastUnequippedItem);
    h = HASH_ARRAY(h, gpGlobals->rgEquipmentEffect);
    h = HASH_ARRAY(h, gpGlobals->rgPlayerStatus);
    h = HASH_FIELD(h, gpGlobals->viewport);
    h = HASH_FIELD(h, gpGlobals->partyoffset);
    h = HASH_FIELD(h, gpGlobals->wLayer);
    h = HASH_FIELD(h, gpGlobals->wMaxPartyMemberIndex);
    h = HASH_ARRAY(h, gpGlobals->rgParty);
    h = HASH_ARRAY(h, gpGlobals->rgTrail);
    h = HASH_FIELD(h, gpGlobals->wPartyDirection);
    h = HASH_FIELD(h, gpGlobals->wNumScene);
    h = HASH_FIELD(h, gpGlobals->wNumPalette);
    h = HASH_FIELD(h, gpGlobals->fNightPalette);
    h = HASH_FIELD(h, gpGlobals->wNumMusic);
    h = HASH_FIELD(h, gpGlobals->wNumBattleMusic);
    h = HASH_FIELD(h, gpGlobals->wNumBattleField);
    h = HASH_FIELD(h, gpGlobals->wCollectValue);
    h = HASH_FIELD(h, gpGlobals->wScreenWave);
    h = HASH_FIELD(h, gpGlobals->sWaveProgression);
    h = HASH_FIELD(h, gpGlobals->wChaseRange);
    h = HASH_FIELD(h, gpGlobals->wChasespeedChangeCycles);
    h = HASH_FIELD(h, gpGlobals->nFollower);
    h = HASH_FIELD(h, gpGlobals->dwCash);
    h = REPLAY_HashBytes(h, &gpGlobals->Exp, (uint32_t)sizeof(gpGlobals->Exp));
    h = HASH_ARRAY(h, gpGlobals->rgPoisonStatus);
    h = HASH_ARRAY(h, gpGlobals->rgInventory);
    h = HASH_FIELD(h, gpGlobals->dwFrameNum);

    // Battle state, while a battle runs.
    if (gpGlobals->fInBattle && g_Battle != NULL)
        h = HashBattle(h);

    // Random numbers consumed so far.
    h = HASH_FIELD(h, UTIL_GetRandomState());
    return h;
}
