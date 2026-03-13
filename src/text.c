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
// Portions based on PALx Project by palxex.
// Copyright (c) 2006-2008, Pal Lockheart <palxex@gmail.com>.
//

#include "text.h"
#include "driver.h"
#include "font.h"
#include "global.h"
#include "input.h"
#include "palcommon.h"
#include "palette.h"
#include "resource.h"
#include "ui.h"
#include "util.h"
#include "video.h"
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>

#define FONT_COLOR_DEFAULT    0x4F
#define FONT_COLOR_YELLOW     0x2D
#define FONT_COLOR_RED        0x1A
#define FONT_COLOR_CYAN       0x8D
#define FONT_COLOR_CYAN_ALT   0x8C
#define FONT_COLOR_RED_ALT    0x17
#define INTERNAL_WBUFFER_SIZE (32)

uint8_t g_fUpdatedInBattle = false;
static wchar_t internal_wbuffer[INTERNAL_WBUFFER_SIZE] = {0};
static wchar_t *WordData = NULL;
static wchar_t *MsgData = NULL;
static wchar_t **lpWordBuf = NULL;
static wchar_t **lpMsgBuf = NULL;
static uint32_t nWords = 0;
static uint32_t nMsgs = 0;

TEXTLIB g_TextLib;

int PAL_InitText(void)
/*++
  Purpose:

    Initialize the in-game texts.

  Parameters:

    None.

  Return value:

    0 = success.
    -1 = memory allocation error.

--*/
{
    uint32_t i;
    uint8_t wchar_size = sizeof(wchar_t);

    // Open the word data files.
    {
        uint16_t *pWordData = NULL;
        void *fpWORD = UTIL_fopen(UTIL_Filename("%s/word.bin", CACHES_PATH), "rb");
        uint32_t word_buffer_size = UTIL_FileLength(fpWORD) / sizeof(uint16_t);
        WordData = (wchar_t *)UTIL_calloc(word_buffer_size, wchar_size);
        pWordData = (uint16_t *)WordData;
        for (i = 0, nWords = 0; i < word_buffer_size; i++, pWordData += (wchar_size >> 1))
        {
            UTIL_fread(pWordData, sizeof(uint16_t), 1, fpWORD);
            if (*pWordData == 0)
                nWords++;
        }
        UTIL_fclose(fpWORD);
        lpWordBuf = (wchar_t **)UTIL_calloc(nWords, sizeof(wchar_t *));
        pWordData = (uint16_t *)WordData;
        for (i = 0, nWords = 0; i < word_buffer_size; i++, pWordData += (wchar_size >> 1))
        {
            if (lpWordBuf[nWords] == NULL)
                lpWordBuf[nWords] = (wchar_t *)pWordData;
            if (*pWordData == 0)
                nWords++;
        }
    }
    // Open the message data files.
    {
        uint16_t *pMsgData = NULL;
        void *fpMSG = UTIL_fopen(UTIL_Filename("%s/msg.bin", CACHES_PATH), "rb");
        uint32_t msg_buffer_size = UTIL_FileLength(fpMSG) / sizeof(uint16_t);
        MsgData = (wchar_t *)UTIL_calloc(msg_buffer_size, wchar_size);
        pMsgData = (uint16_t *)MsgData;
        for (i = 0, nMsgs = 0; i < msg_buffer_size; i++, pMsgData += (wchar_size >> 1))
        {
            UTIL_fread(pMsgData, sizeof(uint16_t), 1, fpMSG);
            if (*pMsgData == 0)
                nMsgs++;
        }
        UTIL_fclose(fpMSG);
        lpMsgBuf = (wchar_t **)UTIL_calloc(nMsgs, sizeof(wchar_t *));
        pMsgData = (uint16_t *)MsgData;
        for (i = 0, nMsgs = 0; i < msg_buffer_size; i++, pMsgData += (wchar_size >> 1))
        {
            if (lpMsgBuf[nMsgs] == NULL)
                lpMsgBuf[nMsgs] = (wchar_t *)pMsgData;
            if (*pMsgData == 0)
                nMsgs++;
        }
    }

    g_TextLib.bCurrentFontColor = FONT_COLOR_DEFAULT;
    g_TextLib.bIcon = 0;
    g_TextLib.posIcon = 0;
    g_TextLib.nCurrentDialogLine = 0;
    g_TextLib.iDelayTime = 3;
    g_TextLib.posDialogTitle = PAL_XY(12, 8);
    g_TextLib.posDialogText = PAL_XY(44, 26);
    g_TextLib.bDialogPosition = kDialogUpper;
    g_TextLib.fUserSkip = false;
    RES_MKFReadChunk(g_TextLib.bufDialogIcons, sizeof(g_TextLib.bufDialogIcons), 12, Res_DATA);

    return 0;
}

