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

#ifndef GLOBAL_H
#define GLOBAL_H

#include <stdint.h>

//
// SOME NOTES ON "AUTO SCRIPT" AND "TRIGGER SCRIPT":
//
// Auto scripts are executed automatically in each frame.
//
// Trigger scripts are only executed when the event is triggered (player touched
// an event object, player triggered an event script by pressing Spacebar).
//

// maximum number of players in party
#define MAX_PLAYERS_IN_PARTY      3

// total number of possible player roles
#define MAX_PLAYER_ROLES          6

// totally number of playable player roles
#define MAX_PLAYABLE_PLAYER_ROLES 5

// maximum entries of inventory
#define MAX_INVENTORY             256

// maximum items in a store
#define MAX_STORE_ITEM            9

// total number of magic attributes
#define NUM_MAGIC_ELEMENTAL       5

// maximum number of enemies in a team
#define MAX_ENEMIES_IN_TEAM       5

// maximum number of equipments for a player
#define MAX_PLAYER_EQUIPMENTS     6

// maximum number of magics for a player
#define MAX_PLAYER_MAGICS         32

// maximum number of scenes
#define MAX_SCENES                300

// maximum number of objects
#define MAX_OBJECTS               600

// maximum number of event objects (should be somewhat more than the original,
// as there are some modified versions which has more)
#define MAX_EVENT_OBJECTS         5074

// maximum number of effective poisons to players
#define MAX_POISONS               16

// maximum number of level
#define MAX_LEVELS                99

#define BATTLEWIN_LEVELUP_MAGIC   20

// status of characters
enum tagSTATUS
{
    kStatusConfused = 0, // attack friends randomly
    kStatusParalyzed,    // paralyzed
    kStatusSleep,        // not allowed to move
    kStatusSilence,      // cannot use magic
    kStatusPuppet,       // for dead players only, continue attacking
    kStatusBravery,      // more power for physical attacks
    kStatusProtect,      // more defense value
    kStatusHaste,        // faster
    kStatusDualAttack,   // dual attack
    kStatusAll
};

// body parts of equipments
enum tagBODYPART
{
    kBodyPartHead = 0,
    kBodyPartBody,
    kBodyPartShoulder,
    kBodyPartHand,
    kBodyPartFeet,
    kBodyPartWear,
    kBodyPartExtra,
};

// state of event object, used by the sState field of the EVENTOBJECT struct
enum tagOBJECTSTATE
{
    kObjStateHidden = 0,
    kObjStateNormal = 1,
    kObjStateBlocker = 2
};

enum tagTRIGGERMODE
{
    kTriggerNone = 0,
    kTriggerSearchNear = 1,
    kTriggerSearchNormal = 2,
    kTriggerSearchFar = 3,
    kTriggerTouchNear = 4,
    kTriggerTouchNormal = 5,
    kTriggerTouchFar = 6,
    kTriggerTouchFarther = 7,
    kTriggerTouchFarthest = 8
};

typedef struct tagEVENTOBJECT
{
    int16_t sVanishTime;                // vanish time (?)
    uint16_t x;                         // X coordinate on the map
    uint16_t y;                         // Y coordinate on the map
    int16_t sLayer;                     // layer value
    uint16_t wTriggerScript;            // Trigger script entry
    uint16_t wAutoScript;               // Auto script entry
    int16_t sState;                     // state of this object
    uint16_t wTriggerMode;              // trigger mode
    uint16_t wSpriteNum;                // number of the sprite
    uint16_t nSpriteFrames;             // total number of frames of the sprite
    uint16_t wDirection;                // direction
    uint16_t wCurrentFrameNum;          // current frame number
    uint16_t nScriptIdleFrame;          // count of idle frames, used by trigger script
    uint16_t wSpritePtrOffset;          // FIXME: ???
    uint16_t nSpriteFramesAuto;         // total number of frames of the sprite, used by auto script
    uint16_t wScriptIdleFrameCountAuto; // count of idle frames, used by auto script
} EVENTOBJECT;

