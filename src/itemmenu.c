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

#include "itemmenu.h"
#include "global.h"
#include "input.h"
#include "palcommon.h"
#include "resource.h"
#include "scene.h"
#include "script.h"
#include "text.h"
#include "ui.h"
#include "util.h"
#include "video.h"
#include <assert.h>
#include <stdbool.h>

static int g_iNumInventory = 0;
static unsigned short g_wItemFlags = 0;
static unsigned char g_fNoDesc = false;

unsigned short
PAL_ItemSelectMenuUpdate(
    void)
/*++
  Purpose:

    Initialize the item selection menu.

  Parameters:

    None.

  Return value:

    The object ID of the selected item. 0 if cancelled, 0xFFFF if not confirmed.

--*/
{
    int i, j, k, line, item_delta = 0;
    unsigned short wObject, wScript;
    unsigned char bColor;
    static unsigned char bufImage[2048];
    const int iItemsPerLine = 32 / 10;
    const int iItemTextWidth = 8 * 10 + 20;
    const int iLinesPerPage = 7;
    const int iCursorXOffset = 10 * 5 / 2;
    const int iAmountXOffset = 10 * 8 + 1;
    const int iPageLineOffset = (iLinesPerPage + 1) / 2;
    const int iPictureYOffset = 0;
    unsigned int cursorPos = PAL_XY(15 + iCursorXOffset, 22);

    //
    // Process input
    //
    if (PAL_GetKeyInput() & kKeyMenu)
        return 0;
    else if (PAL_GetKeyInput() & kKeyUp)
        item_delta = -iItemsPerLine;
    else if (PAL_GetKeyInput() & kKeyDown)
        item_delta = iItemsPerLine;
    else if (PAL_GetKeyInput() & kKeyLeft)
        item_delta = -1;
    else if (PAL_GetKeyInput() & kKeyRight)
        item_delta = 1;
    else if (PAL_GetKeyInput() & kKeyPgUp)
        item_delta = -(iItemsPerLine * iLinesPerPage);
    else if (PAL_GetKeyInput() & kKeyPgDn)
        item_delta = iItemsPerLine * iLinesPerPage;
    else if (PAL_GetKeyInput() & kKeyHome)
        item_delta = -gpGlobals->iCurInvMenuItem;
    else if (PAL_GetKeyInput() & kKeyEnd)
        item_delta = g_iNumInventory - gpGlobals->iCurInvMenuItem - 1;

    // Make sure the current menu item index is in bound
    if (gpGlobals->iCurInvMenuItem + item_delta < 0)
        gpGlobals->iCurInvMenuItem = g_iNumInventory - 1;
    else if (gpGlobals->iCurInvMenuItem + item_delta >= g_iNumInventory)
        gpGlobals->iCurInvMenuItem = 0;
    else
        gpGlobals->iCurInvMenuItem += item_delta;

    //
    // Redraw the box
    //
    PAL_CreateBoxWithShadow(PAL_XY(2, 0), iLinesPerPage - 1, 17, 1, NULL, 0);

    //
    // Draw the texts in the current page
    //
    i = gpGlobals->iCurInvMenuItem / iItemsPerLine * iItemsPerLine - iItemsPerLine * iPageLineOffset;
    if (i < 0)
    {
        i = 0;
    }

    const int xBase = 0, yBase = 140;

    for (j = 0; j < iLinesPerPage; j++)
    {
        for (k = 0; k < iItemsPerLine; k++)
        {
            wObject = gpGlobals->rgInventory[i].wItem;
            bColor = MENUITEM_COLOR;

            if (i >= MAX_INVENTORY || wObject == 0)
            {
                //
                // End of the list reached
                //
                j = iLinesPerPage;
                break;
            }

            if (i == gpGlobals->iCurInvMenuItem)
            {
                if (!(gpGlobals->g.rgObject[wObject].item.wFlags & g_wItemFlags) ||
                    (short)gpGlobals->rgInventory[i].nAmount <= (short)gpGlobals->rgInventory[i].nAmountInUse)
                {
                    //
                    // This item is not selectable
                    //
                    bColor = MENUITEM_COLOR_SELECTED_INACTIVE;
                }
                else
                {
                    //
                    // This item is selectable
                    //
                    if (gpGlobals->rgInventory[i].nAmount == 0)
                    {
                        bColor = MENUITEM_COLOR_EQUIPPEDITEM;
                    }
                    else
                    {
                        bColor = MENUITEM_COLOR_SELECTED;
                    }
                }
            }
            else if (!(gpGlobals->g.rgObject[wObject].item.wFlags & g_wItemFlags) ||
                     (short)gpGlobals->rgInventory[i].nAmount <= (short)gpGlobals->rgInventory[i].nAmountInUse)
            {
                //
                // This item is not selectable
                //
                bColor = MENUITEM_COLOR_INACTIVE;
            }
            else if (gpGlobals->rgInventory[i].nAmount == 0)
            {
                bColor = MENUITEM_COLOR_EQUIPPEDITEM;
            }

            //
            // Draw the text
            //
            PAL_DrawText(PAL_GetWord(wObject), PAL_XY(15 + k * iItemTextWidth, 12 + j * 18), bColor, true, false, false);

            if (i == gpGlobals->iCurInvMenuItem)
            {
                cursorPos = PAL_XY(15 + iCursorXOffset + k * iItemTextWidth, 22 + j * 18);

                //
                // Draw the picture of current selected item
                //
                PAL_RLEBlitToSurfaceWithShadow(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_ITEMBOX), gpScreen,
                                               PAL_XY(xBase + 5, yBase + 5 - iPictureYOffset), true);
                PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_ITEMBOX), gpScreen,
                                     PAL_XY(xBase, yBase - iPictureYOffset));

                if (RES_MKFReadChunk(bufImage, sizeof(bufImage), gpGlobals->g.rgObject[wObject].item.wBitmap, Res_BALL) > 0)
                {
                    PAL_RLEBlitToSurface(bufImage, gpScreen, PAL_XY(xBase + 8, yBase + 7 - iPictureYOffset));
                }
            }

            //
            // Draw the amount of this item
            //
            if ((short)gpGlobals->rgInventory[i].nAmount - (short)gpGlobals->rgInventory[i].nAmountInUse > 1)
            {
                PAL_DrawNumber(gpGlobals->rgInventory[i].nAmount - gpGlobals->rgInventory[i].nAmountInUse,
                               2, PAL_XY(15 + iAmountXOffset + k * iItemTextWidth, 17 + j * 18), kNumColorCyan, kNumAlignRight);
            }

            i++;
        }
    }

    //
    // Draw the cursor on the current selected item
    //
    PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_CURSOR), gpScreen, cursorPos);

    wObject = gpGlobals->rgInventory[gpGlobals->iCurInvMenuItem].wItem;

    //
    // Draw the description of the selected item
    //
    if (!g_fNoDesc)
    {
        wScript = gpGlobals->g.rgObject[wObject].item.wScriptDesc;
        line = 0;
        while (wScript && gpGlobals->g.lprgScriptEntry[wScript].wOperation != 0)
        {
            if (gpGlobals->g.lprgScriptEntry[wScript].wOperation == 0xFFFF)
            {
                int line_incr = (gpGlobals->g.lprgScriptEntry[wScript].rgwOperand[1] != 1) ? 1 : 0;
                wScript = PAL_RunAutoScript(wScript, PAL_ITEM_DESC_BOTTOM | line);
                line += line_incr;
            }
            else
            {
                wScript = PAL_RunAutoScript(wScript, 0);
            }
        }
    }

    if (PAL_GetKeyInput() & kKeySearch)
    {
        if ((gpGlobals->g.rgObject[wObject].item.wFlags & g_wItemFlags) &&
            (short)gpGlobals->rgInventory[gpGlobals->iCurInvMenuItem].nAmount >
                (short)gpGlobals->rgInventory[gpGlobals->iCurInvMenuItem].nAmountInUse)
        {
            if (gpGlobals->rgInventory[gpGlobals->iCurInvMenuItem].nAmount > 0)
            {
                j = (gpGlobals->iCurInvMenuItem < iItemsPerLine * iPageLineOffset) ? (gpGlobals->iCurInvMenuItem / iItemsPerLine) : iPageLineOffset;
                k = gpGlobals->iCurInvMenuItem % iItemsPerLine;

                PAL_DrawText(PAL_GetWord(wObject), PAL_XY(15 + k * iItemTextWidth, 12 + j * 18), MENUITEM_COLOR_CONFIRMED, false, false, false);

                //
                // Draw the cursor on the current selected item
                //
                PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_CURSOR), gpScreen, cursorPos);
            }

            return wObject;
        }
    }

    return 0xFFFF;
}

