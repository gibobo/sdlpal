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

#ifndef BATTLE_H
#define BATTLE_H

#include <stdint.h>

#include "global.h"

#define BATTLE_FRAME_TIME            (1000 / 25) // 25 FPS
#define MAX_BATTLE_MAGICSPRITE_ITEMS 3
#define MAX_BATTLESPRITESEQ_ITEMS    (MAX_ENEMIES_IN_TEAM + MAX_PLAYABLE_PLAYER_ROLES + MAX_BATTLE_MAGICSPRITE_ITEMS)
#define MAX_BATTLE_ACTIONS           256
#define MAX_KILLED_ENEMIES           256
#define MAX_ACTIONQUEUE_ITEMS        (MAX_PLAYERS_IN_PARTY + MAX_ENEMIES_IN_TEAM * 2)

typedef enum tagBATTLERESULT
{
    kBattleResultWon = 3,          // player won the battle
    kBattleResultLost = 1,         // player lost the battle
    kBattleResultFleed = 0xFFFF,   // player fleed from the battle
    kBattleResultTerminated = 0,   // battle terminated with scripts
    kBattleResultOnGoing = 1000,   // the battle is ongoing
    kBattleResultPreBattle = 1001, // running pre-battle scripts
    kBattleResultPause = 1002,     // battle pause
} BATTLERESULT;

typedef enum tagFIGHTERSTATE
{
    kFighterWait, // waiting time
    kFighterCom,  // accepting command
    kFighterAct,  // doing the actual move
} FIGHTERSTATE;

typedef enum tagBATTLEACTIONTYPE
{
    kBattleActionPass,       // do nothing
    kBattleActionDefend,     // defend
    kBattleActionAttack,     // physical attack
    kBattleActionMagic,      // use magic
    kBattleActionCoopMagic,  // use cooperative magic
    kBattleActionFlee,       // flee from the battle
    kBattleActionThrowItem,  // throw item onto enemy
    kBattleActionUseItem,    // use item
    kBattleActionAttackMate, // attack teammate (confused only)
} BATTLEACTIONTYPE;

typedef struct tagBATTLEACTION
{
    BATTLEACTIONTYPE ActionType;
    uint16_t wActionID;    // item/magic to use
    int16_t sTarget;       // -1 for everyone
    float flRemainingTime; // remaining waiting time before the action start
} BATTLEACTION;

typedef struct tagBATTLEENEMY
{
    uint16_t wObjectID;                  // Object ID of this enemy
    ENEMY e;                             // detailed data of this enemy
    uint16_t rgwStatus[kStatusAll];      // status effects
    float flTimeMeter;                   // time-charging meter (0 = empty, 100 = full).
    POISONSTATUS rgPoisons[MAX_POISONS]; // poisons
    uint8_t *lpSprite;
    uint32_t pos;           // current position on the screen
    uint32_t posOriginal;   // original position on the screen
    uint16_t wCurrentFrame; // current frame number
    FIGHTERSTATE state;     // state of this enemy

    uint16_t wScriptOnTurnStart;
    uint16_t wScriptOnBattleEnd;
    uint16_t wScriptOnReady;

    uint16_t wPrevHP; // HP value prior to action

    int32_t iColorShift;
} BATTLEENEMY;

// We only put some data used in battle here; other data can be accessed in the global data.
typedef struct tagBATTLEPLAYER
{
    int32_t iColorShift;
    float flTimeMeter; // time-charging meter (0 = empty, 100 = full).
    float flTimeSpeedModifier;
    uint16_t wHidingTime; // remaining hiding time
    uint8_t *lpSprite;
    uint32_t pos;            // current position on the screen
    uint32_t posOriginal;    // original position on the screen
    uint16_t wCurrentFrame;  // current frame number
    FIGHTERSTATE state;      // state of this player
    BATTLEACTION action;     // action to perform
    BATTLEACTION prevAction; // action of the previous turn
    int32_t fDefending;      // TRUE if player is defending
    int32_t fSecondAttack;   // FALSE for the first full attack, TRUE for the second full attack
    uint16_t wPrevHP;        // HP value prior to action
    uint16_t wPrevMP;        // MP value prior to action
} BATTLEPLAYER;

typedef enum tagBATTLESPRITETYPE
{
    kBattleSpriteTypeNone,
    kBattleSpriteTypeEnemy,
    kBattleSpriteTypePlayer,
    kBattleSpriteTypeMagic,
} BATTLESPRITETYPE;