typedef struct tagSCENE
{
    uint16_t wMapNum;           // number of the map
    uint16_t wScriptOnEnter;    // when entering this scene, execute script from here
    uint16_t wScriptOnTeleport; // when teleporting out of this scene, execute script from here
    uint16_t wEventObjectIndex; // event objects in this scene begins from number wEventObjectIndex + 1
} SCENE;

// object including system strings, players, items, magics, enemies and poison scripts.

// system strings and players
typedef struct tagOBJECT_PLAYER
{
    uint16_t wReserved[2];         // always zero
    uint16_t wScriptOnFriendDeath; // when friends in party dies, execute script from here
    uint16_t wScriptOnDying;       // when dying, execute script from here
} OBJECT_PLAYER;

typedef enum tagITEMFLAG
{
    kItemFlagUsable = (1 << 0),
    kItemFlagEquipable = (1 << 1),
    kItemFlagThrowable = (1 << 2),
    kItemFlagConsuming = (1 << 3),
    kItemFlagApplyToAll = (1 << 4),
    kItemFlagSellable = (1 << 5),
    kItemFlagEquipableByPlayerRole_First = (1 << 6)
} ITEMFLAG;

// items
typedef struct tagOBJECT_ITEM_DOS
{
    uint16_t wBitmap;        // bitmap number in BALL.MKF
    uint16_t wPrice;         // price
    uint16_t wScriptOnUse;   // script executed when using this item
    uint16_t wScriptOnEquip; // script executed when equipping this item
    uint16_t wScriptOnThrow; // script executed when throwing this item to enemy
    uint16_t wFlags;         // flags
} OBJECT_ITEM_DOS;

// items
typedef struct tagOBJECT_ITEM
{
    uint16_t wBitmap;        // bitmap number in BALL.MKF
    uint16_t wPrice;         // price
    uint16_t wScriptOnUse;   // script executed when using this item
    uint16_t wScriptOnEquip; // script executed when equipping this item
    uint16_t wScriptOnThrow; // script executed when throwing this item to enemy
    uint16_t wScriptDesc;    // description script
    uint16_t wFlags;         // flags
} OBJECT_ITEM;

typedef enum tagMAGICFLAG
{
    kMagicFlagUsableOutsideBattle = (1 << 0),
    kMagicFlagUsableInBattle = (1 << 1),
    kMagicFlagUsableToEnemy = (1 << 3),
    kMagicFlagApplyToAll = (1 << 4),
} MAGICFLAG;

// magics
typedef struct tagOBJECT_MAGIC_DOS
{
    uint16_t wMagicNumber;     // magic number, according to DATA.MKF #3
    uint16_t wReserved1;       // always zero
    uint16_t wScriptOnSuccess; // when magic succeed, execute script from here
    uint16_t wScriptOnUse;     // when use this magic, execute script from here
    uint16_t wReserved2;       // always zero
    uint16_t wFlags;           // flags
} OBJECT_MAGIC_DOS;

// magics
typedef struct tagOBJECT_MAGIC
{
    uint16_t wMagicNumber;     // magic number, according to DATA.MKF #3
    uint16_t wReserved1;       // always zero
    uint16_t wScriptOnSuccess; // when magic succeed, execute script from here
    uint16_t wScriptOnUse;     // when use this magic, execute script from here
    uint16_t wScriptDesc;      // description script
    uint16_t wReserved2;       // always zero
    uint16_t wFlags;           // flags
} OBJECT_MAGIC;

// enemies
typedef struct tagOBJECT_ENEMY
{
    uint16_t wEnemyID;             // ID of the enemy, according to DATA.MKF #1.
                                   // Also indicates the bitmap number in ABC.MKF.
    uint16_t wResistanceToSorcery; // resistance to sorcery and poison (0 min, 10 max)
    uint16_t wScriptOnTurnStart;   // script executed when turn starts
    uint16_t wScriptOnBattleEnd;   // script executed when battle ends
    uint16_t wScriptOnReady;       // script executed when the enemy is ready
} OBJECT_ENEMY;