void PAL_FreeText(
    void)
/*++
  Purpose:

    Free the memory used by the texts.

  Parameters:

    None.

  Return value:

    None.

--*/
{
    UTIL_free(WordData);
    UTIL_free(MsgData);
    UTIL_free(lpWordBuf);
    UTIL_free(lpMsgBuf);

    WordData = NULL;
    MsgData = NULL;
    lpWordBuf = NULL;
    lpMsgBuf = NULL;
}

const wchar_t *PAL_GetWord(uint32_t iNumWord)
/*++
  Purpose:

    Get the specified word.

  Parameters:

    [IN]  wNumWord - the number of the requested word.

  Return value:

    Pointer to the requested word. NULL if not found.

--*/
{
    return (iNumWord >= nWords || !lpWordBuf[iNumWord]) ? L"" : lpWordBuf[iNumWord];
}

const wchar_t *PAL_GetMsg(uint32_t iNumMsg)
/*++
  Purpose:

    Get the specified message.

  Parameters:

    [IN]  wNumMsg - the number of the requested message.

  Return value:

    Pointer to the requested message. NULL if not found.

--*/
{
    return (iNumMsg >= nMsgs || !lpMsgBuf[iNumMsg]) ? L"" : lpMsgBuf[iNumMsg];
}

wchar_t *PAL_UnescapeText(const wchar_t *lpszText)
{
    wchar_t *buf = internal_wbuffer;

    if (wcsstr(lpszText, L"\\") == NULL)
        return (wchar_t *)lpszText;

    memset(internal_wbuffer, 0, sizeof(wchar_t) * INTERNAL_WBUFFER_SIZE);

    while (*lpszText != L'\0')
    {
        switch (*lpszText)
        {
            case '-':
            case '\'':
            case '@':
            case '\"':
            case '$':
            case '~':
            case ')':
            case '(':
                lpszText++;
                break;
            case '\\':
                lpszText++;
            default:
                wcsncpy(buf++, lpszText++, 1);
                break;
        }
    }
    return internal_wbuffer;
}

void PAL_DrawText(
    const wchar_t *lpszText,
    uint32_t pos,
    uint8_t bColor,
    uint8_t fShadow,
    uint8_t fUpdate)
{
    PAL_DrawTextUnescape(lpszText, pos, bColor, fShadow, fUpdate, true);
}

void PAL_DrawTextUnescape(
    const wchar_t *lpszText,
    uint32_t pos,
    uint8_t bColor,
    uint8_t fShadow,
    uint8_t fUpdate,
    uint8_t fUnescape)
/*++
  Purpose:

    Draw text on the screen.

  Parameters:

    [IN]  lpszText - the text to be drawn.

    [IN]  pos - Position of the text.

    [IN]  bColor - Color of the text.

    [IN]  fShadow - true if the text is shadowed or not.

    [IN]  fUpdate - true if update the screen area.

    [IN]  fUnescape - true if unescaping needed.

  Return value:

    None.

--*/
{
    uint16_t fontX = (uint16_t)PAL_X(pos);
    uint16_t fontY = (uint16_t)PAL_Y(pos);
    uint8_t char_width;
    VIDEO_Rect urect;
    urect.x = fontX;
    urect.y = fontY;
    urect.h = FONT_HEIGHT;
    urect.w = 0;

    if (lpszText == NULL || *lpszText == L'\0')
        return;

    // Handle text overflow
    if (fontX >= SCREEN_W)
        return;

    if (fUnescape)
        lpszText = PAL_UnescapeText(lpszText);

    while (*lpszText)
    {
        // Draw the character
        char_width = PAL_CharWidth(*lpszText) << 3;
        if (char_width)
        {
            PAL_DrawCharOnSurface(*lpszText, fontX, fontY, bColor, fShadow);
            fontX += char_width;
            urect.w += char_width;
            *lpszText++;
        }
    }

    // Update the screen area
    if (fUpdate && urect.w > 0)
    {
        if (fShadow)
        {
            urect.w++;
            urect.h++;
        }
        urect.x = max(urect.x - 10, 0);
        urect.y = max(urect.y - 10, 0);
        urect.w = min(SCREEN_W - urect.x, urect.w + 20);
        urect.h = min(SCREEN_H - urect.y, urect.h + 20);
        // VIDEO_UpdateScreen(&urect);
    }
}

