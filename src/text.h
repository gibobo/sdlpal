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

#ifndef _TEXT_H
#define _TEXT_H

#include <stdint.h>
#include <wchar.h>

typedef enum tagDIALOGPOSITION
{
    kDialogUpper = 0,
    kDialogCenter,
    kDialogLower,
    kDialogCenterWindow
} DIALOGLOCATION;

typedef struct tagTEXTLIB
{
    int32_t nCurrentDialogLine;
    uint8_t bCurrentFontColor;
    uint32_t posIcon;
    uint32_t posDialogTitle;
    uint32_t posDialogText;
    uint8_t bDialogPosition;
    uint8_t bIcon;
    int32_t iDelayTime;
    int32_t fUserSkip;
    int32_t fPlayingRNG;
    uint8_t bufDialogIcons[282];
} TEXTLIB;

int PAL_InitText(
    void);

void PAL_FreeText(
    void);

const wchar_t *PAL_GetWord(uint32_t iNumWord);

const wchar_t *PAL_GetMsg(uint32_t iNumMsg);

wchar_t *PAL_UnescapeText(const wchar_t *lpszText);

void PAL_DrawText(
    const wchar_t *lpszText,
    uint32_t pos,
    uint8_t bColor,
    uint8_t fShadow,
    uint8_t fUpdate);

void PAL_DrawTextUnescape(
    const wchar_t *lpszText,
    uint32_t pos,
    uint8_t bColor,
    uint8_t fShadow,
    uint8_t fUpdate,
    uint8_t fUnescape);

void PAL_DialogSetDelayTime(
    int iDelayTime);

void PAL_StartDialog(
    uint8_t bDialogLocation,
    uint8_t bFontColor,
    int32_t iNumCharFace,
    int fPlayingRNG);

void PAL_StartDialogWithOffset(
    uint8_t bDialogLocation,
    uint8_t bFontColor,
    int32_t iNumCharFace,
    int32_t fPlayingRNG,
    int32_t xOff,
    int yOff);

int TEXT_DisplayText(
    const wchar_t *lpszText,
    int32_t x,
    int32_t y,
    int isDialog);

void PAL_ShowDialogText(
    const wchar_t *lpszText,
    uint8_t iDialogShadow);

void PAL_ClearDialog(
    char fWaitForKey);

void PAL_EndDialog(
    void);

int PAL_DialogIsPlayingRNG(
    void);

int PAL_swprintf(
    wchar_t *buffer,
    int32_t count,
    const wchar_t *format,
    ...);

#endif