// poisons (scripts executed in each round)
typedef struct tagOBJECT_POISON
{
    uint16_t wPoisonLevel;  // level of the poison
    uint16_t wColor;        // color of avatars
    uint16_t wPlayerScript; // script executed when player has this poison (per round)
    uint16_t wReserved;     // always zero
    uint16_t wEnemyScript;  // script executed when enemy has this poison (per round)
} OBJECT_POISON;

typedef union tagOBJECT_DOS
{
    uint16_t rgwData[6];
    OBJECT_PLAYER player;
    OBJECT_ITEM_DOS item;
    OBJECT_MAGIC_DOS magic;
    OBJECT_ENEMY enemy;
    OBJECT_POISON poison;
} OBJECT_DOS;

typedef union tagOBJECT
{
    uint16_t rgwData[7];
    OBJECT_PLAYER player;
    OBJECT_ITEM item;
    OBJECT_MAGIC magic;
    OBJECT_ENEMY enemy;
    OBJECT_POISON poison;
} OBJECT;

typedef struct tagSCRIPTENTRY
{
    uint16_t wOperation;    // operation code
    uint16_t rgwOperand[3]; // operands
} SCRIPTENTRY;

typedef struct tagINVENTORY
{
    uint16_t wItem;        // item object code
    uint16_t nAmount;      // amount of this item
    uint16_t nAmountInUse; // in-use amount of this item
} INVENTORY;

typedef struct tagSTORE
{
    uint16_t rgwItems[MAX_STORE_ITEM];
} STORE;

typedef struct tagENEMY
{
    uint16_t wIdleFrames;    // total number of frames when idle
    uint16_t wMagicFrames;   // total number of frames when using magics
    uint16_t wAttackFrames;  // total number of frames when doing normal attack
    uint16_t wIdleAnimSpeed; // speed of the animation when idle
    uint16_t wActWaitFrames; // FIXME: ???
    uint16_t wYPosOffset;
    int16_t wAttackSound;                          // sound played when this enemy uses normal attack
    int16_t wActionSound;                          // FIXME: ???
    int16_t wMagicSound;                           // sound played when this enemy uses magic
    int16_t wDeathSound;                           // sound played when this enemy dies
    int16_t wCallSound;                            // sound played when entering the battle
    uint16_t wHealth;                              // total HP of the enemy
    uint16_t wExp;                                 // How many EXPs we'll get for beating this enemy
    uint16_t wCash;                                // how many cashes we'll get for beating this enemy
    uint16_t wLevel;                               // this enemy's level
    uint16_t wMagic;                               // this enemy's magic number
    uint16_t wMagicRate;                           // chance for this enemy to use magic
    uint16_t wAttackEquivItem;                     // equivalence item of this enemy's normal attack
    uint16_t wAttackEquivItemRate;                 // chance for equivalence item
    uint16_t wStealItem;                           // which item we'll get when stealing from this enemy
    uint16_t nStealItem;                           // total amount of the items which can be stolen
    uint16_t wAttackStrength;                      // normal attack strength
    uint16_t wMagicStrength;                       // magical attack strength
    uint16_t wDefense;                             // resistance to all kinds of attacking
    uint16_t wDexterity;                           // dexterity
    uint16_t wFleeRate;                            // chance for successful fleeing
    uint16_t wPoisonResistance;                    // resistance to poison
    uint16_t wElemResistance[NUM_MAGIC_ELEMENTAL]; // resistance to elemental magics
    uint16_t wPhysicalResistance;                  // resistance to physical attack
    uint16_t wDualMove;                            // whether this enemy can do dual move or not
    uint16_t wCollectValue;                        // value for collecting this enemy for items
} ENEMY;

