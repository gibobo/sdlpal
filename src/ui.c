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

#include "ui.h"
#include "font.h"
#include "global.h"
#include "input.h"
#include "palcommon.h"
#include "resource.h"
#include "text.h"
#include "util.h"
#include "video.h"
#include <stdbool.h>

uint8_t *gpSpriteUI = NULL;

static BOX *PAL_CreateBoxInternal(const VIDEO_Rect *rect)
{
    BOX *lpBox = (BOX *)UTIL_calloc(1, sizeof(BOX));
    lpBox->pos = PAL_XY(rect->x, rect->y);
    lpBox->lpSavedArea = VIDEO_DuplicateSurface(rect);
    lpBox->wWidth = (uint16_t)rect->w;
    lpBox->wHeight = (uint16_t)rect->h;
    return lpBox;
}

int PAL_InitUI(
    void)
/*++
  Purpose:

    Initialze the UI subsystem.

  Parameters:

    None.

  Return value:

    0 = success, -1 = fail.

--*/
{
    //
    // Load the UI sprite.
    //
    int32_t iSize = RES_MKFGetChunkSize(CHUNKNUM_SPRITEUI, Res_DATA);
    if (iSize > 0)
    {
        gpSpriteUI = (uint8_t *)UTIL_calloc(1, iSize);
        RES_MKFReadChunk(gpSpriteUI, iSize, CHUNKNUM_SPRITEUI, Res_DATA);
    }
    return (gpSpriteUI != NULL) ? 0 : -1;
}

void PAL_FreeUI(
    void)
/*++
  Purpose:

    Shutdown the UI subsystem.

  Parameters:

    None.

  Return value:

    None.

--*/
{

    UTIL_free(gpSpriteUI);
    gpSpriteUI = NULL;
}

void PAL_CreateBox(
    uint32_t pos,
    int32_t nRows,
    int32_t nColumns,
    int32_t iStyle,
    BOX **lpBox)
{
    PAL_CreateBoxWithShadow(pos, nRows, nColumns, iStyle, lpBox, 6);
}

void PAL_CreateBoxWithShadow(
    uint32_t pos,
    int32_t nRows,
    int32_t nColumns,
    int32_t iStyle,
    BOX **lpBox,
    uint8_t nShadowOffset)
/*++
  Purpose:

    Create a box on the screen.

  Parameters:

    [IN]  pos - position of the box.

    [IN]  nRows - number of rows of the box.

    [IN]  nColumns - number of columns of the box.

    [IN]  iStyle - style of the box (0 or 1).

    [IN]  fSaveScreen - whether save the used screen area or not.

  Return value:

    Pointer to a BOX structure. NULL if failed.
    If fSaveScreen is false, then always returns NULL.

--*/
{
    int32_t i, j, x, m, n;
    const uint8_t *rglpBorderBitmap[3][3];
    VIDEO_Rect rect;

    //
    // Get the bitmaps
    //
    for (i = 0; i < 3; i++)
    {
        for (j = 0; j < 3; j++)
        {
            rglpBorderBitmap[i][j] = PAL_SpriteGetFrame(gpSpriteUI, i * 3 + j + iStyle * 9);
        }
    }

    rect.x = PAL_X(pos);
    rect.y = PAL_Y(pos);
    rect.w = 0;
    rect.h = 0;

    //
    // Get the total width and total height of the box
    //
    for (i = 0; i < 3; i++)
    {
        if (i == 1)
        {
            rect.w += PAL_RLEGetWidth(rglpBorderBitmap[0][i]) * nColumns;
            rect.h += PAL_RLEGetHeight(rglpBorderBitmap[i][0]) * nRows;
        }
        else
        {
            rect.w += PAL_RLEGetWidth(rglpBorderBitmap[0][i]);
            rect.h += PAL_RLEGetHeight(rglpBorderBitmap[i][0]);
        }
    }

    // Include shadow
    rect.w += nShadowOffset;
    rect.h += nShadowOffset;

    if (lpBox)
        *lpBox = PAL_CreateBoxInternal(&rect); // Save the used part of the screen

    //
    // Border takes 2 additional rows and columns...
    //
    nRows += 2;
    nColumns += 2;

    //
    // Draw the box
    //
    for (i = 0; i < nRows; i++)
    {
        x = rect.x;
        m = (i == 0) ? 0 : ((i == nRows - 1) ? 2 : 1);

        for (j = 0; j < nColumns; j++)
        {
            n = (j == 0) ? 0 : ((j == nColumns - 1) ? 2 : 1);
            PAL_RLEBlitToSurfaceWithShadow(rglpBorderBitmap[m][n], gpScreen, PAL_XY(x + nShadowOffset, rect.y + nShadowOffset), true);
            PAL_RLEBlitToSurface(rglpBorderBitmap[m][n], gpScreen, PAL_XY(x, rect.y));
            x += PAL_RLEGetWidth(rglpBorderBitmap[m][n]);
        }

        rect.y += PAL_RLEGetHeight(rglpBorderBitmap[m][0]);
    }
}