void PAL_DialogSetDelayTime(
    int iDelayTime)
/*++
  Purpose:

    Set the delay time for dialog.

  Parameters:

    [IN]  iDelayTime - the delay time to be set.

  Return value:

    None.

--*/
{
    g_TextLib.iDelayTime = iDelayTime;
}

void PAL_StartDialog(
    uint8_t bDialogLocation,
    uint8_t bFontColor,
    int32_t iNumCharFace,
    int fPlayingRNG)
{
    PAL_StartDialogWithOffset(bDialogLocation, bFontColor, iNumCharFace, fPlayingRNG, 0, 0);
}

void PAL_StartDialogWithOffset(
    uint8_t bDialogLocation,
    uint8_t bFontColor,
    int32_t iNumCharFace,
    int32_t fPlayingRNG,
    int32_t xOff,
    int yOff)
/*++
  Purpose:

    Start a new dialog.

  Parameters:

    [IN]  bDialogLocation - the location of the text on the screen.

    [IN]  bFontColor - the font color of the text.

    [IN]  iNumCharFace - number of the character face in RGM.MKF.

    [IN]  fPlayingRNG - whether we are playing a RNG video or not.

  Return value:

    None.

--*/
{
    uint8_t *buf = NULL;
    uint32_t buf_size = RES_MKFCreateChunk((void **)&buf, iNumCharFace, Res_RGM);
    VIDEO_Rect rect;

    if (gpGlobals->fInBattle && !g_fUpdatedInBattle)
    {
        // Update the screen in battle, or the graphics may seem messed up
        // VIDEO_UpdateScreen(NULL);
        g_fUpdatedInBattle = true;
    }

    g_TextLib.bIcon = 0;
    g_TextLib.posIcon = 0;
    g_TextLib.nCurrentDialogLine = 0;
    g_TextLib.posDialogTitle = PAL_XY(12, 8);
    g_TextLib.fUserSkip = false;

    if (bFontColor != 0)
    {
        g_TextLib.bCurrentFontColor = bFontColor;
    }

    if (fPlayingRNG && iNumCharFace)
    {
        VIDEO_BackupScreen(gpScreen);
        g_TextLib.fPlayingRNG = true;
    }

    switch (bDialogLocation)
    {
        case kDialogUpper:
            if (iNumCharFace > 0)
            {
                // Display the character face at the upper part of the screen
                if (RES_MKFReadChunk(buf, buf_size, iNumCharFace, Res_RGM))
                {
                    rect.w = (short)PAL_RLEGetWidth(buf);
                    rect.h = (short)PAL_RLEGetHeight(buf);
                    rect.x = (short)max(48 - rect.w / 2 + xOff, 0);
                    rect.y = (short)max(55 - rect.h / 2 + yOff, 0);
                    PAL_RLEBlitToSurface(buf, gpScreen, PAL_XY(rect.x, rect.y));
                    // VIDEO_UpdateScreen(&rect);
                }
            }
            g_TextLib.posDialogTitle = PAL_XY(iNumCharFace > 0 ? 80 : 12, 8);
            g_TextLib.posDialogText = PAL_XY(iNumCharFace > 0 ? 96 : 44, 26);
            break;

        case kDialogCenter:
            g_TextLib.posDialogText = PAL_XY(80, 40);
            break;

        case kDialogLower:
            if (iNumCharFace > 0)
            {
                // Display the character face at the lower part of the screen
                if (RES_MKFReadChunk(buf, buf_size, iNumCharFace, Res_RGM))
                {
                    rect.x = (short)(270 - PAL_RLEGetWidth(buf) / 2 + xOff);
                    rect.y = (short)(144 - PAL_RLEGetHeight(buf) / 2 + yOff);
                    PAL_RLEBlitToSurface(buf, gpScreen, PAL_XY(rect.x, rect.y));
                    // VIDEO_UpdateScreen(NULL);
                }
            }
            g_TextLib.posDialogTitle = PAL_XY(iNumCharFace > 0 ? 4 : 12, 108);
            g_TextLib.posDialogText = PAL_XY(iNumCharFace > 0 ? 20 : 44, 126);
            break;

        case kDialogCenterWindow:
            g_TextLib.posDialogText = PAL_XY(160, 40);
            break;
    }

    g_TextLib.posDialogTitle = PAL_XY_OFFSET(g_TextLib.posDialogTitle, xOff, yOff);
    g_TextLib.posDialogText = PAL_XY_OFFSET(g_TextLib.posDialogText, xOff, yOff);

    g_TextLib.bDialogPosition = bDialogLocation;
    UTIL_free(buf);
}