typedef struct tagENEMYTEAM
{
    uint16_t rgwEnemy[MAX_ENEMIES_IN_TEAM];
} ENEMYTEAM;

typedef struct tagPLAYERROLES
{
    uint16_t rgwAvatar[MAX_PLAYER_ROLES];                                   // avatar (shown in status view)
    uint16_t rgwSpriteNumInBattle[MAX_PLAYER_ROLES];                        // sprite displayed in battle (in F.MKF)
    uint16_t rgwSpriteNum[MAX_PLAYER_ROLES];                                // sprite displayed in normal scene (in MGO.MKF)
    uint16_t rgwName[MAX_PLAYER_ROLES];                                     // name of player class (in uint16_t.DAT)
    uint16_t rgwAttackAll[MAX_PLAYER_ROLES];                                // whether player can attack everyone in a bulk or not
    uint16_t rgwUnknown1[MAX_PLAYER_ROLES];                                 // FIXME: ???
    uint16_t rgwLevel[MAX_PLAYER_ROLES];                                    // level
    uint16_t rgwMaxHP[MAX_PLAYER_ROLES];                                    // maximum HP
    uint16_t rgwMaxMP[MAX_PLAYER_ROLES];                                    // maximum MP
    uint16_t rgwHP[MAX_PLAYER_ROLES];                                       // current HP
    uint16_t rgwMP[MAX_PLAYER_ROLES];                                       // current MP
    uint16_t rgwEquipment[MAX_PLAYER_EQUIPMENTS][MAX_PLAYER_ROLES];         // equipments
    uint16_t rgwAttackStrength[MAX_PLAYER_ROLES];                           // normal attack strength
    uint16_t rgwMagicStrength[MAX_PLAYER_ROLES];                            // magical attack strength
    uint16_t rgwDefense[MAX_PLAYER_ROLES];                                  // resistance to all kinds of attacking
    uint16_t rgwDexterity[MAX_PLAYER_ROLES];                                // dexterity
    uint16_t rgwFleeRate[MAX_PLAYER_ROLES];                                 // chance of successful fleeing
    uint16_t rgwPoisonResistance[MAX_PLAYER_ROLES];                         // resistance to poison
    uint16_t rgwElementalResistance[NUM_MAGIC_ELEMENTAL][MAX_PLAYER_ROLES]; // resistance to elemental magics
    uint16_t rgwUnknown2[MAX_PLAYER_ROLES];                                 // FIXME: ???
    uint16_t rgwUnknown3[MAX_PLAYER_ROLES];                                 // FIXME: ???
    uint16_t rgwUnknown4[MAX_PLAYER_ROLES];                                 // FIXME: ???
    uint16_t rgwCoveredBy[MAX_PLAYER_ROLES];                                // who will cover me when I am low of HP or not sane
    uint16_t rgwMagic[MAX_PLAYER_MAGICS][MAX_PLAYER_ROLES];                 // magics
    uint16_t rgwWalkFrames[MAX_PLAYER_ROLES];                               // walk frame (???)
    uint16_t rgwCooperativeMagic[MAX_PLAYER_ROLES];                         // cooperative magic
    uint16_t rgwUnknown5[MAX_PLAYER_ROLES];                                 // FIXME: ???
    uint16_t rgwUnknown6[MAX_PLAYER_ROLES];                                 // FIXME: ???
    uint16_t rgwDeathSound[MAX_PLAYER_ROLES];                               // sound played when player dies
    uint16_t rgwAttackSound[MAX_PLAYER_ROLES];                              // sound played when player attacks
    uint16_t rgwWeaponSound[MAX_PLAYER_ROLES];                              // weapon sound (???)
    uint16_t rgwCriticalSound[MAX_PLAYER_ROLES];                            // sound played when player make critical hits
    uint16_t rgwMagicSound[MAX_PLAYER_ROLES];                               // sound played when player is casting a magic
    uint16_t rgwCoverSound[MAX_PLAYER_ROLES];                               // sound played when player cover others
    uint16_t rgwDyingSound[MAX_PLAYER_ROLES];                               // sound played when player is dying
} PLAYERROLES;