void PAL_CreateSingleLineBox(
    uint32_t pos,
    uint16_t nLen,
    BOX **lpBox)
{
    PAL_CreateSingleLineBoxWithShadow(pos, nLen, lpBox, 6);
}

void PAL_CreateSingleLineBoxWithShadow(
    uint32_t pos,
    uint16_t nLen,
    BOX **lpBox,
    uint8_t nShadowOffset)
/*++
  Purpose:

    Create a single-line box on the screen.

  Parameters:

    [IN]  pos - position of the box.

    [IN]  nLen - length of the box.

    [IN]  fSaveScreen - whether save the used screen area or not.

  Return value:

    Pointer to a BOX structure. NULL if failed.
    If fSaveScreen is false, then always returns NULL.

--*/
{
    const uint8_t *lpBitmapLeft;
    const uint8_t *lpBitmapMid;
    const uint8_t *lpBitmapRight;
    VIDEO_Rect rect;
    int32_t i;
    int32_t xSaved;

    //
    // Get the bitmaps
    //
    lpBitmapLeft = PAL_SpriteGetFrame(gpSpriteUI, 44);
    lpBitmapMid = PAL_SpriteGetFrame(gpSpriteUI, 45);
    lpBitmapRight = PAL_SpriteGetFrame(gpSpriteUI, 46);

    rect.x = PAL_X(pos);
    rect.y = PAL_Y(pos);

    //
    // Get the total width and total height of the box
    //
    rect.w = PAL_RLEGetWidth(lpBitmapLeft) + PAL_RLEGetWidth(lpBitmapRight);
    rect.w += PAL_RLEGetWidth(lpBitmapMid) * nLen;
    rect.h = PAL_RLEGetHeight(lpBitmapLeft);

    // Include shadow
    rect.w += nShadowOffset;
    rect.h += nShadowOffset;

    if (lpBox)
    {
        //
        // Save the used part of the screen
        //
        *lpBox = PAL_CreateBoxInternal(&rect);
    }
    xSaved = rect.x;

    //
    // Draw the shadow
    //
    PAL_RLEBlitToSurfaceWithShadow(lpBitmapLeft, gpScreen, PAL_XY(rect.x + nShadowOffset, rect.y + nShadowOffset), true);

    rect.x += PAL_RLEGetWidth(lpBitmapLeft);

    for (i = 0; i < nLen; i++)
    {
        PAL_RLEBlitToSurfaceWithShadow(lpBitmapMid, gpScreen, PAL_XY(rect.x + nShadowOffset, rect.y + nShadowOffset), true);
        rect.x += PAL_RLEGetWidth(lpBitmapMid);
    }

    PAL_RLEBlitToSurfaceWithShadow(lpBitmapRight, gpScreen, PAL_XY(rect.x + nShadowOffset, rect.y + nShadowOffset), true);

    rect.x = xSaved;
    //
    // Draw the box
    //
    PAL_RLEBlitToSurface(lpBitmapLeft, gpScreen, pos);

    rect.x += PAL_RLEGetWidth(lpBitmapLeft);

    for (i = 0; i < nLen; i++)
    {
        PAL_RLEBlitToSurface(lpBitmapMid, gpScreen, PAL_XY(rect.x, rect.y));
        rect.x += PAL_RLEGetWidth(lpBitmapMid);
    }

    PAL_RLEBlitToSurface(lpBitmapRight, gpScreen, PAL_XY(rect.x, rect.y));
}

