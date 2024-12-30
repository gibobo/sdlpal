/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
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

#include "common.h"

typedef enum tagDIALOGPOSITION
{
    kDialogUpper = 0,
    kDialogCenter,
    kDialogLower,
    kDialogCenterWindow
} DIALOGLOCATION;

typedef enum tagCODEPAGE
{
    CP_BIG5 = 0,
    CP_GBK = 1,
    // CP_SHIFTJIS = 2,
    // CP_JISX0208 = 3,
    CP_MAX = CP_GBK + 1,
    CP_UTF_8 = CP_MAX + 1,
    CP_UCS = CP_UTF_8 + 1,
} CODEPAGE;

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct tagTEXTLIB {
  wchar_t **lpWordBuf;
  wchar_t **lpMsgBuf;

  int nWords;
  int nMsgs;
  int nIndices;

  int nCurrentDialogLine;
  unsigned char bCurrentFontColor;
  unsigned int posIcon;
  unsigned int posDialogTitle;
  unsigned int posDialogText;
  unsigned char bDialogPosition;
  unsigned char bIcon;
  int iDelayTime;
  int iDialogShadow;
  int fUserSkip;
  int fPlayingRNG;

  unsigned char bufDialogIcons[282];
} TEXTLIB;

extern TEXTLIB g_TextLib;

extern wchar_t *g_rcCredits[12];

int PAL_InitText(
    void);

void PAL_FreeText(
    void);

const wchar_t *PAL_GetWord(int iNumWord);

const wchar_t *PAL_GetMsg(int iNumMsg);

wchar_t *PAL_UnescapeText(const wchar_t *lpszText);

void PAL_DrawText(
    const wchar_t *lpszText,
    unsigned int pos,
    unsigned char bColor,
    int fShadow,
    int fUpdate,
    int fUse8x8Font);

void PAL_DrawTextUnescape(
    const wchar_t *lpszText,
    unsigned int pos,
    unsigned char bColor,
    int fShadow,
    int fUpdate,
    int fUse8x8Font,
    int fUnescape);

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
    const wchar_t *lpszText);

void PAL_ClearDialog(
    int fWaitForKey);

void PAL_EndDialog(
    void);

int PAL_IsInDialog(
    void);

int PAL_DialogIsPlayingRNG(
    void);

int PAL_MultiByteToWideChar(
    const char *mbs,
    int mbslength,
    wchar_t *wcs,
    int wcslength);

int PAL_MultiByteToWideCharCP(
    CODEPAGE cp,
    const char *mbs,
    int mbslength,
    wchar_t *wcs,
    int wcslength);

CODEPAGE
PAL_GetCodePage(
    void);

void PAL_SetCodePage(
    CODEPAGE uCodePage);

CODEPAGE
PAL_DetectCodePageForString(
    const char *text,
    int text_len,
    CODEPAGE default_cp,
    int *probability);

int PAL_swprintf(
    wchar_t *buffer,
    int count,
    const wchar_t *format,
    ...);

#ifdef __cplusplus
}
#endif

#endif