typedef struct tagBATTLESPRITESEQ
{
    uint16_t wType;
    uint16_t wObjectIndex;
    uint32_t pos;
    int16_t sLayerOffset;
    int32_t fHaveColorShift;
} BATTLESPRITESEQ;

typedef struct tagSUMMON
{
    uint8_t *lpSprite;
    uint16_t wCurrentFrame;
} SUMMON;

typedef enum tabBATTLEPHASE
{
    kBattlePhaseSelectAction,
    kBattlePhasePerformAction
} BATTLEPHASE;

typedef struct tagACTIONQUEUE
{
    uint8_t fIsEnemy;
    uint16_t wDexterity;
    uint16_t wIndex;
    uint8_t fIsSecond;
} ACTIONQUEUE;

typedef struct tagBATTLE
{
    BATTLEPLAYER rgPlayer[MAX_PLAYERS_IN_PARTY];
    BATTLEENEMY rgEnemy[MAX_ENEMIES_IN_TEAM];

    uint16_t wMaxEnemyIndex;

    uint16_t sBackgroundColorShift;

    uint8_t *lpSummonSprite; // sprite of summoned god
    uint32_t posSummon;
    uint16_t iSummonFrame; // current frame of the summoned god
    uint8_t fSummonColorShift;

    int32_t iExpGained;  // total experience value gained
    int32_t iCashGained; // total cash gained

    uint8_t fIsBoss;       // TRUE if boss fight
    uint8_t fEnemyCleared; // TRUE if enemies are cleared
    BATTLERESULT BattleResult;

    float flTimeChargingUnit; // the base waiting time unit

    uint8_t *lpEffectSprite;

    uint8_t fEnemyMoving; // TRUE if enemy is moving

    int32_t iHidingTime; // Time of hiding

    uint16_t wMovingPlayerIndex; // current moving player index

    int32_t iBlow;

    const uint8_t *lpMagicBitmap; // current magic frame bitmap

    BATTLESPRITESEQ SpriteDrawSeq[MAX_BATTLESPRITESEQ_ITEMS];
    uint16_t wMaxSpriteDrawSeqIndex;
    uint8_t fSpriteAddLock;

    BATTLEPHASE Phase;
    ACTIONQUEUE ActionQueue[MAX_ACTIONQUEUE_ITEMS];
    uint8_t iCurAction;
    uint8_t fRepeat;            // TRUE if player pressed Repeat
    uint8_t fForce;             // TRUE if player pressed Force
    uint8_t fFlee;              // TRUE if player pressed Flee
    uint8_t fPrevAutoAtk;       // TRUE if auto-attack was used in the previous turn
    uint8_t fPrevPlayerAutoAtk; // TRUE if auto-attack was used by previous player in the same turn

    uint16_t coopContributors[MAX_PLAYERS_IN_PARTY];
    uint8_t fThisTurnCoop;
} BATTLE;

void PAL_LoadBattleSprites(void);

void PAL_BattleDrawBackground(void);

void PAL_BattleDrawEnemySprites(
    uint16_t wEnemyIndex,
    void *lpDstSurface);

void PAL_BattleDrawPlayerSprites(
    uint16_t wPlayerIndex,
    void *lpDstSurface);

void PAL_BattleDrawMagicSprites(
    void *lpDstSurface,
    uint32_t pos);

void PAL_BattleClearSpriteObject(void);

void PAL_BattleSpriteAddUnlock(void);

void PAL_BattleAddSpriteObject(
    uint16_t wType,
    uint16_t wObjectIndex,
    uint32_t pos,
    int16_t sLayerOffset,
    int fHaveColorShift);

void PAL_BattleAddFighterSpriteObject(void);

void PAL_BattleSortSpriteObjecByPos(void);

void PAL_BattleDrawAllSprites(void);

void PAL_BattleDrawAllSpritesWithColorShift(uint8_t fColorShift);

void PAL_BattleMakeScene(void);

void PAL_BattleFadeScene(void);

void PAL_BattleEnemyEscape(void);

void PAL_BattlePlayerEscape(void);

BATTLERESULT PAL_StartBattle(
    uint16_t wEnemyTeam,
    uint8_t fIsBoss);

void PAL_GetPlayerPos(
    uint8_t PlayerIndex,
    int32_t *posX,
    int32_t *posY);

void PAL_RLEBlitToBattleSurface(
    const uint8_t *lpBitmapRLE,
    int32_t posX,
    int32_t posY);

void PAL_BattleBackupScreen(void);

void PAL_BattleUpdateScreen(void);

#endif