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

//
// SOME NOTES ON "AUTO SCRIPT" AND "TRIGGER SCRIPT":
//
// Auto scripts are executed automatically in each frame.
//
// Trigger scripts are only executed when the event is triggered (player touched
// an event object, player triggered an event script by pressing Spacebar).
//

// maximum number of players in party
#define MAX_PLAYERS_IN_PARTY 3

// total number of possible player roles
#define MAX_PLAYER_ROLES 6

// totally number of playable player roles
#define MAX_PLAYABLE_PLAYER_ROLES 5

// maximum entries of inventory
#define MAX_INVENTORY 256

// maximum items in a store
#define MAX_STORE_ITEM 9

// total number of magic attributes
#define NUM_MAGIC_ELEMENTAL 5

// maximum number of enemies in a team
#define MAX_ENEMIES_IN_TEAM 5

// maximum number of equipments for a player
#define MAX_PLAYER_EQUIPMENTS 6

// maximum number of magics for a player
#define MAX_PLAYER_MAGICS 32

// maximum number of scenes
#define MAX_SCENES 300

// maximum number of objects
#define MAX_OBJECTS 600

// maximum number of event objects (should be somewhat more than the original,
// as there are some modified versions which has more)
#define MAX_EVENT_OBJECTS 5500

// maximum number of effective poisons to players
#define MAX_POISONS 16

// maximum number of level
#define MAX_LEVELS 99

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
   short sVanishTime;                        // vanish time (?)
   unsigned short x;                         // X coordinate on the map
   unsigned short y;                         // Y coordinate on the map
   short sLayer;                             // layer value
   unsigned short wTriggerScript;            // Trigger script entry
   unsigned short wAutoScript;               // Auto script entry
   short sState;                             // state of this object
   unsigned short wTriggerMode;              // trigger mode
   unsigned short wSpriteNum;                // number of the sprite
   unsigned short nSpriteFrames;             // total number of frames of the sprite
   unsigned short wDirection;                // direction
   unsigned short wCurrentFrameNum;          // current frame number
   unsigned short nScriptIdleFrame;          // count of idle frames, used by trigger script
   unsigned short wSpritePtrOffset;          // FIXME: ???
   unsigned short nSpriteFramesAuto;         // total number of frames of the sprite, used by auto script
   unsigned short wScriptIdleFrameCountAuto; // count of idle frames, used by auto script
} EVENTOBJECT;

typedef struct tagSCENE
{
   unsigned short wMapNum;           // number of the map
   unsigned short wScriptOnEnter;    // when entering this scene, execute script from here
   unsigned short wScriptOnTeleport; // when teleporting out of this scene, execute script from here
   unsigned short wEventObjectIndex; // event objects in this scene begins from number wEventObjectIndex + 1
} SCENE;

// object including system strings, players, items, magics, enemies and poison scripts.

// system strings and players
typedef struct tagOBJECT_PLAYER
{
   unsigned short wReserved[2];         // always zero
   unsigned short wScriptOnFriendDeath; // when friends in party dies, execute script from here
   unsigned short wScriptOnDying;       // when dying, execute script from here
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
   unsigned short wBitmap;        // bitmap number in BALL.MKF
   unsigned short wPrice;         // price
   unsigned short wScriptOnUse;   // script executed when using this item
   unsigned short wScriptOnEquip; // script executed when equipping this item
   unsigned short wScriptOnThrow; // script executed when throwing this item to enemy
   unsigned short wFlags;         // flags
} OBJECT_ITEM_DOS;

// items
typedef struct tagOBJECT_ITEM
{
   unsigned short wBitmap;        // bitmap number in BALL.MKF
   unsigned short wPrice;         // price
   unsigned short wScriptOnUse;   // script executed when using this item
   unsigned short wScriptOnEquip; // script executed when equipping this item
   unsigned short wScriptOnThrow; // script executed when throwing this item to enemy
   unsigned short wScriptDesc;    // description script
   unsigned short wFlags;         // flags
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
   unsigned short wMagicNumber;     // magic number, according to DATA.MKF #3
   unsigned short wReserved1;       // always zero
   unsigned short wScriptOnSuccess; // when magic succeed, execute script from here
   unsigned short wScriptOnUse;     // when use this magic, execute script from here
   unsigned short wReserved2;       // always zero
   unsigned short wFlags;           // flags
} OBJECT_MAGIC_DOS;