static void
PAL_DialogWaitForKeyWithMaximumSeconds(
    float fMaxSeconds)
/*++
  Purpose:

    Wait for player to press a key after showing a dialog.

  Parameters:

    None.

  Return value:

    None.

--*/
{
    uint8_t new_palette[PALETTE_SIZE];
    uint8_t *org_palette = NULL;
    uint8_t t[3];
    int32_t i;
    uint32_t dwTime = UTIL_GetMilliseconds() + (uint32_t)(1000U * fMaxSeconds);

    // get the current palette
    org_palette = PAL_GetPalette(gpGlobals->wNumPalette, gpGlobals->fNightPalette);
    memcpy(new_palette, org_palette, PALETTE_SIZE);

    if (g_TextLib.bDialogPosition != kDialogCenterWindow &&
        g_TextLib.bDialogPosition != kDialogCenter)
    {
        // show the icon
        const uint8_t *p = PAL_SpriteGetFrame(g_TextLib.bufDialogIcons, g_TextLib.bIcon);
        if (p != NULL)
        {
            VIDEO_Rect rect;

            rect.x = PAL_X(g_TextLib.posIcon);
            rect.y = PAL_Y(g_TextLib.posIcon);
            rect.w = 16;
            rect.h = 16;

            PAL_RLEBlitToSurface(p, gpScreen, g_TextLib.posIcon);
            // VIDEO_UpdateScreen(&rect);
        }
    }

    while ((fMaxSeconds == 0.0f || UTIL_GetMilliseconds() <= dwTime) &&
           (UTIL_WaitKeys(FRAME_TIME, 0) == kKeyNone))
    {
        if (g_TextLib.bDialogPosition != kDialogCenterWindow &&
            g_TextLib.bDialogPosition != kDialogCenter)
        {
            // palette shift
            t[0] = new_palette[0xF9 * 3 + 0];
            t[1] = new_palette[0xF9 * 3 + 1];
            t[2] = new_palette[0xF9 * 3 + 2];
            for (i = 0xF9; i < 0xFE; i++)
            {
                new_palette[i * 3 + 0] = new_palette[i * 3 + 3];
                new_palette[i * 3 + 1] = new_palette[i * 3 + 4];
                new_palette[i * 3 + 2] = new_palette[i * 3 + 5];
            }
            new_palette[0xFE * 3 + 0] = t[0];
            new_palette[0xFE * 3 + 1] = t[1];
            new_palette[0xFE * 3 + 2] = t[2];

            DRIVER_UpdatePalette(new_palette);
            VIDEO_UpdateScreen(NULL);
        }
    }

    if (g_TextLib.bDialogPosition != kDialogCenterWindow &&
        g_TextLib.bDialogPosition != kDialogCenter)
    {
        DRIVER_UpdatePalette(org_palette);
        // VIDEO_UpdateScreen(NULL);
    }

    // PAL_ClearKeyState();

    g_TextLib.fUserSkip = false;
}

