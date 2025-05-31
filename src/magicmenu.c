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

#include "magicmenu.h"
#include "global.h"
#include "input.h"
#include "palcommon.h"
#include "scene.h"
#include "script.h"
#include "text.h"
#include "uibattle.h"
#include "util.h"
#include "video.h"
#include <stdbool.h>

static struct MAGICITEM
{
    unsigned short wMagic;
    unsigned short wMP;
    int fEnabled;
} rgMagicItem[MAX_PLAYER_MAGICS];

static int g_iNumMagic = 0;
static int g_iCurrentItem = 0;
static unsigned short g_wPlayerMP = 0;

unsigned short
PAL_MagicSelectionMenuUpdate(
    void)
/*++
  Purpose:

    Update the magic selection menu.

  Parameters:

    None.

  Return value:

    The selected magic. 0 if cancelled, 0xFFFF if not confirmed.

--*/
{
    int i, j, k, line, item_delta = 0;
    unsigned char bColor;
    unsigned short wScript;
    const int iItemsPerLine = 32 / 10;
    const int iItemTextWidth = 8 * 10 + 7;
    const int iLinesPerPage = 5;
    const int iBoxYOffset = 0;
    const int iCursorXOffset = 10 * 5 / 2;
    const int iPageLineOffset = iLinesPerPage / 2;

    //
    // Check for inputs
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
        item_delta = -g_iCurrentItem;
    else if (PAL_GetKeyInput() & kKeyEnd)
        item_delta = g_iNumMagic - g_iCurrentItem - 1;

    // Make sure the current menu item index is in bound
    if (g_iCurrentItem + item_delta < 0)
        g_iCurrentItem = g_iNumMagic - 1;
    else if (g_iCurrentItem + item_delta >= g_iNumMagic)
        g_iCurrentItem = 0;
    else
        g_iCurrentItem = g_iCurrentItem + item_delta;

    //
    // Create the box.
    //
    PAL_CreateBoxWithShadow(PAL_XY(10, 42 + iBoxYOffset), iLinesPerPage - 1, 16, 1, NULL, 0);

    wScript = gpGlobals->g.rgObject[rgMagicItem[g_iCurrentItem].wMagic].item.wScriptDesc;
    line = 0;
    while (wScript && gpGlobals->g.lprgScriptEntry[wScript].wOperation != 0)
    {
        if (gpGlobals->g.lprgScriptEntry[wScript].wOperation == 0xFFFF)
        {
            int line_incr = (gpGlobals->g.lprgScriptEntry[wScript].rgwOperand[1] != 1) ? 1 : 0;
            wScript = PAL_RunAutoScript(wScript, line);
            line += line_incr;
        }
        else
        {
            wScript = PAL_RunAutoScript(wScript, 0);
        }
    }

    //
    // Draw the MP of the selected magic.
    //
    PAL_CreateSingleLineBox(PAL_XY(0, 0), PAL_X(PAL_XY(5, 0)), NULL);
    PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen, PAL_XY(45, 14));
    PAL_DrawNumber(rgMagicItem[g_iCurrentItem].wMP, 4, PAL_XY(15, 14), kNumColorYellow, kNumAlignRight);
    PAL_DrawNumber(g_wPlayerMP, 4, PAL_XY(50, 14), kNumColorCyan, kNumAlignRight);

    //
    // Draw the texts of the current page
    //
    i = g_iCurrentItem / iItemsPerLine * iItemsPerLine - iItemsPerLine * iPageLineOffset;
    if (i < 0)
    {
        i = 0;
    }

    for (j = 0; j < iLinesPerPage; j++)
    {
        for (k = 0; k < iItemsPerLine; k++)
        {
            bColor = MENUITEM_COLOR;

            if (i >= g_iNumMagic)
            {
                //
                // End of the list reached
                //
                j = iLinesPerPage;
                break;
            }

            if (i == g_iCurrentItem)
            {
                if (rgMagicItem[i].fEnabled)
                {
                    bColor = MENUITEM_COLOR_SELECTED;
                }
                else
                {
                    bColor = MENUITEM_COLOR_SELECTED_INACTIVE;
                }
            }
            else if (!rgMagicItem[i].fEnabled)
            {
                bColor = MENUITEM_COLOR_INACTIVE;
            }

            //
            // Draw the text
            //
            PAL_DrawText(PAL_GetWord(rgMagicItem[i].wMagic), PAL_XY(35 + k * iItemTextWidth, 54 + j * 18 + iBoxYOffset), bColor, true, false, false);

            //
            // Draw the cursor on the current selected item
            //
            if (i == g_iCurrentItem)
            {
                PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_CURSOR),
                                     gpScreen, PAL_XY(35 + iCursorXOffset + k * iItemTextWidth, 64 + j * 18 + iBoxYOffset));
            }

            i++;
        }
    }

    if (PAL_GetKeyInput() & kKeySearch)
    {
        if (rgMagicItem[g_iCurrentItem].fEnabled)
        {
            j = g_iCurrentItem % iItemsPerLine;
            k = (g_iCurrentItem < iItemsPerLine * iPageLineOffset) ? (g_iCurrentItem / iItemsPerLine) : iPageLineOffset;

            j = 35 + j * iItemTextWidth;
            k = 54 + k * 18 + iBoxYOffset;

            PAL_DrawText(PAL_GetWord(rgMagicItem[g_iCurrentItem].wMagic), PAL_XY(j, k), MENUITEM_COLOR_CONFIRMED, false, true, false);

            //
            // Draw the cursor on the current selected item
            //
            PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_CURSOR), gpScreen, PAL_XY(j + iCursorXOffset, k + 10));

            return rgMagicItem[g_iCurrentItem].wMagic;
        }
    }

    return 0xFFFF;
}