// magics
typedef struct tagOBJECT_MAGIC
{
   unsigned short wMagicNumber;     // magic number, according to DATA.MKF #3
   unsigned short wReserved1;       // always zero
   unsigned short wScriptOnSuccess; // when magic succeed, execute script from here
   unsigned short wScriptOnUse;     // when use this magic, execute script from here
   unsigned short wScriptDesc;      // description script
   unsigned short wReserved2;       // always zero
   unsigned short wFlags;           // flags
} OBJECT_MAGIC;

// enemies
typedef struct tagOBJECT_ENEMY
{
   unsigned short wEnemyID;             // ID of the enemy, according to DATA.MKF #1.
                                        // Also indicates the bitmap number in ABC.MKF.
   unsigned short wResistanceToSorcery; // resistance to sorcery and poison (0 min, 10 max)
   unsigned short wScriptOnTurnStart;   // script executed when turn starts
   unsigned short wScriptOnBattleEnd;   // script executed when battle ends
   unsigned short wScriptOnReady;       // script executed when the enemy is ready
} OBJECT_ENEMY;

// poisons (scripts executed in each round)
typedef struct tagOBJECT_POISON
{
   unsigned short wPoisonLevel;  // level of the poison
   unsigned short wColor;        // color of avatars
   unsigned short wPlayerScript; // script executed when player has this poison (per round)
   unsigned short wReserved;     // always zero
   unsigned short wEnemyScript;  // script executed when enemy has this poison (per round)
} OBJECT_POISON;

typedef union tagOBJECT_DOS
{
   unsigned short rgwData[6];
   OBJECT_PLAYER player;
   OBJECT_ITEM_DOS item;
   OBJECT_MAGIC_DOS magic;
   OBJECT_ENEMY enemy;
   OBJECT_POISON poison;
} OBJECT_DOS;

typedef union tagOBJECT
{
   unsigned short rgwData[7];
   OBJECT_PLAYER player;
   OBJECT_ITEM item;
   OBJECT_MAGIC magic;
   OBJECT_ENEMY enemy;
   OBJECT_POISON poison;
} OBJECT;

typedef struct tagSCRIPTENTRY
{
   unsigned short wOperation;    // operation code
   unsigned short rgwOperand[3]; // operands
} SCRIPTENTRY;

typedef struct tagINVENTORY
{
   unsigned short wItem;        // item object code
   unsigned short nAmount;      // amount of this item
   unsigned short nAmountInUse; // in-use amount of this item
} INVENTORY;

typedef struct tagSTORE
{
   unsigned short rgwItems[MAX_STORE_ITEM];
} STORE;

typedef struct tagENEMY
{
   unsigned short wIdleFrames;    // total number of frames when idle
   unsigned short wMagicFrames;   // total number of frames when using magics
   unsigned short wAttackFrames;  // total number of frames when doing normal attack
   unsigned short wIdleAnimSpeed; // speed of the animation when idle
   unsigned short wActWaitFrames; // FIXME: ???
   unsigned short wYPosOffset;
   short wAttackSound;                                  // sound played when this enemy uses normal attack
   short wActionSound;                                  // FIXME: ???
   short wMagicSound;                                   // sound played when this enemy uses magic
   short wDeathSound;                                   // sound played when this enemy dies
   short wCallSound;                                    // sound played when entering the battle
   unsigned short wHealth;                              // total HP of the enemy
   unsigned short wExp;                                 // How many EXPs we'll get for beating this enemy
   unsigned short wCash;                                // how many cashes we'll get for beating this enemy
   unsigned short wLevel;                               // this enemy's level
   unsigned short wMagic;                               // this enemy's magic number
   unsigned short wMagicRate;                           // chance for this enemy to use magic
   unsigned short wAttackEquivItem;                     // equivalence item of this enemy's normal attack
   unsigned short wAttackEquivItemRate;                 // chance for equivalence item
   unsigned short wStealItem;                           // which item we'll get when stealing from this enemy
   unsigned short nStealItem;                           // total amount of the items which can be stolen
   unsigned short wAttackStrength;                      // normal attack strength
   unsigned short wMagicStrength;                       // magical attack strength
   unsigned short wDefense;                             // resistance to all kinds of attacking
   unsigned short wDexterity;                           // dexterity
   unsigned short wFleeRate;                            // chance for successful fleeing
   unsigned short wPoisonResistance;                    // resistance to poison
   unsigned short wElemResistance[NUM_MAGIC_ELEMENTAL]; // resistance to elemental magics
   unsigned short wPhysicalResistance;                  // resistance to physical attack
   unsigned short wDualMove;                            // whether this enemy can do dual move or not
   unsigned short wCollectValue;                        // value for collecting this enemy for items
} ENEMY;