int TEXT_DisplayText(
    const wchar_t *lpszText,
    int32_t x,
    int32_t y,
    int isDialog)
{
    //
    // normal texts
    //
    wchar_t text[2];
    uint8_t color;
    uint8_t isNumber = 0;

    while (lpszText != NULL && *lpszText != '\0')
    {
        switch (*lpszText)
        {
            case '-':
                //
                // Set the font color to Cyan
                //
                g_TextLib.bCurrentFontColor = (g_TextLib.bCurrentFontColor == FONT_COLOR_CYAN) ? FONT_COLOR_DEFAULT : FONT_COLOR_CYAN;
                lpszText++;
                break;
            case '\'':
                //
                // Set the font color to Red
                //
                g_TextLib.bCurrentFontColor = (g_TextLib.bCurrentFontColor == FONT_COLOR_RED) ? FONT_COLOR_DEFAULT : FONT_COLOR_RED;
                lpszText++;
                break;
            case '@':
                //
                // Set the font color to Red
                //
                g_TextLib.bCurrentFontColor = (g_TextLib.bCurrentFontColor == FONT_COLOR_RED_ALT) ? FONT_COLOR_DEFAULT : FONT_COLOR_RED_ALT;
                lpszText++;
                break;
            case '\"':
                //
                // Set the font color to Yellow
                //
                if (!isDialog)
                {
                    g_TextLib.bCurrentFontColor = (g_TextLib.bCurrentFontColor == FONT_COLOR_YELLOW) ? FONT_COLOR_DEFAULT : FONT_COLOR_YELLOW;
                }
                lpszText++;
                break;

            case '$':
                //
                // Set the delay time of text-displaying
                //
                g_TextLib.iDelayTime = wcstol(lpszText + 1, NULL, 10) * 10 / 7;
                lpszText += 3;
                break;

            case '~':
                //
                // Delay for a period and quit
                //
                if (g_TextLib.fUserSkip)
                {
                    VIDEO_UpdateScreen(NULL);
                }
                if (!isDialog)
                    UTIL_Delay(wcstol(lpszText + 1, NULL, 10) * 80 / 7);
                g_TextLib.nCurrentDialogLine = -1;
                g_TextLib.fUserSkip = false;
                return x; // don't go further

            case ')':
                //
                // Set the waiting icon
                //
                g_TextLib.bIcon = 1;
                lpszText++;
                break;

            case '(':
                //
                // Set the waiting icon
                //
                g_TextLib.bIcon = 2;
                lpszText++;
                break;

            case '\\':
                lpszText++;

            default:
                text[0] = *lpszText++;
                text[1] = 0;

                color = g_TextLib.bCurrentFontColor;
                if (isDialog)
                {
                    if (g_TextLib.bCurrentFontColor == FONT_COLOR_DEFAULT)
                        color = 0;
                    if (text[0] >= '0' && text[0] <= '9')
                    {
                        isNumber = 1;
                    }
                    else
                    {
                        isNumber = 0;
                    }
                }

                // Update the screen on each draw operation is time-consuming, so disable it if user want to skip
                if (isNumber)
                    PAL_DrawNumber(text[0] - '0', 1, PAL_XY(x, y + 4), kNumColorYellow, kNumAlignLeft);
                else
                    PAL_DrawTextUnescape(text, PAL_XY(x, y), color, !isDialog, !isDialog && !g_TextLib.fUserSkip, false);
                x += (PAL_CharWidth(text[0]) << 3);

                if (!isDialog && !g_TextLib.fUserSkip)
                {
                    VIDEO_UpdateScreen(NULL);
                    UTIL_Delay(g_TextLib.iDelayTime * 8);
                    if (PAL_GetKeyInput() & (kKeySearch | kKeyMenu))
                    {
                        // User pressed a key to skip the dialog
                        g_TextLib.fUserSkip = true;
                    }
                }
        }
    }
    return x;
}

void PAL_ShowDialogText(
    const wchar_t *lpszText,
    uint8_t iDialogShadow)