void PAL_ItemSelectMenuInit(
    unsigned short wItemFlags)
/*++
  Purpose:

    Initialize the item selection menu.

  Parameters:

    [IN]  wItemFlags - flags for usable item.

  Return value:

    None.

--*/
{
    int i, j;
    unsigned short w;

    g_wItemFlags = wItemFlags;

    //
    // Compress the inventory
    //
    PAL_CompressInventory();

    //
    // Count the total number of items in inventory
    //
    g_iNumInventory = 0;
    while (g_iNumInventory < MAX_INVENTORY &&
           gpGlobals->rgInventory[g_iNumInventory].wItem != 0)
    {
        g_iNumInventory++;
    }

    //
    // Also add usable equipped items to the list
    //
    if ((wItemFlags & kItemFlagUsable) && !gpGlobals->fInBattle)
    {
        for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
        {
            w = gpGlobals->rgParty[i].wPlayerRole;

            for (j = 0; j < MAX_PLAYER_EQUIPMENTS; j++)
            {
                if (gpGlobals->g.rgObject[gpGlobals->g.PlayerRoles->rgwEquipment[j][w]].item.wFlags & kItemFlagUsable)
                {
                    if (g_iNumInventory < MAX_INVENTORY)
                    {
                        gpGlobals->rgInventory[g_iNumInventory].wItem = gpGlobals->g.PlayerRoles->rgwEquipment[j][w];
                        gpGlobals->rgInventory[g_iNumInventory].nAmount = 0;
                        gpGlobals->rgInventory[g_iNumInventory].nAmountInUse = (unsigned short)-1;

                        g_iNumInventory++;
                    }
                }
            }
        }
    }
}

