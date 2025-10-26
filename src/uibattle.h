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

#include <wchar.h>
#include "ui.h"

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

#define SPRITENUM_BATTLEICON_ATTACK 40
#define SPRITENUM_BATTLEICON_MAGIC 41
#define SPRITENUM_BATTLEICON_COOPMAGIC 42
#define SPRITENUM_BATTLEICON_MISCMENU 43

#define SPRITENUM_BATTLE_ARROW_CURRENTPLAYER 69
#define SPRITENUM_BATTLE_ARROW_CURRENTPLAYER_RED 68

#define SPRITENUM_BATTLE_ARROW_SELECTEDPLAYER 67
#define SPRITENUM_BATTLE_ARROW_SELECTEDPLAYER_RED 66

#define BATTLEUI_LABEL_ITEM 5
#define BATTLEUI_LABEL_DEFEND 58
#define BATTLEUI_LABEL_AUTO 56
#define BATTLEUI_LABEL_INVENTORY 57
#define BATTLEUI_LABEL_FLEE 59
#define BATTLEUI_LABEL_STATUS 60

#define BATTLEUI_LABEL_USEITEM 23
#define BATTLEUI_LABEL_THROWITEM 24

#define TIMEMETER_COLOR_DEFAULT 0x1B
#define TIMEMETER_COLOR_SLOW 0x5B
#define TIMEMETER_COLOR_HASTE 0x2A

#define BATTLEUI_MAX_SHOWNUM 16

typedef struct tagSHOWNUM
{
   unsigned short wNum;
   unsigned int pos;
   unsigned long dwTime;
   NUMCOLOR color;
} SHOWNUM;

typedef struct tagBATTLEUI
{
   BATTLEUISTATE state;
   BATTLEMENUSTATE MenuState;

   wchar_t szMsg[32];               // message to be shown on the screen
   wchar_t szNextMsg[32];           // next message to be shown on the screen
   unsigned long dwMsgShowTime;      // the end time of showing the message
   unsigned short wNextMsgDuration; // duration of the next message

   unsigned short wCurPlayerIndex; // index of the current player
   unsigned short wSelectedAction; // current selected action
   int iSelectedIndex;             // current selected index of player or enemy
   int iPrevEnemyTarget;           // previous enemy target

   unsigned short wActionType; // type of action to be performed
   unsigned short wObjectID;   // object ID of the item or magic to use

   int fAutoAttack; // TRUE if auto attack

   SHOWNUM rgShowNum[BATTLEUI_MAX_SHOWNUM];
} BATTLEUI;

void PAL_PlayerInfoBox(
    unsigned int pos,
    unsigned short wPlayerRole);

void PAL_BattleUIShowText(
    const wchar_t *lpszText,
    unsigned short wDuration);

void PAL_BattleUIPlayerReady(
    unsigned short wPlayerIndex);

void PAL_BattleUIUpdate(
    void);

void PAL_BattleUIShowNum(
    unsigned short wNum,
    unsigned int pos,
    NUMCOLOR color);

#endif