/*++
  Purpose:

    Show one line of the dialog text.

  Parameters:

    [IN]  lpszText - the text to be shown.

  Return value:

    None.

--*/
{
    VIDEO_Rect rect;
    int32_t x, y;

    // PAL_ClearKeyState();
    g_TextLib.bIcon = 0;

    if (gpGlobals->fInBattle && !g_fUpdatedInBattle)
    {
        //
        // Update the screen in battle, or the graphics may seem messed up
        //
        // VIDEO_UpdateScreen(NULL);
        g_fUpdatedInBattle = true;
    }

    if (g_TextLib.nCurrentDialogLine > 3)
    {
        //
        // The rest dialogs should be shown in the next page.
        //
        PAL_DialogWaitForKeyWithMaximumSeconds(0.0f);
        g_TextLib.nCurrentDialogLine = 0;
        VIDEO_RestoreScreen(gpScreen);
        // VIDEO_UpdateScreen(NULL);
    }

    x = PAL_X(g_TextLib.posDialogText);
    y = PAL_Y(g_TextLib.posDialogText) + g_TextLib.nCurrentDialogLine * 18;

    if (g_TextLib.bDialogPosition == kDialogCenterWindow)
    {
        //
        // The text should be shown in a small window at the center of the screen
        //
        uint32_t len = PAL_TextWidth(lpszText) >> 3;

        // Create the window box
        rect.x = PAL_X(g_TextLib.posDialogText) - len * 4;
        rect.y = PAL_Y(g_TextLib.posDialogText);
        rect.w = SCREEN_W - rect.x * 2 + 32;
        rect.h = 64;

        // Follow behavior of original version
        PAL_CreateSingleLineBoxWithShadow(PAL_XY(rect.x, rect.y), (len + 1) / 2, NULL, iDialogShadow);

        // VIDEO_UpdateScreen(&rect);

        // Show the text on the screen
        TEXT_DisplayText(lpszText, rect.x + 8 + ((len & 1) << 2), rect.y + 10, true);
        VIDEO_UpdateScreen(&rect);

        PAL_DialogWaitForKeyWithMaximumSeconds(1.4f);

        // VIDEO_UpdateScreen(&rect);

        PAL_EndDialog();
    }
    else
    {
        int32_t len = (int)wcslen(lpszText);
        if (g_TextLib.nCurrentDialogLine == 0 &&
            g_TextLib.bDialogPosition != kDialogCenter &&
            (lpszText[len - 1] == 0xff1a ||
             lpszText[len - 1] == 0x2236 || // Special case for Pal WIN95 Simplified Chinese version
             lpszText[len - 1] == ':'))
        {
            //
            // name of character
            //
            PAL_DrawText(lpszText, g_TextLib.posDialogTitle, FONT_COLOR_CYAN_ALT, true, true);
        }
        else
        {
            if (!g_TextLib.fPlayingRNG && g_TextLib.nCurrentDialogLine == 0)
            {
                //
                // Save the screen before we show the first line of dialog
                //
                VIDEO_BackupScreen(gpScreen);
            }

            x = TEXT_DisplayText(lpszText, x, y, false);

            // and update the full screen at once after all texts are drawn
            if (g_TextLib.fUserSkip)
            {
                // VIDEO_UpdateScreen(NULL);
            }

            g_TextLib.posIcon = PAL_XY(x, y);
            g_TextLib.nCurrentDialogLine++;
        }
    }
}

void PAL_ClearDialog(
    char fWaitForKey)
/*++
  Purpose:

    Clear the state of the dialog.

  Parameters:

    [IN]  fWaitForKey - whether wait for any key or not.

  Return value:

    None.

--*/
{
    if (g_TextLib.nCurrentDialogLine > 0 && fWaitForKey)
    {
        PAL_DialogWaitForKeyWithMaximumSeconds(0.0f);
    }

    g_TextLib.nCurrentDialogLine = 0;

    if (g_TextLib.bDialogPosition == kDialogCenter)
    {
        g_TextLib.posDialogTitle = PAL_XY(12, 8);
        g_TextLib.posDialogText = PAL_XY(44, 26);
        g_TextLib.bCurrentFontColor = FONT_COLOR_DEFAULT;
        g_TextLib.bDialogPosition = kDialogUpper;
    }
}

void PAL_EndDialog(
    void)