void PAL_MagicSelectionMenuInit(
    unsigned short wPlayerRole,
    int fInBattle,
    unsigned short wDefaultMagic)
/*++
  Purpose:

    Initialize the magic selection menu.

  Parameters:

    [IN]  wPlayerRole - the player ID.

    [IN]  fInBattle - true if in battle, false if not.

    [IN]  wDefaultMagic - the default magic item.

  Return value:

    None.

--*/
{
    unsigned short w;
    int i, j;

    g_iCurrentItem = 0;
    g_iNumMagic = 0;

    g_wPlayerMP = gpGlobals->g.PlayerRoles->rgwMP[wPlayerRole];

    //
    // Put all magics of this player to the array
    //
    for (i = 0; i < MAX_PLAYER_MAGICS; i++)
    {
        w = gpGlobals->g.PlayerRoles->rgwMagic[i][wPlayerRole];
        if (w != 0)
        {
            rgMagicItem[g_iNumMagic].wMagic = w;

            w = gpGlobals->g.rgObject[w].magic.wMagicNumber;
            rgMagicItem[g_iNumMagic].wMP = gpGlobals->g.lprgMagic[w].wCostMP;

            rgMagicItem[g_iNumMagic].fEnabled = true;

            if (rgMagicItem[g_iNumMagic].wMP > g_wPlayerMP)
            {
                rgMagicItem[g_iNumMagic].fEnabled = false;
            }

            w = gpGlobals->g.rgObject[rgMagicItem[g_iNumMagic].wMagic].magic.wFlags;
            if (fInBattle)
            {
                if (!(w & kMagicFlagUsableInBattle))
                {
                    rgMagicItem[g_iNumMagic].fEnabled = false;
                }
            }
            else
            {
                if (!(w & kMagicFlagUsableOutsideBattle))
                {
                    rgMagicItem[g_iNumMagic].fEnabled = false;
                }
            }

            g_iNumMagic++;
        }
    }

    //
    // Sort the array
    //
    for (i = 0; i < g_iNumMagic - 1; i++)
    {
        int fCompleted = true;

        for (j = 0; j < g_iNumMagic - 1 - i; j++)
        {
            if (rgMagicItem[j].wMagic > rgMagicItem[j + 1].wMagic)
            {
                struct MAGICITEM t = rgMagicItem[j];
                rgMagicItem[j] = rgMagicItem[j + 1];
                rgMagicItem[j + 1] = t;

                fCompleted = false;
            }
        }

        if (fCompleted)
        {
            break;
        }
    }

    //
    // Place the cursor to the default item
    //
    for (i = 0; i < g_iNumMagic; i++)
    {
        if (rgMagicItem[i].wMagic == wDefaultMagic)
        {
            g_iCurrentItem = i;
            break;
        }
    }
}

unsigned short
PAL_MagicSelectionMenu(
    unsigned short wPlayerRole,
    int fInBattle,
    unsigned short wDefaultMagic)
/*++
  Purpose:

    Show the magic selection menu.

  Parameters:

    [IN]  wPlayerRole - the player ID.

    [IN]  fInBattle - true if in battle, false if not.

    [IN]  wDefaultMagic - the default magic item.

  Return value:

    The selected magic. 0 if cancelled.

--*/
{
    unsigned long dwTime;
    unsigned short w;
    int i;

    PAL_MagicSelectionMenuInit(wPlayerRole, fInBattle, wDefaultMagic);
    PAL_ClearKeyState();

    dwTime = UTIL_GetTicks() + 5;

    while (true)
    {
        PAL_MakeScene();

        w = 45;

        for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
        {
            PAL_PlayerInfoBox(PAL_XY(w, 165), gpGlobals->rgParty[i].wPlayerRole, 100,
                              TIMEMETER_COLOR_DEFAULT, false);
            w += 78;
        }

        w = PAL_MagicSelectionMenuUpdate();
        VIDEO_UpdateScreen(NULL);

        PAL_ClearKeyState();

        if (w != 0xFFFF)
        {
            return w;
        }

        do
        {
            UTIL_Delay(1);
            if (PAL_GetKeyInput() != kKeyNone)
                break;
        } while (UTIL_GetTicks() < dwTime);

        dwTime = UTIL_GetTicks() + FRAME_TIME;
    }

    return 0; // should not really reach here
}
