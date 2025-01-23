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
#include "text.h"
#include "util.h"
#include "video.h"
#include "common.h"

unsigned char *gpSpriteUI = NULL;

static BOX *PAL_CreateBoxInternal(
    const PAL_Rect *rect) {
  BOX *lpBox = (BOX *)calloc(1, sizeof(BOX));
  if (lpBox == NULL) {
    return NULL;
  }

  lpBox->pos = PAL_XY(rect->x, rect->y);
  lpBox->lpSavedArea = VIDEO_DuplicateSurface(rect);
  lpBox->wHeight = (unsigned short)rect->w;
  lpBox->wWidth = (unsigned short)rect->h;

  if (lpBox->lpSavedArea == NULL) {
    free(lpBox);
    return NULL;
  }

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
   int iSize;

   //
   // Load the UI sprite.
   //
   iSize = PAL_MKFGetChunkSize(CHUNKNUM_SPRITEUI, gpGlobals->f.fpDATA);
   if (iSize < 0)
   {
      return -1;
   }

   gpSpriteUI = (unsigned char *)calloc(1, iSize);
   if (gpSpriteUI == NULL)
   {
      return -1;
   }

   PAL_MKFReadChunk(gpSpriteUI, iSize, CHUNKNUM_SPRITEUI, gpGlobals->f.fpDATA);

   return 0;
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
   if (gpSpriteUI != NULL)
   {
      free(gpSpriteUI);
      gpSpriteUI = NULL;
   }
}

BOX *PAL_CreateBox(
    unsigned int pos,
    int nRows,
    int nColumns,
    int iStyle,
    int fSaveScreen)
{
   return PAL_CreateBoxWithShadow(pos, nRows, nColumns, iStyle, fSaveScreen, 6);
}

BOX *PAL_CreateBoxWithShadow(
    unsigned int pos,
    int nRows,
    int nColumns,
    int iStyle,
    int fSaveScreen,
    int nShadowOffset)
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
   int i, j, x, m, n;
   const unsigned char *rglpBorderBitmap[3][3];
   BOX *lpBox = NULL;
   PAL_Rect rect;

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

   if (fSaveScreen)
   {
      //
      // Save the used part of the screen
      //
      lpBox = PAL_CreateBoxInternal(&rect);
   }

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
         PAL_RLEBlitToSurfaceWithShadow(rglpBorderBitmap[m][n], gpScreen, PAL_XY(x + nShadowOffset, rect.y + nShadowOffset), TRUE);
         PAL_RLEBlitToSurface(rglpBorderBitmap[m][n], gpScreen, PAL_XY(x, rect.y));
         x += PAL_RLEGetWidth(rglpBorderBitmap[m][n]);
      }

      rect.y += PAL_RLEGetHeight(rglpBorderBitmap[m][0]);
   }

   return lpBox;
}

BOX *PAL_CreateSingleLineBox(
    unsigned int pos,
    int nLen,
    int fSaveScreen)
{
   return PAL_CreateSingleLineBoxWithShadow(pos, nLen, fSaveScreen, 6);
}

BOX *PAL_CreateSingleLineBoxWithShadow(
    unsigned int pos,
    int nLen,
    int fSaveScreen,
    int nShadowOffset)
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
   static const int iNumLeftSprite = 44;
   static const int iNumMidSprite = 45;
   static const int iNumRightSprite = 46;

   const unsigned char *lpBitmapLeft;
   const unsigned char *lpBitmapMid;
   const unsigned char *lpBitmapRight;
   PAL_Rect rect;
   BOX *lpBox = NULL;
   int i;
   int xSaved;

   //
   // Get the bitmaps
   //
   lpBitmapLeft = PAL_SpriteGetFrame(gpSpriteUI, iNumLeftSprite);
   lpBitmapMid = PAL_SpriteGetFrame(gpSpriteUI, iNumMidSprite);
   lpBitmapRight = PAL_SpriteGetFrame(gpSpriteUI, iNumRightSprite);

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

   if (fSaveScreen)
   {
      //
      // Save the used part of the screen
      //
      lpBox = PAL_CreateBoxInternal(&rect);
   }
   xSaved = rect.x;

   //
   // Draw the shadow
   //
   PAL_RLEBlitToSurfaceWithShadow(lpBitmapLeft, gpScreen, PAL_XY(rect.x + nShadowOffset, rect.y + nShadowOffset), TRUE);

   rect.x += PAL_RLEGetWidth(lpBitmapLeft);

   for (i = 0; i < nLen; i++)
   {
      PAL_RLEBlitToSurfaceWithShadow(lpBitmapMid, gpScreen, PAL_XY(rect.x + nShadowOffset, rect.y + nShadowOffset), TRUE);
      rect.x += PAL_RLEGetWidth(lpBitmapMid);
   }

   PAL_RLEBlitToSurfaceWithShadow(lpBitmapRight, gpScreen, PAL_XY(rect.x + nShadowOffset, rect.y + nShadowOffset), TRUE);

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

   return lpBox;
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
   PAL_Rect rect;

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
   PAL_FreeSurface(lpBox->lpSavedArea);
   free(lpBox);
}

