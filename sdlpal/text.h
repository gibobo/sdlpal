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
   kDialogUpper       = 0,
   kDialogCenter,
   kDialogLower,
   kDialogCenterWindow
} DIALOGLOCATION;

typedef enum tagCODEPAGE {
   CP_BIG5 = 0,
   CP_GBK = 1,
   //CP_SHIFTJIS = 2,
   //CP_JISX0208 = 3,
   CP_MAX = CP_GBK + 1,
   CP_UTF_8 = CP_MAX + 1,
   CP_UCS = CP_UTF_8 + 1,
} CODEPAGE;

#ifdef __cplusplus
extern "C" {
#endif

typedef struct tagTEXTLIB
{
    unsigned short*         *lpWordBuf;
    unsigned short*         *lpMsgBuf;

    int             nWords;
    int             nMsgs;
    int             nIndices;

    int             nCurrentDialogLine;
    BYTE            bCurrentFontColor;
    unsigned int           posIcon;
    unsigned int           posDialogTitle;
    unsigned int           posDialogText;
    BYTE            bDialogPosition;
    BYTE            bIcon;
    int             iDelayTime;
    int             iDialogShadow;
    BOOL            fUserSkip;
    BOOL            fPlayingRNG;

    BYTE            bufDialogIcons[282];
} TEXTLIB, *LPTEXTLIB;

extern TEXTLIB         g_TextLib;

extern unsigned short* g_rcCredits[12];

int
PAL_InitText(
   void
);

void
PAL_FreeText(
   void
);

const unsigned short*
PAL_GetWord(
   int        iNumWord
);

const unsigned short*
PAL_GetMsg(
   int        iNumMsg
);

unsigned short*
PAL_UnescapeText(
   const unsigned short*    lpszText
);

void
PAL_DrawText(
   const unsigned short*    lpszText,
   unsigned int      pos,
   BYTE       bColor,
   BOOL       fShadow,
   BOOL       fUpdate,
   BOOL       fUse8x8Font
);

void
PAL_DrawTextUnescape(
   const unsigned short*    lpszText,
   unsigned int      pos,
   BYTE       bColor,
   BOOL       fShadow,
   BOOL       fUpdate,
   BOOL       fUse8x8Font,
   BOOL       fUnescape
);

void
PAL_DialogSetDelayTime(
   int          iDelayTime
);

void
PAL_StartDialog(
   BYTE         bDialogLocation,
   BYTE         bFontColor,
   int          iNumCharFace,
   BOOL         fPlayingRNG
);

void
PAL_StartDialogWithOffset(
   BYTE         bDialogLocation,
   BYTE         bFontColor,
   int          iNumCharFace,
   BOOL         fPlayingRNG,
   int          xOff,
   int          yOff
);

int
TEXT_DisplayText(
   const unsigned short*        lpszText,
   int            x,
   int            y,
   BOOL           isDialog
);

void
PAL_ShowDialogText(
   const unsigned short*    lpszText
);

void
PAL_ClearDialog(
   BOOL         fWaitForKey
);

void
PAL_EndDialog(
   void
);

BOOL
PAL_IsInDialog(
   void
);

BOOL
PAL_DialogIsPlayingRNG(
   void
);

int
PAL_MultiByteToWideChar(
   const char*        mbs,
   int           mbslength,
   unsigned short*        wcs,
   int           wcslength
);

int
PAL_MultiByteToWideCharCP(
	CODEPAGE      cp,
	const char*        mbs,
	size_t        mbslength,
	unsigned short*        wcs,
	size_t        wcslength
	);

CODEPAGE
PAL_GetCodePage(
	void
);

void
PAL_SetCodePage(
	CODEPAGE    uCodePage
);

CODEPAGE
PAL_DetectCodePageForString(
	const char *   text,
	size_t         text_len,
	CODEPAGE       default_cp,
	int *          probability
);

int
PAL_swprintf(
	unsigned short* buffer,
	size_t count,
	const unsigned short* format,
	...
);

#ifdef __cplusplus
}
#endif

#endif