typedef enum tagMAGIC_TYPE
{
    kMagicTypeNormal = 0,
    kMagicTypeAttackAll = 1,     // draw the effect on each of the enemies
    kMagicTypeAttackWhole = 2,   // draw the effect on the whole enemy team
    kMagicTypeAttackField = 3,   // draw the effect on the battle field
    kMagicTypeApplyToPlayer = 4, // the magic is used on one player
    kMagicTypeApplyToParty = 5,  // the magic is used on the whole party
    kMagicTypeTrance = 8,        // trance the player
    kMagicTypeSummon = 9,        // summon
} MAGIC_TYPE;

typedef union tagMAGIC_SPECIAL
{
    uint16_t wSummonEffect; // summon effect sprite (in F.MKF)
    int16_t sLayerOffset;   // limited to non-summon magic.
                            // actual layer: PAL_Y(pos) + wYOffset + wMagicLayerOffset
} MAGIC_SPECIAL;

typedef struct tagMAGIC
{
    uint16_t wEffect; // effect sprite
    uint16_t wType;   // type of this magic
    uint16_t wXOffset;
    uint16_t wYOffset;
    MAGIC_SPECIAL rgSpecific; // have multiple meanings
    int16_t wSpeed;           // speed of the effect
    uint16_t wKeepEffect;     // FIXME: ???
    uint16_t wFireDelay;      // start frame of the magic fire stage
    uint16_t wEffectTimes;    // total times of effect
    uint16_t wShake;          // shake screen
    uint16_t wWave;           // wave screen
    uint16_t wUnknown;        // FIXME: ???
    uint16_t wCostMP;         // MP cost
    uint16_t wBaseDamage;     // base damage
    uint16_t wElemental;      // elemental (0 = No Elemental, last = poison)
    int16_t wSound;           // sound played when using this magic
} MAGIC;

typedef struct tagBATTLEFIELD
{
    uint16_t wScreenWave;                        // level of screen waving
    int16_t rgsMagicEffect[NUM_MAGIC_ELEMENTAL]; // effect of attributed magics
} BATTLEFIELD;

// magics learned when level up
typedef struct tagLEVELUPMAGIC
{
    uint16_t wLevel; // level reached
    uint16_t wMagic; // magic learned
} LEVELUPMAGIC;

typedef struct tagLEVELUPMAGIC_ALL
{
    LEVELUPMAGIC m[MAX_PLAYABLE_PLAYER_ROLES];
} LEVELUPMAGIC_ALL;

typedef struct tagPALPOS
{
    uint16_t x;
    uint16_t y;
} PALPOS;

// Exp. points needed for the next level
typedef uint16_t LEVELUPEXP;

// game data which is available in data files.
typedef struct tagGAMEDATA
{
    EVENTOBJECT *lprgEventObject;
    int32_t nEventObject;

    SCENE rgScene[MAX_SCENES];
    OBJECT rgObject[MAX_OBJECTS];

    SCRIPTENTRY *lprgScriptEntry;
    int32_t nScriptEntry;

    STORE *lprgStore;
    int32_t nStore;

    ENEMY *lprgEnemy;
    int32_t nEnemy;

    ENEMYTEAM *lprgEnemyTeam;
    int32_t nEnemyTeam;

    PLAYERROLES *PlayerRoles;
    int32_t nPlayerRoles;

    MAGIC *lprgMagic;
    int32_t nMagic;

    BATTLEFIELD *lprgBattleField;
    int32_t nBattleField;

    LEVELUPMAGIC_ALL *lprgLevelUpMagic;
    int32_t nLevelUpMagic;

    PALPOS EnemyPos[MAX_ENEMIES_IN_TEAM * MAX_ENEMIES_IN_TEAM];
    LEVELUPEXP rgLevelUpExp[MAX_LEVELS + 1];

    uint16_t rgwBattleEffectIndex[10 * 2];
} GAMEDATA;