typedef struct tagENEMYTEAM
{
   unsigned short rgwEnemy[MAX_ENEMIES_IN_TEAM];
} ENEMYTEAM;

typedef unsigned short PLAYERS[MAX_PLAYER_ROLES];

typedef struct tagPLAYERROLES
{
   PLAYERS rgwAvatar;                                                            // avatar (shown in status view)
   PLAYERS rgwSpriteNumInBattle;                                                 // sprite displayed in battle (in F.MKF)
   PLAYERS rgwSpriteNum;                                                         // sprite displayed in normal scene (in MGO.MKF)
   PLAYERS rgwName;                                                              // name of player class (in unsigned short.DAT)
   PLAYERS rgwAttackAll;                                                         // whether player can attack everyone in a bulk or not
   PLAYERS rgwUnknown1;                                                          // FIXME: ???
   PLAYERS rgwLevel;                                                             // level
   PLAYERS rgwMaxHP;                                                             // maximum HP
   PLAYERS rgwMaxMP;                                                             // maximum MP
   PLAYERS rgwHP;                                                                // current HP
   PLAYERS rgwMP;                                                                // current MP
   unsigned short rgwEquipment[MAX_PLAYER_EQUIPMENTS][MAX_PLAYER_ROLES];         // equipments
   PLAYERS rgwAttackStrength;                                                    // normal attack strength
   PLAYERS rgwMagicStrength;                                                     // magical attack strength
   PLAYERS rgwDefense;                                                           // resistance to all kinds of attacking
   PLAYERS rgwDexterity;                                                         // dexterity
   PLAYERS rgwFleeRate;                                                          // chance of successful fleeing
   PLAYERS rgwPoisonResistance;                                                  // resistance to poison
   unsigned short rgwElementalResistance[NUM_MAGIC_ELEMENTAL][MAX_PLAYER_ROLES]; // resistance to elemental magics
   PLAYERS rgwUnknown2;                                                          // FIXME: ???
   PLAYERS rgwUnknown3;                                                          // FIXME: ???
   PLAYERS rgwUnknown4;                                                          // FIXME: ???
   PLAYERS rgwCoveredBy;                                                         // who will cover me when I am low of HP or not sane
   unsigned short rgwMagic[MAX_PLAYER_MAGICS][MAX_PLAYER_ROLES];                 // magics
   PLAYERS rgwWalkFrames;                                                        // walk frame (???)
   PLAYERS rgwCooperativeMagic;                                                  // cooperative magic
   PLAYERS rgwUnknown5;                                                          // FIXME: ???
   PLAYERS rgwUnknown6;                                                          // FIXME: ???
   PLAYERS rgwDeathSound;                                                        // sound played when player dies
   PLAYERS rgwAttackSound;                                                       // sound played when player attacks
   PLAYERS rgwWeaponSound;                                                       // weapon sound (???)
   PLAYERS rgwCriticalSound;                                                     // sound played when player make critical hits
   PLAYERS rgwMagicSound;                                                        // sound played when player is casting a magic
   PLAYERS rgwCoverSound;                                                        // sound played when player cover others
   PLAYERS rgwDyingSound;                                                        // sound played when player is dying
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
   unsigned short wSummonEffect; // summon effect sprite (in F.MKF)
   short sLayerOffset;           // limited to non-summon magic.
                                 // actual layer: PAL_Y(pos) + wYOffset + wMagicLayerOffset
} MAGIC_SPECIAL;

typedef struct tagMAGIC
{
   unsigned short wEffect; // effect sprite
   unsigned short wType;   // type of this magic
   unsigned short wXOffset;
   unsigned short wYOffset;
   MAGIC_SPECIAL rgSpecific;    // have multiple meanings
   short wSpeed;                // speed of the effect
   unsigned short wKeepEffect;  // FIXME: ???
   unsigned short wFireDelay;   // start frame of the magic fire stage
   unsigned short wEffectTimes; // total times of effect
   unsigned short wShake;       // shake screen
   unsigned short wWave;        // wave screen
   unsigned short wUnknown;     // FIXME: ???
   unsigned short wCostMP;      // MP cost
   unsigned short wBaseDamage;  // base damage
   unsigned short wElemental;   // elemental (0 = No Elemental, last = poison)
   short wSound;                // sound played when using this magic
} MAGIC;

