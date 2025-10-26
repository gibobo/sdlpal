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

#ifndef UI_H
#define UI_H

#include <wchar.h>

#define CHUNKNUM_SPRITEUI                9

#define MENUITEM_COLOR                   0x4F
#define MENUITEM_COLOR_INACTIVE          0x18
#define MENUITEM_COLOR_CONFIRMED         0x2C
#define MENUITEM_COLOR_SELECTED_INACTIVE 0x1C
#define MENUITEM_COLOR_SELECTED_FIRST    0xF9
#define MENUITEM_COLOR_SELECTED_TOTALNUM 6U

#define MENUITEM_COLOR_SELECTED      \
    (MENUITEM_COLOR_SELECTED_FIRST + \
     UTIL_GetMilliseconds() / (600U / MENUITEM_COLOR_SELECTED_TOTALNUM) % MENUITEM_COLOR_SELECTED_TOTALNUM)

#define MENUITEM_COLOR_EQUIPPEDITEM   0xC8

#define DESCTEXT_COLOR                0x3C

#define MAINMENU_LABEL_NEWGAME        7
#define MAINMENU_LABEL_LOADGAME       8

#define LOADMENU_LABEL_SLOT_FIRST     43

#define CONFIRMMENU_LABEL_NO          19
#define CONFIRMMENU_LABEL_YES         20

#define CASH_LABEL                    21

#define SWITCHMENU_LABEL_DISABLE      17
#define SWITCHMENU_LABEL_ENABLE       18

#define GAMEMENU_LABEL_STATUS         3
#define GAMEMENU_LABEL_MAGIC          4
#define GAMEMENU_LABEL_INVENTORY      5
#define GAMEMENU_LABEL_SYSTEM         6

#define SYSMENU_LABEL_SAVE            11
#define SYSMENU_LABEL_LOAD            12
#define SYSMENU_LABEL_MUSIC           13
#define SYSMENU_LABEL_SOUND           14
#define SYSMENU_LABEL_QUIT            15

#define INVMENU_LABEL_USE             23
#define INVMENU_LABEL_EQUIP           22

#define STATUS_BACKGROUND_FBPNUM      0
#define STATUS_LABEL_EXP              2
#define STATUS_LABEL_LEVEL            48
#define STATUS_LABEL_HP               49
#define STATUS_LABEL_MP               50
#define STATUS_LABEL_EXP_LAYOUT       29
#define STATUS_LABEL_LEVEL_LAYOUT     30
#define STATUS_LABEL_HP_LAYOUT        31
#define STATUS_LABEL_MP_LAYOUT        32
#define STATUS_LABEL_ATTACKPOWER      51
#define STATUS_LABEL_MAGICPOWER       52
#define STATUS_LABEL_RESISTANCE       53
#define STATUS_LABEL_DEXTERITY        54
#define STATUS_LABEL_FLEERATE         55
#define STATUS_COLOR_EQUIPMENT        0xBE

#define EQUIP_LABEL_HEAD              600
#define EQUIP_LABEL_SHOULDER          601
#define EQUIP_LABEL_BODY              602
#define EQUIP_LABEL_HAND              603
#define EQUIP_LABEL_FOOT              604
#define EQUIP_LABEL_NECK              605

#define BUYMENU_LABEL_CURRENT         35
#define SELLMENU_LABEL_PRICE          25

#define SPRITENUM_SLASH               39
#define SPRITENUM_ITEMBOX             70
#define SPRITENUM_CURSOR_YELLOW_UP    66
#define SPRITENUM_CURSOR_UP           67
#define SPRITENUM_CURSOR_YELLOW       68
#define SPRITENUM_CURSOR              69
#define SPRITENUM_PLAYERINFOBOX       18
#define SPRITENUM_PLAYERFACE_FIRST    48

#define EQUIPMENU_BACKGROUND_FBPNUM   1

#define ITEMUSEMENU_COLOR_STATLABEL   0xBB

#define BATTLEWIN_GETEXP_LABEL        30
#define BATTLEWIN_BEATENEMY_LABEL     9
#define BATTLEWIN_DOLLAR_LABEL        10
#define BATTLEWIN_LEVELUP_LABEL       32
#define BATTLEWIN_ADDMAGIC_LABEL      33
#define BATTLEWIN_LEVELUP_LABEL_COLOR 0xBB
#define SPRITENUM_ARROW               47

#define BATTLE_LABEL_ESCAPEFAIL       31

typedef struct tagBOX
{
    unsigned int pos;
    unsigned short wWidth;
    unsigned short wHeight;
    void *lpSavedArea;
} BOX;

typedef struct tagMENUITEM
{
    unsigned short wValue;
    unsigned short wNumWord;
    int fEnabled;
    unsigned int pos;
} MENUITEM;

#define MENUITEM_VALUE_CANCELLED 0xFFFF

typedef enum tagNUMCOLOR
{
    kNumColorYellow,
    kNumColorBlue,
    kNumColorCyan
} NUMCOLOR;

typedef enum tagNUMALIGN
{
    kNumAlignLeft,
    kNumAlignMid,
    kNumAlignRight
} NUMALIGN;

int PAL_InitUI(
    void);

void PAL_FreeUI(
    void);

void PAL_CreateBox(
    unsigned int pos,
    int nRows,
    int nColumns,
    int iStyle,
    BOX **lpBox);

void PAL_CreateBoxWithShadow(
    unsigned int pos,
    int nRows,
    int nColumns,
    int iStyle,
    BOX **lpBox,
    int nShadowOffset);

void PAL_CreateSingleLineBox(
    unsigned int pos,
    int nLen,
    BOX **lpBox);

void PAL_CreateSingleLineBoxWithShadow(
    unsigned int pos,
    int nLen,
    BOX **lpBox,
    int nShadowOffset);

void PAL_DeleteBox(
    BOX *lpBox);

unsigned short
PAL_ReadMenu(
    void (*lpfnMenuItemChanged)(unsigned short),
    const MENUITEM *rgMenuItem,
    int nMenuItem,
    unsigned short wDefaultItem,
    unsigned char bLabelColor);

void
PAL_DrawNumber(
    unsigned int iNum,
    unsigned int nLength,
    unsigned int pos,
    NUMCOLOR color,
    NUMALIGN align);

unsigned int
PAL_TextWidth(
    const wchar_t *itemText);

unsigned int
PAL_MenuTextMaxWidth(
    const MENUITEM *rgMenuItem,
    unsigned int nMenuItem);

unsigned int
PAL_WordMaxWidth(
    int nFirstWord,
    unsigned int nWordNum);

unsigned int
PAL_WordWidth(
    unsigned int nWordIndex);

extern unsigned char *gpSpriteUI;

#endif