// player party
typedef struct tagPARTY
{
    uint16_t wPlayerRole;  // player role
    int16_t x, y;          // position
    uint16_t wFrame;       // current frame number
    uint16_t wImageOffset; // FIXME: ???
} PARTY;

// player trail, used for other party members to follow the main party member
typedef struct tagTRAIL
{
    uint16_t x, y;       // position
    uint16_t wDirection; // direction
} TRAIL;

typedef struct tagEXPERIENCE
{
    uint16_t wExp; // current experience points
    uint16_t wReserved;
    uint16_t wLevel; // current level
    uint16_t wCount;
} EXPERIENCE;

typedef struct tagALLEXPERIENCE
{
    EXPERIENCE rgPrimaryExp[MAX_PLAYER_ROLES];
    EXPERIENCE rgHealthExp[MAX_PLAYER_ROLES];
    EXPERIENCE rgMagicExp[MAX_PLAYER_ROLES];
    EXPERIENCE rgAttackExp[MAX_PLAYER_ROLES];
    EXPERIENCE rgMagicPowerExp[MAX_PLAYER_ROLES];
    EXPERIENCE rgDefenseExp[MAX_PLAYER_ROLES];
    EXPERIENCE rgDexterityExp[MAX_PLAYER_ROLES];
    EXPERIENCE rgFleeExp[MAX_PLAYER_ROLES];
} ALLEXPERIENCE;

typedef struct tagPOISONSTATUS
{
    uint16_t wPoisonID;     // kind of the poison
    uint16_t wPoisonScript; // script entry
} POISONSTATUS;

typedef struct tagGLOBALVARS
{
    GAMEDATA g;

    int32_t iCurMainMenuItem;     // current main menu item number
    int32_t iCurSystemMenuItem;   // current system menu item number
    int32_t iCurInvMenuItem;      // current inventory menu item number
    int32_t iCurPlayingRNG;       // current playing RNG animation
    uint8_t bCurrentSaveSlot;     // current save slot (1-5)
    uint8_t fEnteringScene;       // TRUE if entering a new scene
    uint8_t fNeedToFadeIn;        // TRUE if need to fade in when drawing scene
    uint8_t fInBattle;            // TRUE if in battle
    uint8_t fAutoBattle;          // TRUE if auto-battle
    uint16_t wLastUnequippedItem; // last unequipped item

    PLAYERROLES rgEquipmentEffect[MAX_PLAYER_EQUIPMENTS + 1]; // equipment effects
    uint16_t rgPlayerStatus[MAX_PLAYER_ROLES][kStatusAll];    // player status

    uint32_t viewport; // viewport coordination
    uint32_t partyoffset;
    uint16_t wLayer;
    uint16_t wMaxPartyMemberIndex;            // max index of members in party (0 to MAX_PLAYERS_IN_PARTY - 1)
    PARTY rgParty[MAX_PLAYABLE_PLAYER_ROLES]; // player party
    TRAIL rgTrail[MAX_PLAYABLE_PLAYER_ROLES]; // player trail
    uint16_t wPartyDirection;                 // direction of the party
    uint16_t wNumScene;                       // current scene number
    uint16_t wNumPalette;                     // current palette number
    uint8_t fNightPalette;                    // TRUE if use the darker night palette
    uint16_t wNumMusic;                       // current music number
    uint16_t wNumBattleMusic;                 // current music number in battle
    uint16_t wNumBattleField;                 // current battle field number
    uint16_t wCollectValue;                   // value of "collected" items
    uint16_t wScreenWave;                     // level of screen waving
    int16_t sWaveProgression;
    uint16_t wChaseRange;
    uint16_t wChasespeedChangeCycles;
    uint16_t nFollower;

    uint32_t dwCash; // amount of cash

    ALLEXPERIENCE Exp;                                                   // experience status
    POISONSTATUS rgPoisonStatus[MAX_POISONS][MAX_PLAYABLE_PLAYER_ROLES]; // poison status
    INVENTORY rgInventory[MAX_INVENTORY];                                // inventory status
    uint32_t dwFrameNum;
} GLOBALVARS;