/*++
  Purpose:

    Ends a dialog.

  Parameters:

    None.

  Return value:

    None.

--*/
{
    PAL_ClearDialog(true);

    //
    // Set some default parameters, as there are some parts of script
    // which doesn't have a "start dialog" instruction before showing the dialog.
    //
    g_TextLib.posDialogTitle = PAL_XY(12, 8);
    g_TextLib.posDialogText = PAL_XY(44, 26);
    g_TextLib.bCurrentFontColor = FONT_COLOR_DEFAULT;
    g_TextLib.bDialogPosition = kDialogUpper;
    g_TextLib.fUserSkip = false;
    g_TextLib.fPlayingRNG = false;
}

int PAL_DialogIsPlayingRNG(
    void)
/*++
  Purpose:

    Check if the script used the RNG playing parameter when displaying texts.

  Parameters:

    None.

  Return value:

    true if the script used the RNG playing parameter, false if not.

--*/
{
    return g_TextLib.fPlayingRNG;
}

int PAL_swprintf(
    wchar_t *buffer,
    int32_t count,
    const wchar_t *format,
    ...)
/*++
  Purpose:

    Formatted wide-character output conversion that output Chinese characters correctly.
    This function supported a subset of format strings that are commonly supported by
    various C libraries, which can be formalized as following:

    %[flags] [width] [.precision] [{h | l | ll}] type

    When handling '%c' and '%s', this function follows the Linux's library convention,
    which means '%c' and '%s' always output multi-byte strings, and '%lc' and '%ls'
    always output wide-char strings.

  Parameters:

    [IN]  buffer - Storage location for output.
    [IN]  count - Length of the output buffer in characters including the termination null one.
    [IN]  format - Format-control string.
    [IN]  ... - Optional arguments.

  Return value:

    The length of outputed wide string, not including the termination null character.

--*/
{
    va_list ap;
    const wchar_t *const format_end = format + wcslen(format);
    const wchar_t *const buffer_end = buffer + count - 1;
    wchar_t chr_buf[2] = {0, 0};
    const wchar_t *fmt_start = NULL;
    wchar_t *cur_fmt = NULL;
    int32_t fmt_len = 0;
    int32_t state, precision = 0, width = 0;
    int32_t left_aligned = 0, wide = 0, narrow = 0;
    int32_t width_var = 0, precision_var = 0, precision_defined = 0;

    // Buffer & length check
    if (buffer == NULL || format == NULL)
    {
        return -1;
    }

    if (buffer_end <= buffer)
        return 0;

    va_start(ap, format);

    count = 0;
    state = 0;
    while (buffer < buffer_end && format < format_end)
    {
        switch (state)
        {
            case 0: // Outside format spec
                if (*format != L'%')
                {
                    *buffer++ = *format++;
                    count++;
                }
                else
                {
                    fmt_start = format++;
                    left_aligned = wide = narrow = 0;
                    precision_var = width_var = 0;
                    precision_defined = 0;
                    state = 1;
                }
                continue;
            case 1: // [flags]
                switch (*format)
                {
                    case L'-':
                        left_aligned = 1;
                    case L'+':
                    case L' ':
                    case L'#':
                    case L'0':
                        format++;
                        continue;
                    default:
                        state = 2;
                        width = width_var = 0;
                }
            case 2: // [width]
                switch (*format)
                {
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                        if (width >= 0)
                            width = width * 10 + (*format - L'0');
                        format++;
                        continue;
                    case '*':
                        if (width == 0)
                            width_var = 1;
                        format++;
                        continue;
                    case '.':
                        format++;
                        precision = precision_var = 0;
                        precision_defined = 1;
                        state = 3;
                        continue;
                    default:
                        state = 4;
                        continue;
                }
            case 3: // [.precision]
                switch (*format)
                {
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                        if (precision >= 0)
                            precision = precision * 10 + (*format - L'0');
                        format++;
                        continue;
                    case '*':
                        if (precision == 0)
                            precision_var = 1;
                        format++;
                        continue;
                    default:
                        state = 4;
                }
            case 4: // [{h | l | ll}]
                switch (*format)
                {
                    case 'l':
                        if (narrow == 0)
                            wide++;
                        format++;
                        continue;
                    case 'h':
                        if (wide == 0)
                            narrow++;
                        format++;
                        continue;
                    default:
                        state = 5;
                }
            case 5: // type
                if (*format == 'c' || *format == 's')
                {
                    // We handle char & str specially
                    wchar_t *buf;
                    int32_t len;
                    int32_t i;

                    // Check width
                    if (width_var)
                    {
                        width = va_arg(ap, int);
                        left_aligned = (width < 0);
                        width = left_aligned ? -width : width;
                    }
                    // Although precision has no meaning to '%c' output, however
                    // the argument still needs to be read if '.*' is provided
                    if (precision_var)
                        precision = va_arg(ap, int);
                    else if (!precision_defined)
                        precision = INT_MAX;

                    if (*format == 's')
                    {
                        buf = va_arg(ap, wchar_t *);
                        len = (int)wcslen(buf);
                    }
                    else
                    {
                        // For ANSI character, put it into the internal buffer
                        if (wide)
                            chr_buf[0] = (wchar_t)va_arg(ap, wchar_t);
                        else
                            chr_buf[0] = (wchar_t)va_arg(ap, int);
                        buf = chr_buf;
                        len = 1;
                    }

                    // Limit output length no longer then precision
                    if (precision > len)
                        precision = len;

                    // Left-side padding
                    for (i = 0; !left_aligned && i < width - precision && buffer < buffer_end; i++)
                    {
                        *buffer++ = L' ';
                        count++;
                    }

                    // Do not overflow the output buffer
                    if (buffer + precision > buffer_end)
                        precision = (int)(buffer_end - buffer);

                    // Convert or copy string (char) into output buffer
                    wcsncpy(buffer, buf, precision);
                    buffer += precision;
                    count += precision;

                    // Right-side padding
                    for (i = 0; left_aligned && i < width - precision && buffer < buffer_end; i++)
                    {
                        *buffer++ = L' ';
                        count++;
                    }
                }
                else
                {
                    // For other types, pass them directly into vswprintf
                    int32_t cur_cnt = 0;
                    va_list apd;

                    // We copy this argument's format string into internal buffer
                    if (fmt_len < (int)(format - fmt_start + 1))
                    {
                        UTIL_free(cur_fmt);
                        fmt_len = (int)(format - fmt_start) + 2;
                        cur_fmt = (wchar_t *)UTIL_calloc(fmt_len, sizeof(wchar_t));
                    }
                    wcsncpy(cur_fmt, fmt_start, fmt_len);
                    cur_fmt[fmt_len] = L'\0';
                    // And pass it into vswprintf to get the output
                    va_copy(apd, ap);
                    cur_cnt = vswprintf(buffer, buffer_end - buffer, cur_fmt, apd);
                    va_end(apd);
                    buffer += cur_cnt;
                    count += cur_cnt;

                    // Then we need to move the argument pointer into next one
                    // Check if width/precision should be read from arguments
                    if (width_var)
                        va_arg(ap, int);
                    if (precision_var)
                        va_arg(ap, int);

                    // Move pointer to pass the actual value argument
                    switch (*format)
                    {
                        case 'd':
                        case 'i':
                        case 'o':
                        case 'u':
                        case 'x':
                        case 'X':
                            if (wide == 1)
                                va_arg(ap, long);
                            else if (wide >= 2)
                                va_arg(ap, int64_t);
                            else
                                va_arg(ap, int);
                            break;
                        case 'e':
                        case 'E':
                        case 'f':
                        case 'g':
                        case 'G':
                        case 'a':
                        case 'A':
                            va_arg(ap, double);
                            break;
                        case 'p':
                        case 'n':
                            va_arg(ap, void *);
                            break;
                    }
                }
                state = 0;
                format++;
                break;
        }
    }

    // If the format string is malformed, try to copy it into the dest buffer
    if (state && buffer < buffer_end)
    {
        int32_t fmt_len2 = (int)(format - fmt_start);
        int32_t buf_len = (int)(buffer_end - buffer);
        if (fmt_len2 <= buf_len)
        {
            wcsncpy(buffer, fmt_start, fmt_len2);
            buffer += fmt_len2;
        }
        else
        {
            wcsncpy(buffer, fmt_start, buf_len);
            buffer += buf_len;
        }
    }

    // NULL-terminate the string
    *buffer = L'\0';

    va_end(ap);
    return count;
}