unsigned short
PAL_ReadMenu(
    LPITEMCHANGED_CALLBACK lpfnMenuItemChanged,
    LPCMENUITEM rgMenuItem,
    int nMenuItem,
    unsigned short wDefaultItem,
    unsigned char bLabelColor)
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
   int i;
   unsigned short wCurrentItem = (wDefaultItem < nMenuItem) ? wDefaultItem : 0;

   //
   // Fix issue #166
   //
   VIDEO_RenderPaused(TRUE);
   //
   // Draw all the menu texts.
   //
   for (i = 0; i < nMenuItem; i++)
   {
      unsigned char bColor = bLabelColor;

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

      PAL_DrawText(PAL_GetWord(rgMenuItem[i].wNumWord), rgMenuItem[i].pos, bColor, TRUE, TRUE, FALSE);
   }
   //
   // Fix issue #166
   //
   VIDEO_RenderPaused(FALSE);
   VIDEO_UpdateScreen(NULL);

   if (lpfnMenuItemChanged != NULL)
   {
      (*lpfnMenuItemChanged)(rgMenuItem[wDefaultItem].wValue);
   }

   while (TRUE)
   {
      PAL_ClearKeyState();

      //
      // Redraw the selected item if needed.
      //
      if (rgMenuItem[wCurrentItem].fEnabled)
      {
         PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
                      rgMenuItem[wCurrentItem].pos, MENUITEM_COLOR_SELECTED, FALSE, TRUE, FALSE);
      }

      PAL_ProcessEvent();

      if (PAL_GetKeyInput() & (kKeyDown | kKeyRight))
      {
         //
         // Fix issue #166
         //
         VIDEO_RenderPaused(TRUE);

         //
         // User pressed the down or right arrow key
         //
         if (rgMenuItem[wCurrentItem].fEnabled)
         {
            //
            // Dehighlight the unselected item.
            //
            PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
                         rgMenuItem[wCurrentItem].pos, bLabelColor, FALSE, TRUE, FALSE);
         }
         else
         {
            PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
                         rgMenuItem[wCurrentItem].pos, MENUITEM_COLOR_INACTIVE, FALSE, TRUE, FALSE);
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
                         rgMenuItem[wCurrentItem].pos, MENUITEM_COLOR_SELECTED, FALSE, TRUE, FALSE);
         }
         else
         {
            PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
                         rgMenuItem[wCurrentItem].pos, MENUITEM_COLOR_SELECTED_INACTIVE, FALSE, TRUE, FALSE);
         }
         //
         // Fix issue #166
         //
         VIDEO_RenderPaused(FALSE);
         VIDEO_UpdateScreen(NULL);

         if (lpfnMenuItemChanged != NULL)
         {
            (*lpfnMenuItemChanged)(rgMenuItem[wCurrentItem].wValue);
         }
      }
      else if (PAL_GetKeyInput() & (kKeyUp | kKeyLeft))
      {
         //
         // Fix issue #166
         //
         VIDEO_RenderPaused(TRUE);

         //
         // User pressed the up or left arrow key
         //
         if (rgMenuItem[wCurrentItem].fEnabled)
         {
            //
            // Dehighlight the unselected item.
            //
            PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
                         rgMenuItem[wCurrentItem].pos, bLabelColor, FALSE, TRUE, FALSE);
         }
         else
         {
            PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
                         rgMenuItem[wCurrentItem].pos, MENUITEM_COLOR_INACTIVE, FALSE, TRUE, FALSE);
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
                         rgMenuItem[wCurrentItem].pos, MENUITEM_COLOR_SELECTED, FALSE, TRUE, FALSE);
         }
         else
         {
            PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
                         rgMenuItem[wCurrentItem].pos, MENUITEM_COLOR_SELECTED_INACTIVE, FALSE, TRUE, FALSE);
         }
         //
         // Fix issue #166
         //
         VIDEO_RenderPaused(FALSE);
         VIDEO_UpdateScreen(NULL);

         if (lpfnMenuItemChanged != NULL)
         {
            (*lpfnMenuItemChanged)(rgMenuItem[wCurrentItem].wValue);
         }
      }
      else if (PAL_GetKeyInput() & kKeyMenu)
      {
         //
         // User cancelled
         //
         if (rgMenuItem[wCurrentItem].fEnabled)
         {
            PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
                         rgMenuItem[wCurrentItem].pos, bLabelColor, FALSE, TRUE, FALSE);
         }
         else
         {
            PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
                         rgMenuItem[wCurrentItem].pos, MENUITEM_COLOR_INACTIVE, FALSE, TRUE, FALSE);
         }

         break;
      }
      else if (PAL_GetKeyInput() & kKeySearch)
      {
         //
         // User pressed Enter
         //
         if (rgMenuItem[wCurrentItem].fEnabled)
         {
            PAL_DrawText(PAL_GetWord(rgMenuItem[wCurrentItem].wNumWord),
                         rgMenuItem[wCurrentItem].pos, MENUITEM_COLOR_CONFIRMED, FALSE, TRUE, FALSE);

            return rgMenuItem[wCurrentItem].wValue;
         }
      }

      //
      // Use delay function to avoid high CPU usage.
      //
      UTIL_Sleep(50);
   }

   return MENUITEM_VALUE_CANCELLED;
}