extern GLOBALVARS *gpGlobals;

int PAL_InitGlobals(
    void);

void PAL_FreeGlobals(
    void);

void PAL_SaveGame(
    int32_t iSaveSlot,
    uint16_t wSavedTimes);

void PAL_InitGameData(
    int iSaveSlot);

#ifdef __cplusplus
extern "C" {
#endif

void PAL_ReloadInNextTick(uint8_t iSaveSlot);

#ifdef __cplusplus
}
#endif

int PAL_CountItem(
    uint16_t wObjectID);

int PAL_GetItemIndexToInventory(
    uint16_t wObjectID,
    int32_t *index);

int PAL_AddItemToInventory(
    uint16_t wObjectID,
    int iNum);

int PAL_IncreaseHPMP(
    uint16_t wPlayerRole,
    int16_t sHP,
    int16_t sMP);

uint16_t PAL_GetItemAmount(
    uint16_t wItem);

void PAL_UpdateEquipments(
    void);

void PAL_CompressInventory(
    void);

void PAL_RemoveEquipmentEffect(
    uint16_t wPlayerRole,
    uint16_t wEquipPart);

void PAL_AddPoisonForPlayer(
    uint16_t wPlayerRole,
    uint16_t wPoisonID);

void PAL_CurePoisonByKind(
    uint16_t wPlayerRole,
    uint16_t wPoisonID);

void PAL_CurePoisonByLevel(
    uint16_t wPlayerRole,
    uint16_t wMaxLevel);

int PAL_IsPlayerPoisonedByLevel(
    uint16_t wPlayerRole,
    uint16_t wMinLevel);

int PAL_IsPlayerPoisonedByKind(
    uint16_t wPlayerRole,
    uint16_t wPoisonID);

uint16_t
PAL_GetPlayerAttackStrength(
    uint16_t wPlayerRole);

uint16_t
PAL_GetPlayerMagicStrength(
    uint16_t wPlayerRole);

uint16_t
PAL_GetPlayerDefense(
    uint16_t wPlayerRole);

uint16_t
PAL_GetPlayerDexterity(
    uint16_t wPlayerRole);

uint16_t
PAL_GetPlayerFleeRate(
    uint16_t wPlayerRole);

uint16_t
PAL_GetPlayerPoisonResistance(
    uint16_t wPlayerRole);

uint16_t
PAL_GetPlayerElementalResistance(
    uint16_t wPlayerRole,
    int iAttrib);

uint16_t
PAL_GetPlayerBattleSprite(
    uint16_t wPlayerRole);

uint16_t
PAL_GetPlayerCooperativeMagic(
    uint16_t wPlayerRole);

int PAL_PlayerCanAttackAll(
    uint16_t wPlayerRole);

int PAL_AddMagic(
    uint16_t wPlayerRole,
    uint16_t wMagic);

void PAL_RemoveMagic(
    uint16_t wPlayerRole,
    uint16_t wMagic);

int PAL_SetPlayerStatus(
    uint16_t wPlayerRole,
    uint16_t wStatusID,
    uint16_t wNumRound);

void PAL_RemovePlayerStatus(
    uint16_t wPlayerRole,
    uint16_t wStatusID);

void PAL_ClearAllPlayerStatus(
    void);

void PAL_PlayerLevelUp(
    uint16_t wPlayerRole,
    uint16_t wNumLevel);

#endif
