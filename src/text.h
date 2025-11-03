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

#include <wchar.h>

typedef enum tagDIALOGPOSITION
{
    kDialogUpper = 0,
    kDialogCenter,
    kDialogLower,
    kDialogCenterWindow
} DIALOGLOCATION;

typedef struct tagTEXTLIB {
  unsigned int nWords;
  unsigned int nMsgs;

  int nCurrentDialogLine;
  unsigned char bCurrentFontColor;
  unsigned int posIcon;
  unsigned int posDialogTitle;
  unsigned int posDialogText;
  unsigned char bDialogPosition;
  unsigned char bIcon;
  int iDelayTime;
  int fUserSkip;
  int fPlayingRNG;

  unsigned char bufDialogIcons[282];
} TEXTLIB;

int PAL_InitText(
    void);

void PAL_FreeText(
    void);

const wchar_t *PAL_GetWord(unsigned int iNumWord);

const wchar_t *PAL_GetMsg(unsigned int iNumMsg);

wchar_t *PAL_UnescapeText(const wchar_t *lpszText);

void PAL_DrawText(
    const wchar_t *lpszText,
    unsigned int pos,
    unsigned char bColor,
    unsigned char fShadow,
    unsigned char fUpdate);

void PAL_DrawTextUnescape(
    const wchar_t *lpszText,
    unsigned int pos,
    unsigned char bColor,
    unsigned char fShadow,
    unsigned char fUpdate,
    unsigned char fUnescape);

void PAL_DialogSetDelayTime(
    int iDelayTime);

void PAL_StartDialog(
    unsigned char bDialogLocation,
    unsigned char bFontColor,
    int iNumCharFace,
    int fPlayingRNG);

void PAL_StartDialogWithOffset(
    unsigned char bDialogLocation,
    unsigned char bFontColor,
    int iNumCharFace,
    int fPlayingRNG,
    int xOff,
    int yOff);

int TEXT_DisplayText(
    const wchar_t *lpszText,
    int x,
    int y,
    int isDialog);

void PAL_ShowDialogText(
    const wchar_t *lpszText,
    int iDialogShadow);

void PAL_ClearDialog(
    char fWaitForKey);

void PAL_EndDialog(
    void);

int PAL_DialogIsPlayingRNG(
    void);

int PAL_swprintf(
    wchar_t *buffer,
    int count,
    const wchar_t *format,
    ...);

#endif