typedef struct tagBATTLEFIELD
{
   unsigned short wScreenWave;                // level of screen waving
   short rgsMagicEffect[NUM_MAGIC_ELEMENTAL]; // effect of attributed magics
} BATTLEFIELD;

// magics learned when level up
typedef struct tagLEVELUPMAGIC
{
   unsigned short wLevel; // level reached
   unsigned short wMagic; // magic learned
} LEVELUPMAGIC;

typedef struct tagLEVELUPMAGIC_ALL
{
   LEVELUPMAGIC m[MAX_PLAYABLE_PLAYER_ROLES];
} LEVELUPMAGIC_ALL;

typedef struct tagPALPOS
{
   unsigned short x;
   unsigned short y;
} PALPOS;

// Exp. points needed for the next level
typedef unsigned short LEVELUPEXP;

// game data which is available in data files.
typedef struct tagGAMEDATA
{
   EVENTOBJECT *lprgEventObject;
   int nEventObject;

   SCENE rgScene[MAX_SCENES];
   OBJECT rgObject[MAX_OBJECTS];

   SCRIPTENTRY *lprgScriptEntry;
   int nScriptEntry;

   STORE *lprgStore;
   int nStore;

   ENEMY *lprgEnemy;
   int nEnemy;

   ENEMYTEAM *lprgEnemyTeam;
   int nEnemyTeam;

   PLAYERROLES PlayerRoles;

   MAGIC *lprgMagic;
   int nMagic;

   BATTLEFIELD *lprgBattleField;
   int nBattleField;

   LEVELUPMAGIC_ALL *lprgLevelUpMagic;
   int nLevelUpMagic;

   PALPOS EnemyPos[MAX_ENEMIES_IN_TEAM * MAX_ENEMIES_IN_TEAM];
   LEVELUPEXP rgLevelUpExp[MAX_LEVELS + 1];

   unsigned short rgwBattleEffectIndex[10 * 2];
} GAMEDATA;

typedef struct tagFILES
{
   void *fpFBP;   // battlefield background images
   void *fpMGO;   // sprites in scenes
   void *fpBALL;  // item bitmaps
   void *fpDATA;  // misc data
   void *fpF;     // player sprites during battle
   void *fpFIRE;  // fire effect sprites
   void *fpRGM;   // character face bitmaps
   void *fpSSS;   // script data
} FILES;

// player party
typedef struct tagPARTY
{
   unsigned short wPlayerRole;  // player role
   short x, y;                  // position
   unsigned short wFrame;       // current frame number
   unsigned short wImageOffset; // FIXME: ???
} PARTY;

// player trail, used for other party members to follow the main party member
typedef struct tagTRAIL
{
   unsigned short x, y;       // position
   unsigned short wDirection; // direction
} TRAIL;

typedef struct tagEXPERIENCE
{
   unsigned short wExp; // current experience points
   unsigned short wReserved;
   unsigned short wLevel; // current level
   unsigned short wCount;
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
   unsigned short wPoisonID;     // kind of the poison
   unsigned short wPoisonScript; // script entry
} POISONSTATUS;

typedef struct tagGLOBALVARS
{
   FILES f;
   GAMEDATA g;

   int iCurMainMenuItem;               // current main menu item number
   int iCurSystemMenuItem;             // current system menu item number
   int iCurInvMenuItem;                // current inventory menu item number
   int iCurPlayingRNG;                 // current playing RNG animation
   unsigned char bCurrentSaveSlot;     // current save slot (1-5)
   int fEnteringScene;                 // TRUE if entering a new scene
   int fNeedToFadeIn;                  // TRUE if need to fade in when drawing scene
   int fInBattle;                      // TRUE if in battle
   int fAutoBattle;                    // TRUE if auto-battle
   unsigned short wLastUnequippedItem; // last unequipped item

   PLAYERROLES rgEquipmentEffect[MAX_PLAYER_EQUIPMENTS + 1];    // equipment effects
   unsigned short rgPlayerStatus[MAX_PLAYER_ROLES][kStatusAll]; // player status

   unsigned int viewport; // viewport coordination
   unsigned int partyoffset;
   unsigned short wLayer;
   unsigned short wMaxPartyMemberIndex;      // max index of members in party (0 to MAX_PLAYERS_IN_PARTY - 1)
   PARTY rgParty[MAX_PLAYABLE_PLAYER_ROLES]; // player party
   TRAIL rgTrail[MAX_PLAYABLE_PLAYER_ROLES]; // player trail
   unsigned short wPartyDirection;           // direction of the party
   unsigned short wNumScene;                 // current scene number
   unsigned short wNumPalette;               // current palette number
   int fNightPalette;                        // TRUE if use the darker night palette
   unsigned short wNumMusic;                 // current music number
   unsigned short wNumBattleMusic;           // current music number in battle
   unsigned short wNumBattleField;           // current battle field number
   unsigned short wCollectValue;             // value of "collected" items
   unsigned short wScreenWave;               // level of screen waving
   short sWaveProgression;
   unsigned short wChaseRange;
   unsigned short wChasespeedChangeCycles;
   unsigned short nFollower;

   unsigned int dwCash; // amount of cash

   ALLEXPERIENCE Exp;                                                   // experience status
   POISONSTATUS rgPoisonStatus[MAX_POISONS][MAX_PLAYABLE_PLAYER_ROLES]; // poison status
   INVENTORY rgInventory[MAX_INVENTORY];                                // inventory status
   unsigned int dwFrameNum;
} GLOBALVARS;