void PAL_DeleteBox(BOX *lpBox)
/*++
  Purpose:

    Delete a box and restore the saved part of the screen.

  Parameters:

    [IN]  lpBox - pointer to the BOX struct.

  Return value:

    None.

--*/
{
    VIDEO_Rect rect;

    //
    // Check for NULL pointer.
    //
    if (lpBox == NULL)
    {
        return;
    }

    //
    // Restore the saved screen part
    //
    rect.x = PAL_X(lpBox->pos);
    rect.y = PAL_Y(lpBox->pos);
    rect.w = lpBox->wWidth;
    rect.h = lpBox->wHeight;

    VIDEO_CopySurface(lpBox->lpSavedArea, NULL, gpScreen, &rect);

    //
    // Free the memory used by the box
    //
    VIDEO_FreeSurface(lpBox->lpSavedArea);
    UTIL_free(lpBox);
}

uint16_t
PAL_ReadMenu(
    void (*lpfnMenuItemChanged)(uint16_t),
    const MENUITEM *rgMenuItem,
    int32_t nMenuItem,
    uint16_t wDefaultItem,
    uint8_t bLabelColor)
/*++
  Purpose:

    Execute a menu.

  Parameters:

    [IN]  lpfnMenuItemChanged - Callback function which is called when user
                                changed the current menu item.

    [IN]  rgMenuItem - Array of the menu items.

    [IN]  nMenuItem - Number of menu items.

    [IN]  wDefaultItem - default item index.

    [IN]  bLabelColor - color of the labels.

  Return value:

    Return value of the selected menu item. MENUITEM_VALUE_CANCELLED if cancelled.

--*/
{
    int32_t i;
    uint16_t wCurrentItem = (wDefaultItem < nMenuItem) ? wDefaultItem : 0;

    //
    // Fix issue #166
    //
    VIDEO_RenderPaused(true);
    //
    // Draw all the menu texts.
    //
    for (i = 0; i < nMenuItem; i++)
    {
        uint8_t bColor = bLabelColor;

        if (!rgMenuItem[i].fEnabled)
        {
            if (i == wCurrentItem)
            {
                bColor = MENUITEM_COLOR_SELECTED_INACTIVE;
            }
            else
            {
                bColor = MENUITEM_COLOR_INACTIVE;
            }
        }

        PAL_DrawText(PAL_GetWord(rgMenuItem[i].wNumWord), rgMenuItem[i].pos, bColor, true, false);
    }
    //
    // Fix issue #166
    //
    VIDEO_RenderPaused(false);
    // VIDEO_UpdateScreen(NULL);

    if (lpfnMenuItemChanged != NULL)
    {
        (*lpfnMenuItemChanged)(rgMenuItem[wDefaultItem].wValue);
    }

    while (true)
    {
        //
        // Redraw the selected item if needed.
        //
        if (rgMenuItem[wCurrentItem].fEnabled)
        {
            PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
                         rgMenuItem[wCurrentItem].pos, MENUITEM_COLOR_SELECTED, false, true);
        }

        VIDEO_UpdateScreen(NULL);
        PALKEY keys = UTIL_WaitKeys(FRAME_TIME, kKeyMenu | kKeySearch | kKeyLeft | kKeyRight | kKeyUp | kKeyDown);

        if (keys & (kKeyDown | kKeyRight))
        {
            //
            // Fix issue #166
            //
            VIDEO_RenderPaused(true);

            //
            // User pressed the down or right arrow key
            //
            if (rgMenuItem[wCurrentItem].fEnabled)
            {
                //
                // Dehighlight the unselected item.
                //
                PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
                             rgMenuItem[wCurrentItem].pos, bLabelColor, false, true);
            }
            else
            {
                PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
                             rgMenuItem[wCurrentItem].pos, MENUITEM_COLOR_INACTIVE, false, true);
            }

            wCurrentItem++;

            if (wCurrentItem >= nMenuItem)
            {
                wCurrentItem = 0;
            }

            //
            // Highlight the selected item.
            //
            if (rgMenuItem[wCurrentItem].fEnabled)
            {
                PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
                             rgMenuItem[wCurrentItem].pos, MENUITEM_COLOR_SELECTED, false, true);
            }
            else
            {
                PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
                             rgMenuItem[wCurrentItem].pos, MENUITEM_COLOR_SELECTED_INACTIVE, false, true);
            }
            //
            // Fix issue #166
            //
            VIDEO_RenderPaused(false);
            // VIDEO_UpdateScreen(NULL);

            if (lpfnMenuItemChanged != NULL)
            {
                (*lpfnMenuItemChanged)(rgMenuItem[wCurrentItem].wValue);
            }
        }
        else if (keys & (kKeyUp | kKeyLeft))
        {
            //
            // Fix issue #166
            //
            VIDEO_RenderPaused(true);

            //
            // User pressed the up or left arrow key
            //
            if (rgMenuItem[wCurrentItem].fEnabled)
            {
                //
                // Dehighlight the unselected item.
                //
                PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
                             rgMenuItem[wCurrentItem].pos, bLabelColor, false, true);
            }
            else
            {
                PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
                             rgMenuItem[wCurrentItem].pos, MENUITEM_COLOR_INACTIVE, false, true);
            }

            if (wCurrentItem > 0)
            {
                wCurrentItem--;
            }
            else
            {
                wCurrentItem = nMenuItem - 1;
            }

            //
            // Highlight the selected item.
            //
            if (rgMenuItem[wCurrentItem].fEnabled)
            {
                PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
                             rgMenuItem[wCurrentItem].pos, MENUITEM_COLOR_SELECTED, false, true);
            }
            else
            {
                PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
                             rgMenuItem[wCurrentItem].pos, MENUITEM_COLOR_SELECTED_INACTIVE, false, true);
            }
            //
            // Fix issue #166
            //
            VIDEO_RenderPaused(false);
            // VIDEO_UpdateScreen(NULL);

            if (lpfnMenuItemChanged != NULL)
            {
                (*lpfnMenuItemChanged)(rgMenuItem[wCurrentItem].wValue);
            }
        }
        else if (keys & kKeyMenu)
        {
            //
            // User cancelled
            //
            if (rgMenuItem[wCurrentItem].fEnabled)
            {
                PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
                             rgMenuItem[wCurrentItem].pos, bLabelColor, false, true);
            }
            else
            {
                PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
                             rgMenuItem[wCurrentItem].pos, MENUITEM_COLOR_INACTIVE, false, true);
            }

            break;
        }
        else if (keys & kKeySearch)
        {
            //
            // User pressed Enter
            //
            if (rgMenuItem[wCurrentItem].fEnabled)
            {
                PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
                             rgMenuItem[wCurrentItem].pos, MENUITEM_COLOR_CONFIRMED, false, true);

                return rgMenuItem[wCurrentItem].wValue;
            }
        }
    }

    return MENUITEM_VALUE_CANCELLED;
}