unsigned short
PAL_ItemSelectMenu(
    void (*lpfnMenuItemChanged)(unsigned short),
    unsigned short wItemFlags)
/*++
  Purpose:

    Show the item selection menu.

  Parameters:

    [IN]  lpfnMenuItemChanged - Callback function which is called when user
                                changed the current menu item.

    [IN]  wItemFlags - flags for usable item.

  Return value:

    The object ID of the selected item. 0 if cancelled.

--*/
{
    unsigned short w = 0xFFFF;
    int iPrevIndex = gpGlobals->iCurInvMenuItem;

    PAL_ItemSelectMenuInit(wItemFlags);
    PAL_ClearKeyState();

    if (lpfnMenuItemChanged != NULL)
    {
        g_fNoDesc = true;
        (*lpfnMenuItemChanged)(gpGlobals->rgInventory[gpGlobals->iCurInvMenuItem].wItem);
    }

    while (true)
    {
        if (lpfnMenuItemChanged == NULL)
        {
            PAL_MakeScene();
        }

        if (iPrevIndex != gpGlobals->iCurInvMenuItem)
        {
            if (gpGlobals->iCurInvMenuItem >= 0 && gpGlobals->iCurInvMenuItem < MAX_INVENTORY)
            {
                if (lpfnMenuItemChanged != NULL)
                {
                    (*lpfnMenuItemChanged)(gpGlobals->rgInventory[gpGlobals->iCurInvMenuItem].wItem);
                }
            }

            iPrevIndex = gpGlobals->iCurInvMenuItem;
        }

        w = PAL_ItemSelectMenuUpdate();
        VIDEO_UpdateScreen(NULL);

        if (w != 0xFFFF)
        {
            g_fNoDesc = false;
            return w;
        }
        UTIL_WaitKeys(FRAME_TIME, 0);
    }

    assert(false);
    return 0; // should not really reach here
}