void PAL_DrawNumber(
    unsigned int iNum,
    unsigned int nLength,
    unsigned int pos,
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
   unsigned int nActualLength, i;
   int x, y;
   const unsigned char *rglpBitmap[10];

   //
   // Get the bitmaps. Blue starts from 29, Cyan from 56, Yellow from 19.
   //
   x = (color == kNumColorBlue) ? 29 : ((color == kNumColorCyan) ? 56 : 19);

   for (i = 0; i < 10; i++)
   {
      rglpBitmap[i] = PAL_SpriteGetFrame(gpSpriteUI, (unsigned int)x + i);
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

/*++
   Purpose:

      Calculate the text width of the given text.

   Parameters:

      [IN]  itemText - Pointer to the text.

   Return value:

      text width.

--*/
int
PAL_TextWidth(
    const wchar_t *lpszItemText)
{
   int l = (int)wcslen(lpszItemText);
   int j = 0;
   int w = 0;
   for (j = 0; j < l; j++)
   {
      w += PAL_CharWidth(lpszItemText[j]);
   }
   return w;
}

int PAL_MenuTextMaxWidth(
    LPCMENUITEM rgMenuItem,
    int nMenuItem)
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
   int i;
   int r = 0;
   int w;
   for (i = 0; i < nMenuItem; i++)
   {
      const wchar_t *itemText = PAL_GetWord(rgMenuItem[i].wNumWord);
      w = (int)((PAL_TextWidth(PAL_UnescapeText(itemText)) + 8) >> 4);
      if (r < w)
      {
         r = w;
      }
   }
   return r;
}

int PAL_WordMaxWidth(
    int nFirstWord,
    int nWordNum)
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
   int i;
   int j = 0;
   int r = 0;
   for (i = 0; i < nWordNum; i++)
   {
      const wchar_t *itemText = PAL_GetWord(nFirstWord + i);
      int l = (int)wcslen(itemText);
      int w = 0;
      for (j = 0; j < l; j++)
      {
         w += PAL_CharWidth(itemText[j]);
      }
      w = (w + 8) >> 4;
      if (r < w)
      {
         r = w;
      }
   }
   return r;
}

int PAL_WordWidth(
    int nWordIndex)
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
   unsigned int i = 0;
   unsigned int w = 8;
   unsigned int l = (unsigned int)wcslen(itemText);

   for (i = 0; i < l; i++)
   {
      w += PAL_CharWidth(itemText[i]);
   }
   return (w >> 4);
}