void PAL_DrawNumber(
    uint32_t iNum,
    uint32_t nLength,
    uint32_t pos,
    NUMCOLOR color,
    NUMALIGN align)
/*++
  Purpose:

    Draw the specified number with the bitmaps in the UI sprite.

  Parameters:

    [IN]  iNum - the number to be drawn.

    [IN]  nLength - max. length of the number.

    [IN]  pos - position on the screen.

    [IN]  color - color of the number (yellow or blue).

    [IN]  align - align mode of the number.

  Return value:

    None.

--*/
{
    uint32_t nActualLength, i;
    int32_t x, y;
    const uint8_t *rglpBitmap[10];

    //
    // Get the bitmaps. Blue starts from 29, Cyan from 56, Yellow from 19.
    //
    x = (color == kNumColorBlue) ? 29 : ((color == kNumColorCyan) ? 56 : 19);

    for (i = 0; i < 10; i++)
    {
        rglpBitmap[i] = PAL_SpriteGetFrame(gpSpriteUI, (uint32_t)x + i);
    }

    i = iNum;
    nActualLength = 0;

    //
    // Calculate the actual length of the number.
    //
    while (i > 0)
    {
        i /= 10;
        nActualLength++;
    }

    if (nActualLength > nLength)
    {
        nActualLength = nLength;
    }
    else if (nActualLength == 0)
    {
        nActualLength = 1;
    }

    x = PAL_X(pos) - 6;
    y = PAL_Y(pos);

    switch (align)
    {
        case kNumAlignLeft:
            x += 6 * nActualLength;
            break;

        case kNumAlignMid:
            x += 3 * (nLength + nActualLength);
            break;

        case kNumAlignRight:
            x += 6 * nLength;
            break;
    }

    //
    // Draw the number.
    //
    while (nActualLength-- > 0)
    {
        PAL_RLEBlitToSurface(rglpBitmap[iNum % 10], gpScreen, PAL_XY(x, y));
        x -= 6;
        iNum /= 10;
    }
}