typedef struct tagCONFIGURATION {
  /* Configurable options */
  unsigned int dwTextureWidth;
  unsigned int dwTextureHeight;
  int iAudioChannels;
  int iSampleRate;
  unsigned short wAudioBufferSize;
} CONFIGURATION;

extern GLOBALVARS *const gpGlobals;
extern CONFIGURATION gConfig;

int PAL_InitGlobals(
    void);

void PAL_FreeGlobals(
    void);

void PAL_SaveGame(
    int iSaveSlot,
    unsigned short wSavedTimes);

void PAL_InitGameData(
    int iSaveSlot);

void PAL_ReloadInNextTick(
    int iSaveSlot);

int PAL_CountItem(
    unsigned short wObjectID);

int PAL_GetItemIndexToInventory(
    unsigned short wObjectID,
    int *index);

int PAL_AddItemToInventory(
    unsigned short wObjectID,
    int iNum);

int PAL_IncreaseHPMP(
    unsigned short wPlayerRole,
    short sHP,
    short sMP);

int PAL_GetItemAmount(
    unsigned short wItem);

void PAL_UpdateEquipments(
    void);

void PAL_CompressInventory(
    void);

void PAL_RemoveEquipmentEffect(
    unsigned short wPlayerRole,
    unsigned short wEquipPart);

void PAL_AddPoisonForPlayer(
    unsigned short wPlayerRole,
    unsigned short wPoisonID);

void PAL_CurePoisonByKind(
    unsigned short wPlayerRole,
    unsigned short wPoisonID);

void PAL_CurePoisonByLevel(
    unsigned short wPlayerRole,
    unsigned short wMaxLevel);

int PAL_IsPlayerPoisonedByLevel(
    unsigned short wPlayerRole,
    unsigned short wMinLevel);

int PAL_IsPlayerPoisonedByKind(
    unsigned short wPlayerRole,
    unsigned short wPoisonID);

unsigned short
PAL_GetPlayerAttackStrength(
    unsigned short wPlayerRole);

unsigned short
PAL_GetPlayerMagicStrength(
    unsigned short wPlayerRole);

unsigned short
PAL_GetPlayerDefense(
    unsigned short wPlayerRole);

unsigned short
PAL_GetPlayerDexterity(
    unsigned short wPlayerRole);

unsigned short
PAL_GetPlayerFleeRate(
    unsigned short wPlayerRole);

unsigned short
PAL_GetPlayerPoisonResistance(
    unsigned short wPlayerRole);

unsigned short
PAL_GetPlayerElementalResistance(
    unsigned short wPlayerRole,
    int iAttrib);

unsigned short
PAL_GetPlayerBattleSprite(
    unsigned short wPlayerRole);

unsigned short
PAL_GetPlayerCooperativeMagic(
    unsigned short wPlayerRole);

int PAL_PlayerCanAttackAll(
    unsigned short wPlayerRole);

int PAL_AddMagic(
    unsigned short wPlayerRole,
    unsigned short wMagic);

void PAL_RemoveMagic(
    unsigned short wPlayerRole,
    unsigned short wMagic);

int PAL_SetPlayerStatus(
    unsigned short wPlayerRole,
    unsigned short wStatusID,
    unsigned short wNumRound);

void PAL_RemovePlayerStatus(
    unsigned short wPlayerRole,
    unsigned short wStatusID);

void PAL_ClearAllPlayerStatus(
    void);

void PAL_PlayerLevelUp(
    unsigned short wPlayerRole,
    unsigned short wNumLevel);

#endif
