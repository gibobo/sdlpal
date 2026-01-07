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

#ifndef UIBATTLE_H
#define UIBATTLE_H

#include "ui.h"
#include <stdint.h>

typedef enum tagBATTLEUISTATE
{
    kBattleUIWait,
    kBattleUISelectMove,
    kBattleUISelectTargetEnemy,
    kBattleUISelectTargetPlayer,
    kBattleUISelectTargetEnemyAll,
    kBattleUISelectTargetPlayerAll,
} BATTLEUISTATE;

typedef enum tagBATTLEMENUSTATE
{
    kBattleMenuMain,
    kBattleMenuMagicSelect,
    kBattleMenuUseItemSelect,
    kBattleMenuThrowItemSelect,
    kBattleMenuMisc,
    kBattleMenuMiscItemSubMenu,
} BATTLEMENUSTATE;

typedef enum tagBATTLEUIACTION
{
    kBattleUIActionAttack,
    kBattleUIActionMagic,
    kBattleUIActionCoopMagic,
    kBattleUIActionMisc,
} BATTLEUIACTION;

#define SPRITENUM_BATTLEICON_ATTACK               40
#define SPRITENUM_BATTLEICON_MAGIC                41
#define SPRITENUM_BATTLEICON_COOPMAGIC            42
#define SPRITENUM_BATTLEICON_MISCMENU             43

#define SPRITENUM_BATTLE_ARROW_CURRENTPLAYER      69
#define SPRITENUM_BATTLE_ARROW_CURRENTPLAYER_RED  68

#define SPRITENUM_BATTLE_ARROW_SELECTEDPLAYER     67
#define SPRITENUM_BATTLE_ARROW_SELECTEDPLAYER_RED 66

#define BATTLEUI_LABEL_ITEM                       5
#define BATTLEUI_LABEL_DEFEND                     58
#define BATTLEUI_LABEL_AUTO                       56
#define BATTLEUI_LABEL_INVENTORY                  57
#define BATTLEUI_LABEL_FLEE                       59
#define BATTLEUI_LABEL_STATUS                     60

#define BATTLEUI_LABEL_USEITEM                    23
#define BATTLEUI_LABEL_THROWITEM                  24

#define TIMEMETER_COLOR_DEFAULT                   0x1B
#define TIMEMETER_COLOR_SLOW                      0x5B
#define TIMEMETER_COLOR_HASTE                     0x2A

#define BATTLEUI_MAX_SHOWNUM                      16

typedef struct tagSHOWNUM
{
    uint16_t wNum;
    uint32_t pos;
    uint32_t dwTime;
    NUMCOLOR color;
} SHOWNUM;

typedef struct tagBATTLEUI
{
    BATTLEUISTATE state;
    BATTLEMENUSTATE MenuState;

    uint32_t dwMsgShowTime;    // the end time of showing the message
    uint16_t wNextMsgDuration; // duration of the next message

    uint16_t wCurPlayerIndex; // index of the current player
    uint16_t wSelectedAction; // current selected action
    int32_t iSelectedIndex;   // current selected index of player or enemy

    uint16_t wActionType; // type of action to be performed
    uint16_t wObjectID;   // object ID of the item or magic to use

    uint8_t fAutoAttack; // TRUE if auto attack

    SHOWNUM rgShowNum[BATTLEUI_MAX_SHOWNUM];
} BATTLEUI;

void PAL_PlayerInfoBox(
    uint32_t pos,
    uint16_t wPlayerRole);

void PAL_BattleUIPlayerReady(
    uint16_t wPlayerIndex);

void PAL_BattleUIUpdate(
    void);

void PAL_BattleUIShowNum(
    uint16_t wNum,
    uint32_t pos,
    NUMCOLOR color);

#endif