uint32_t
PAL_TextWidth(
    const wchar_t *itemText)
/*++
   Purpose:

      Calculate the text width of the given text.

   Parameters:

      [IN]  itemText - Pointer to the text.

   Return value:

      text width.

--*/
{
    const uint32_t l = (uint32_t)wcslen(itemText);
    uint32_t i = 0;
    uint32_t w = 0;

    for (i = 0; i < l; i++)
        w += PAL_CharWidth(itemText[i]);

    return w << 3;
}

uint32_t PAL_MenuTextMaxWidth(
    const MENUITEM *rgMenuItem,
    uint32_t nMenuItem)
/*++
  Purpose:

    Calculate the maximal text width of all the menu items in number of full width characters.

  Parameters:

   [IN]  rgMenuItem - Pointer to the menu item array.
   [IN]  nMenuItem - Number of menu items.

  Return value:

    Maximal text width.

--*/
{
    uint32_t i = 0;
    uint32_t r = 0;
    uint32_t w = 0;
    for (i = 0; i < nMenuItem; i++)
    {
        const wchar_t *itemText = PAL_GetWord(rgMenuItem[i].wNumWord);
        w = (PAL_TextWidth(PAL_UnescapeText(itemText)) + 8U) >> 4U;
        if (r < w)
            r = w;
    }
    return r;
}

uint32_t PAL_WordMaxWidth(
    int32_t nFirstWord,
    uint32_t nWordNum)
/*++
  Purpose:

    Calculate the maximal text width of a specific range of words in number of full width characters.

  Parameters:

    [IN]  nFirstWord - First index of word.
   [IN]  nWordNum - Number of words.

  Return value:

    Maximal text width.

--*/
{
    uint32_t i = 0;
    uint32_t w = 0;
    uint32_t r = 0;
    for (i = 0; i < nWordNum; i++)
    {
        w = PAL_WordWidth(nFirstWord + i);
        if (r < w)
            r = w;
    }
    return r;
}

uint32_t PAL_WordWidth(
    uint32_t nWordIndex)
/*++
  Purpose:

    Calculate the text width of a specific word.

  Parameters:

   [IN]  nWordNum - Index of the word.

  Return value:

    Text width.

--*/
{
    const wchar_t *itemText = PAL_GetWord(nWordIndex);
    return (PAL_TextWidth(itemText) + 8U) >> 4U;
}
