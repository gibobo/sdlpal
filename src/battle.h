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

#include "global.h"
#include "uibattle.h"
#include "video.h"

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
    unsigned short wActionID; // item/magic to use
    short sTarget;            // -1 for everyone
    float flRemainingTime;    // remaining waiting time before the action start
} BATTLEACTION;

typedef struct tagBATTLEENEMY
{
    unsigned short wObjectID;             // Object ID of this enemy
    ENEMY e;                              // detailed data of this enemy
    unsigned short rgwStatus[kStatusAll]; // status effects
    float flTimeMeter;                    // time-charging meter (0 = empty, 100 = full).
    POISONSTATUS rgPoisons[MAX_POISONS];  // poisons
    unsigned char *lpSprite;
    unsigned int pos;             // current position on the screen
    unsigned int posOriginal;     // original position on the screen
    unsigned short wCurrentFrame; // current frame number
    FIGHTERSTATE state;           // state of this enemy

    unsigned short wScriptOnTurnStart;
    unsigned short wScriptOnBattleEnd;
    unsigned short wScriptOnReady;

    unsigned short wPrevHP; // HP value prior to action

    int iColorShift;
} BATTLEENEMY;

// We only put some data used in battle here; other data can be accessed in the global data.
typedef struct tagBATTLEPLAYER
{
    int iColorShift;
    float flTimeMeter; // time-charging meter (0 = empty, 100 = full).
    float flTimeSpeedModifier;
    unsigned short wHidingTime; // remaining hiding time
    unsigned char *lpSprite;
    unsigned int pos;             // current position on the screen
    unsigned int posOriginal;     // original position on the screen
    unsigned short wCurrentFrame; // current frame number
    FIGHTERSTATE state;           // state of this player
    BATTLEACTION action;          // action to perform
    BATTLEACTION prevAction;      // action of the previous turn
    int fDefending;               // TRUE if player is defending
    int fSecondAttack;            // FALSE for the first full attack, TRUE for the second full attack
    unsigned short wPrevHP;       // HP value prior to action
    unsigned short wPrevMP;       // MP value prior to action
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
    unsigned short wType;
    unsigned short wObjectIndex;
    unsigned int pos;
    short sLayerOffset;
    int fHaveColorShift;
} BATTLESPRITESEQ;

typedef struct tagSUMMON
{
    unsigned char *lpSprite;
    unsigned short wCurrentFrame;
} SUMMON;

typedef enum tabBATTLEPHASE
{
    kBattlePhaseSelectAction,
    kBattlePhasePerformAction
} BATTLEPHASE;

typedef struct tagACTIONQUEUE
{
    unsigned char fIsEnemy;
    unsigned short wDexterity;
    unsigned short wIndex;
    unsigned char fIsSecond;
} ACTIONQUEUE;

typedef struct tagBATTLE
{
    BATTLEPLAYER rgPlayer[MAX_PLAYERS_IN_PARTY];
    BATTLEENEMY rgEnemy[MAX_ENEMIES_IN_TEAM];

    unsigned short wMaxEnemyIndex;

    VIDEO_Surface *lpSceneBuf;
    VIDEO_Surface *lpBackground;

    short sBackgroundColorShift;

    unsigned char *lpSummonSprite; // sprite of summoned god
    unsigned int posSummon;
    unsigned short iSummonFrame; // current frame of the summoned god
    unsigned char fSummonColorShift;

    int iExpGained;  // total experience value gained
    int iCashGained; // total cash gained

    unsigned char fIsBoss;       // TRUE if boss fight
    unsigned char fEnemyCleared; // TRUE if enemies are cleared
    BATTLERESULT BattleResult;

    float flTimeChargingUnit; // the base waiting time unit

    BATTLEUI UI;

    unsigned char *lpEffectSprite;

    unsigned char fEnemyMoving; // TRUE if enemy is moving

    int iHidingTime; // Time of hiding

    unsigned short wMovingPlayerIndex; // current moving player index

    int iBlow;

    const unsigned char *lpMagicBitmap; // current magic frame bitmap

    BATTLESPRITESEQ SpriteDrawSeq[MAX_BATTLESPRITESEQ_ITEMS];
    unsigned short wMaxSpriteDrawSeqIndex;
    unsigned char fSpriteAddLock;

    BATTLEPHASE Phase;
    ACTIONQUEUE ActionQueue[MAX_ACTIONQUEUE_ITEMS];
    unsigned char iCurAction;
    unsigned char fRepeat;            // TRUE if player pressed Repeat
    unsigned char fForce;             // TRUE if player pressed Force
    unsigned char fFlee;              // TRUE if player pressed Flee
    unsigned char fPrevAutoAtk;       // TRUE if auto-attack was used in the previous turn
    unsigned char fPrevPlayerAutoAtk; // TRUE if auto-attack was used by previous player in the same turn

    unsigned short coopContributors[MAX_PLAYERS_IN_PARTY];
    unsigned char fThisTurnCoop;
} BATTLE;

void PAL_LoadBattleSprites(void);

void PAL_BattleDrawBackground(void);

void PAL_BattleDrawEnemySprites(
    unsigned short wEnemyIndex,
    VIDEO_Surface *lpDstSurface);

void PAL_BattleDrawPlayerSprites(
    unsigned short wPlayerIndex,
    VIDEO_Surface *lpDstSurface);

void PAL_BattleDrawMagicSprites(
    int iMagicNum,
    VIDEO_Surface *lpDstSurface,
    unsigned int pos);

void PAL_BattleClearSpriteObject(void);

void PAL_BattleSpriteAddUnlock(void);

void PAL_BattleAddSpriteObject(
    unsigned short wType,
    unsigned short wObjectIndex,
    unsigned int pos,
    short sLayerOffset,
    int fHaveColorShift);

void PAL_BattleAddFighterSpriteObject(void);

void PAL_BattleSortSpriteObjecByPos(void);

void PAL_BattleDrawAllSprites(void);

void PAL_BattleDrawAllSpritesWithColorShift(unsigned char fColorShift);

void PAL_BattleMakeScene(void);

void PAL_BattleFadeScene(void);

void PAL_BattleEnemyEscape(void);

void PAL_BattlePlayerEscape(void);

BATTLERESULT PAL_StartBattle(
    unsigned short wEnemyTeam,
    unsigned char fIsBoss);

void PAL_GetPlayerPos(
    unsigned char PlayerIndex,
    int *posX,
    int *posY);

#endif
