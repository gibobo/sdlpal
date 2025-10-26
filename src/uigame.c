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

#include "uigame.h"
#include "audio.h"
#include "driver.h"
#include "global.h"
#include "input.h"
#include "itemmenu.h"
#include "magicmenu.h"
#include "main.h"
#include "palcommon.h"
#include "palette.h"
#include "play.h"
#include "resource.h"
#include "script.h"
#include "text.h"
#include "uibattle.h"
#include "util.h"
#include "video.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#define bufImageSize 8192 //bigger than 5034

static int __buymenu_firsttime_render;
// Equipment Screen
static const unsigned int EquipImageBox = PAL_XY(8, 8);
static const unsigned int EquipRoleListBox = PAL_XY(2, 95);
static const unsigned int EquipItemName = PAL_XY(5, 70);
static const unsigned int EquipItemAmount = PAL_XY(51, 57);
static const unsigned int EquipLabels[] = {
    PAL_XY(92, 11), PAL_XY(92, 33),
    PAL_XY(92, 55), PAL_XY(92, 77),
    PAL_XY(92, 99), PAL_XY(92, 121)};
static const unsigned int EquipNames[] = {
    PAL_XY(130, 11), PAL_XY(130, 33),
    PAL_XY(130, 55), PAL_XY(130, 77),
    PAL_XY(130, 99), PAL_XY(130, 121)};
static const unsigned int EquipStatusLabels[] = {
    PAL_XY(226, 10), PAL_XY(226, 32),
    PAL_XY(226, 54), PAL_XY(226, 76),
    PAL_XY(226, 98)};
static const unsigned int EquipStatusValues[] = {
    PAL_XY(260, 14), PAL_XY(260, 36),
    PAL_XY(260, 58), PAL_XY(260, 80),
    PAL_XY(260, 102)};

// Status Screen
static const unsigned int RoleName = PAL_XY(110, 8);
static const unsigned int RoleImage = PAL_XY(110, 30);
static const unsigned int RoleLabels[] = {
    PAL_XY(6, 6),  //RoleExpLabel
    PAL_XY(6, 32), //RoleLevelLabel
    PAL_XY(6, 54), //RoleHPLabel
    PAL_XY(6, 76)  //RoleMPLabel
};
static const unsigned int RoleStatusLabels[] = {
    PAL_XY(6, 98), PAL_XY(6, 118),
    PAL_XY(6, 138), PAL_XY(6, 158),
    PAL_XY(6, 178)};
static const unsigned int RoleCurrExp = PAL_XY(58, 6);
static const unsigned int RoleNextExp = PAL_XY(58, 15);
static const unsigned int RoleExpSlash = PAL_XY(0, 0);
static const unsigned int RoleLevel = PAL_XY(54, 35);
static const unsigned int RoleCurHP = PAL_XY(42, 56);
static const unsigned int RoleMaxHP = PAL_XY(63, 61);
static const unsigned int RoleHPSlash = PAL_XY(65, 58);
static const unsigned int RoleCurMP = PAL_XY(42, 78);
static const unsigned int RoleMaxMP = PAL_XY(63, 83);
static const unsigned int RoleMPSlash = PAL_XY(65, 80);
static const unsigned int RoleStatusValues[] = {
    PAL_XY(42, 102), PAL_XY(42, 122),
    PAL_XY(42, 142), PAL_XY(42, 162),
    PAL_XY(42, 182)};
static const unsigned int RoleEquipImageBoxes[] = {
    PAL_XY(189, -1), PAL_XY(247, 39),
    PAL_XY(251, 101), PAL_XY(201, 133),
    PAL_XY(141, 141), PAL_XY(81, 125)};
static const unsigned int RoleEquipNames[] = {
    PAL_XY(195, 38), PAL_XY(253, 78),
    PAL_XY(257, 140), PAL_XY(207, 172),
    PAL_XY(147, 180), PAL_XY(87, 164)};
static const unsigned int RolePoisonNames[] = {
    PAL_XY(185, 58), PAL_XY(185, 76),
    PAL_XY(185, 94), PAL_XY(185, 112),
    PAL_XY(185, 130), PAL_XY(185, 148),
    PAL_XY(185, 166), PAL_XY(185, 184),
    PAL_XY(185, 184), PAL_XY(185, 184)};

// Extra Lines
static const unsigned int ExtraItemDescLines = PAL_XY(0, 0);
static const unsigned int ExtraMagicDescLines = PAL_XY(0, 0);

// Magic Menu Desc
static const unsigned int MagicMPDescLines = PAL_XY(5, 0);
static const unsigned int MagicMPSlashPos = PAL_XY(45, 14);
static const unsigned int MagicMPNeededPos = PAL_XY(15, 14);
static const unsigned int MagicMPCurrentPos = PAL_XY(50, 14);

// Magic Desc Message Pos
static const unsigned int MagicDescMsgPos = PAL_XY(102, 0);

static unsigned short GetSavedTimes(int iSaveSlot)
{
    unsigned short wSavedTimes = 0;
    char save_path[256] = {0};
    sprintf(save_path, RESOURCE_PATH "/%d.rpg", iSaveSlot);
    void *fpSAVE = UTIL_fopen_without_checking(save_path, "rb");
    if (fpSAVE)
    {
        if (UTIL_fread(&wSavedTimes, sizeof(unsigned short), 1, fpSAVE) != 1)
            wSavedTimes = 0;
        UTIL_fclose(fpSAVE);
    }
    return wSavedTimes;
}

void PAL_OpeningMenu(void)
/*++
  Purpose:

    Show the opening menu.

  Parameters:

    None.

  Return value:

    Which saved slot to load from (1-5). 0 to start a new game.

--*/
{
    unsigned short wItemSelected;
    unsigned short wDefaultItem = 0;
    unsigned int w[2] = {PAL_WordWidth(MAINMENU_LABEL_NEWGAME), PAL_WordWidth(MAINMENU_LABEL_LOADGAME)};

    MENUITEM rgMainMenuItem[2] = {
        // value   label                     enabled   position
        {0, MAINMENU_LABEL_NEWGAME, true, PAL_XY(125 - (w[0] > 4 ? (w[0] - 4) * 8 : 0), 95)},
        {1, MAINMENU_LABEL_LOADGAME, true, PAL_XY(125 - (w[1] > 4 ? (w[1] - 4) * 8 : 0), 112)}};

    // Play the background music
    AUDIO_PlayMusic(0x04, true, 1);

    // Draw the background
    // Read the picture from fbp.mkf.
    RES_MKFDecompressChunk(&gpScreen->pixels, SCREEN_SIZE, 2, Res_FBP);

    // ...and blit it to the screen buffer.
    VIDEO_UpdateScreen(NULL);
    PAL_FadeIn(0, false, 1);

    while (true)
    {
        // Activate the menu
        wItemSelected = PAL_ReadMenu(NULL, rgMainMenuItem, 2, wDefaultItem, MENUITEM_COLOR);

        if (wItemSelected == 0 || wItemSelected == MENUITEM_VALUE_CANCELLED)
        {
            // Start a new game
            wItemSelected = 0;
            break;
        }
        else
        {
            // Load game
            VIDEO_BackupScreen(gpScreen);
            wItemSelected = PAL_SaveSlotMenu(1);
            VIDEO_RestoreScreen(gpScreen);
            // VIDEO_UpdateScreen(NULL);
            if (wItemSelected != MENUITEM_VALUE_CANCELLED)
            {
                break;
            }
            wDefaultItem = 0;
        }
    }

    // Fade out the screen and the music
    AUDIO_PlayMusic(0x00, false, 1);
    PAL_FadeOut(1);

    // Initialize game data and set the flags to load the game resources.
    PAL_ReloadInNextTick(wItemSelected & 0xFF);
}

unsigned short PAL_SaveSlotMenu(
    unsigned short wDefaultSlot)
/*++
  Purpose:

    Show the load game menu.

  Parameters:

    [IN]  wDefaultSlot - default save slot number (1-5).

  Return value:

    Which saved slot to load from (1-5). MENUITEM_VALUE_CANCELLED if cancelled.

--*/
{
    unsigned int i;
    unsigned int w = PAL_WordMaxWidth(LOADMENU_LABEL_SLOT_FIRST, 5U);
    unsigned int dx = (w > 4) ? (w - 4) * 16U : 0U;
    unsigned short wItemSelected;

    MENUITEM rgMenuItem[5];

    const PAL_Rect rect = {195 - dx, 7, 120 + dx, 190};

    //
    // Create the boxes and create the menu items
    //
    for (i = 0; i < 5; i++)
    {
        // Fix render problem with shadow
        PAL_CreateSingleLineBox(PAL_XY(195 - dx, 7 + 38 * i), 6 + (w > 4 ? w - 4 : 0), NULL);

        rgMenuItem[i].wValue = i + 1;
        rgMenuItem[i].fEnabled = true;
        rgMenuItem[i].wNumWord = LOADMENU_LABEL_SLOT_FIRST + i;
        rgMenuItem[i].pos = PAL_XY(210 - dx, 17 + 38 * i);
    }

    //
    // Draw the numbers of saved times
    //
    for (i = 1; i <= 5; i++)
    {
        //
        // Draw the number
        //
        PAL_DrawNumber((unsigned int)GetSavedTimes(i), 4, PAL_XY(270, 38 * i - 17),
                       kNumColorYellow, kNumAlignRight);
    }

    //
    // Activate the menu
    //
    wItemSelected = PAL_ReadMenu(NULL, rgMenuItem, 5, wDefaultSlot - 1, MENUITEM_COLOR);

    // VIDEO_UpdateScreen(&rect);

    return wItemSelected;
}

static unsigned short
PAL_SelectionMenu(
    const unsigned char nWords,
    const unsigned char nDefault,
    const unsigned short *wItems)
/*++
  Purpose:

    Show a common selection box.

  Parameters:

   [IN]  nWords - number of emnu items.
   [IN]  nDefault - index of default item.
   [IN]  wItems - item word array.

  Return value:

    User-selected index.

--*/
{
    BOX *rgpBox[4];
    MENUITEM rgMenuItem[4];
    unsigned char i;
    unsigned short wReturnValue;
    unsigned int w[4] = {
        (nWords >= 1 && wItems[0]) ? PAL_WordWidth(wItems[0]) : 1,
        (nWords >= 2 && wItems[1]) ? PAL_WordWidth(wItems[1]) : 1,
        (nWords >= 3 && wItems[2]) ? PAL_WordWidth(wItems[2]) : 1,
        (nWords >= 4 && wItems[3]) ? PAL_WordWidth(wItems[3]) : 1};
    unsigned int dx[4] = {
        (w[0] - 1) * 16,
        (w[1] - 1) * 16,
        (w[2] - 1) * 16,
        (w[3] - 1) * 16};
    unsigned int pos[4] = {
        PAL_XY(145, 110),
        PAL_XY(220 + dx[0], 110),
        PAL_XY(145, 160),
        PAL_XY(220 + dx[2], 160)};
    const PAL_Rect rect = {130, 100, 125 + max(dx[0] + dx[1], dx[2] + dx[3]), 100};

    //
    // Create menu items
    //
    for (i = 0; i < nWords; i++)
    {
        if (!wItems[i])
            return MENUITEM_VALUE_CANCELLED;
        rgMenuItem[i].fEnabled = true;
        rgMenuItem[i].pos = pos[i];
        rgMenuItem[i].wValue = i;
        rgMenuItem[i].wNumWord = wItems[i];
    }

    //
    // Create the boxes
    //
    dx[1] = dx[0];
    dx[3] = dx[2];
    dx[0] = dx[2] = 0;
    for (i = 0; i < nWords; i++)
    {
        PAL_CreateSingleLineBox(PAL_XY(130 + 75 * (i % 2) + dx[i], 100 + 50 * (i / 2)), w[i] + 1, &rgpBox[i]);
    }

    //
    // Activate the menu
    //
    wReturnValue = PAL_ReadMenu(NULL, rgMenuItem, nWords, nDefault, MENUITEM_COLOR);

    //
    // Delete the boxes
    //
    for (i = 0; i < nWords; i++)
    {
        PAL_DeleteBox(rgpBox[i]);
    }

    // VIDEO_UpdateScreen(&rect);

    return wReturnValue;
}

unsigned short
PAL_TripleMenu(
    unsigned short wThirdWord)
/*++
  Purpose:

    Show a triple-selection box.

  Parameters:

    None.

  Return value:

    User-selected index.

--*/
{
    unsigned short wItems[3] = {CONFIRMMENU_LABEL_NO, CONFIRMMENU_LABEL_YES, wThirdWord};
    return PAL_SelectionMenu(3, 0, wItems);
}

int PAL_ConfirmMenu(
    void)
/*++
  Purpose:

    Show a "Yes or No?" confirm box.

  Parameters:

    None.

  Return value:

    true if user selected Yes, false if selected No.

--*/
{
    unsigned short wItems[2] = {CONFIRMMENU_LABEL_NO, CONFIRMMENU_LABEL_YES};
    unsigned short wReturnValue = PAL_SelectionMenu(2, 0, wItems);

    return (wReturnValue == MENUITEM_VALUE_CANCELLED || wReturnValue == 0) ? false : true;
}

int PAL_SwitchMenu(
    int fEnabled)
/*++
  Purpose:

    Show a "Enable/Disable" selection box.

  Parameters:

    [IN]  fEnabled - whether the option is originally enabled or not.

  Return value:

    true if user selected "Enable", false if selected "Disable".

--*/
{
    unsigned short wItems[2] = {SWITCHMENU_LABEL_DISABLE, SWITCHMENU_LABEL_ENABLE};
    unsigned short wReturnValue = PAL_SelectionMenu(2, fEnabled ? 1 : 0, wItems);
    return (wReturnValue == MENUITEM_VALUE_CANCELLED) ? fEnabled : ((wReturnValue == 0) ? false : true);
}

static void
PAL_SystemMenu_OnItemChange(
    unsigned short wCurrentItem)
/*++
  Purpose:

    Callback function when user selected another item in the system menu.

  Parameters:

    [IN]  wCurrentItem - current selected item.

  Return value:

    None.

--*/
{
    gpGlobals->iCurSystemMenuItem = wCurrentItem - 1;
}

static int
PAL_SystemMenu(
    void)
/*++
  Purpose:

    Show the system menu.

  Parameters:

    None.

  Return value:

    true if user made some operations in the menu, false if user cancelled.

--*/
{
    BOX *lpMenuBox = NULL;
    unsigned short wReturnValue;
    unsigned short iSlot;
    int i;
    const PAL_Rect rect = {40, 60, 280, 135};

    //
    // Create menu items
    //
    const MENUITEM rgSystemMenuItem[] =
        {
            // value  label                        enabled   pos
            {1, SYSMENU_LABEL_SAVE, true, PAL_XY(53, 72)},
            {2, SYSMENU_LABEL_LOAD, true, PAL_XY(53, 72 + 18)},
            {3, SYSMENU_LABEL_MUSIC, true, PAL_XY(53, 72 + 36)},
            {4, SYSMENU_LABEL_SOUND, true, PAL_XY(53, 72 + 54)},
            {5, SYSMENU_LABEL_QUIT, true, PAL_XY(53, 72 + 72)},
        };
    const int nSystemMenuItem = sizeof(rgSystemMenuItem) / sizeof(MENUITEM);

    //
    // Create the menu box.
    //
    PAL_CreateBox(PAL_XY(40, 60), nSystemMenuItem - 1, PAL_MenuTextMaxWidth(rgSystemMenuItem, nSystemMenuItem) - 1, 0, &lpMenuBox);

    //
    // Perform the menu.
    //
    wReturnValue = PAL_ReadMenu(PAL_SystemMenu_OnItemChange, rgSystemMenuItem, nSystemMenuItem, gpGlobals->iCurSystemMenuItem, MENUITEM_COLOR);

    if (wReturnValue == MENUITEM_VALUE_CANCELLED)
    {
        //
        // User cancelled the menu
        //
        PAL_DeleteBox(lpMenuBox);
        VIDEO_UpdateScreen(&rect);
        return false;
    }

    switch (wReturnValue)
    {
        case 1:
            //
            // Save game
            //
            iSlot = PAL_SaveSlotMenu(gpGlobals->bCurrentSaveSlot);
            if (iSlot != MENUITEM_VALUE_CANCELLED)
            {
                unsigned short wSavedTimes = 0;
                gpGlobals->bCurrentSaveSlot = (unsigned char)iSlot;

                for (i = 1; i <= 5; i++)
                {
                    unsigned short curSavedTimes = GetSavedTimes(i);
                    if (curSavedTimes > wSavedTimes)
                    {
                        wSavedTimes = curSavedTimes;
                    }
                }
                PAL_SaveGame(iSlot, wSavedTimes + 1);
            }
            break;

        case 2:
            //
            // Load game
            //
            iSlot = PAL_SaveSlotMenu(gpGlobals->bCurrentSaveSlot);
            if (iSlot != MENUITEM_VALUE_CANCELLED)
            {
                AUDIO_PlayMusic(0x00, false, 1);
                PAL_FadeOut(1);
                PAL_ReloadInNextTick(iSlot & 0xFF);
            }
            break;

        case 3:
            //
            // Music
            //
            AUDIO_EnableMusic(PAL_SwitchMenu(AUDIO_MusicEnabled()));
            break;

        case 4:
            //
            // Sound
            //
            AUDIO_EnableSound(PAL_SwitchMenu(AUDIO_SoundEnabled()));
            break;

        case 5:
            //
            // Quit
            //
            PAL_QuitGame();
            break;
    }

    PAL_DeleteBox(lpMenuBox);
    return true;
}

void PAL_InGameMagicMenu(
    void)
/*++
  Purpose:

    Show the magic menu.

  Parameters:

    None.

  Return value:

    None.

--*/
{
    MENUITEM rgMenuItem[MAX_PLAYERS_IN_PARTY];
    int i, y;
    static unsigned short w;
    unsigned short wMagic;

    if (gpGlobals->wMaxPartyMemberIndex == 0)
    {
        w = 0;
        goto start_magicmenu;
    }

    //
    // Draw the player info boxes
    //
    y = 45;

    for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
    {
        PAL_PlayerInfoBox(PAL_XY(y, 165), gpGlobals->rgParty[i].wPlayerRole);
        y += 78;
    }

    y = 75;

    //
    // Generate one menu items for each player in the party
    //
    for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
    {
        assert(i < MAX_PLAYERS_IN_PARTY);

        rgMenuItem[i].wValue = i;
        rgMenuItem[i].wNumWord =
            gpGlobals->g.PlayerRoles->rgwName[gpGlobals->rgParty[i].wPlayerRole];
        rgMenuItem[i].fEnabled =
            (gpGlobals->g.PlayerRoles->rgwHP[gpGlobals->rgParty[i].wPlayerRole] > 0);
        rgMenuItem[i].pos = PAL_XY(48, y);

        y += 18;
    }

    //
    // Draw the box
    //
    PAL_CreateBox(PAL_XY(35, 62), gpGlobals->wMaxPartyMemberIndex, PAL_MenuTextMaxWidth(rgMenuItem, sizeof(rgMenuItem) / sizeof(MENUITEM)) - 1, 0, NULL);

    w = PAL_ReadMenu(NULL, rgMenuItem, gpGlobals->wMaxPartyMemberIndex + 1, w, MENUITEM_COLOR);

    if (w == MENUITEM_VALUE_CANCELLED)
    {
        return;
    }

start_magicmenu:

    wMagic = 0;

    while (true)
    {
        wMagic = PAL_MagicSelectionMenu(gpGlobals->rgParty[w].wPlayerRole, false, wMagic);
        if (wMagic == 0)
        {
            break;
        }

        VIDEO_BackupScreen(gpScreen);

        if (gpGlobals->g.rgObject[wMagic].magic.wFlags & kMagicFlagApplyToAll)
        {
            gpGlobals->g.rgObject[wMagic].magic.wScriptOnUse =
                PAL_RunTriggerScript(gpGlobals->g.rgObject[wMagic].magic.wScriptOnUse, 0);

            if (g_fScriptSuccess)
            {
                gpGlobals->g.rgObject[wMagic].magic.wScriptOnSuccess =
                    PAL_RunTriggerScript(gpGlobals->g.rgObject[wMagic].magic.wScriptOnSuccess, 0);

                if (g_fScriptSuccess)
                    gpGlobals->g.PlayerRoles->rgwMP[gpGlobals->rgParty[w].wPlayerRole] -=
                        gpGlobals->g.lprgMagic[gpGlobals->g.rgObject[wMagic].magic.wMagicNumber].wCostMP;
            }

            if (gpGlobals->fNeedToFadeIn)
            {
                PAL_FadeIn(gpGlobals->wNumPalette, gpGlobals->fNightPalette, 1);
                gpGlobals->fNeedToFadeIn = false;
            }
        }
        else
        {
            //
            // Need to select which player to use the magic on.
            //
            unsigned short wPlayer = 0;
            PAL_Rect rect;

            while (wPlayer != MENUITEM_VALUE_CANCELLED)
            {
                //
                // Redraw the player info boxes first
                //
                y = 45;

                for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
                {
                    PAL_PlayerInfoBox(PAL_XY(y, 165), gpGlobals->rgParty[i].wPlayerRole);
                    y += 78;
                }

                //
                // Draw the cursor on the selected item
                //
                rect.x = 0;
                rect.y = 158;
                rect.w = SCREEN_W;
                rect.h = 6;

                VIDEO_RestoreScreen(gpScreen);

                PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_CURSOR_UP),
                                     gpScreen, PAL_XY(75 + 78 * wPlayer, rect.y));

                VIDEO_UpdateScreen(&rect);

                while (true)
                {
                    UTIL_WaitKeys(FRAME_TIME, kKeyMenu | kKeySearch | kKeyLeft | kKeyRight | kKeyUp | kKeyDown);

                    if (PAL_GetKeyInput() & kKeyMenu)
                    {
                        wPlayer = MENUITEM_VALUE_CANCELLED;
                        break;
                    }
                    else if (PAL_GetKeyInput() & kKeySearch)
                    {
                        gpGlobals->g.rgObject[wMagic].magic.wScriptOnUse =
                            PAL_RunTriggerScript(gpGlobals->g.rgObject[wMagic].magic.wScriptOnUse,
                                                 gpGlobals->rgParty[wPlayer].wPlayerRole);

                        if (g_fScriptSuccess)
                        {
                            gpGlobals->g.rgObject[wMagic].magic.wScriptOnSuccess =
                                PAL_RunTriggerScript(gpGlobals->g.rgObject[wMagic].magic.wScriptOnSuccess,
                                                     gpGlobals->rgParty[wPlayer].wPlayerRole);

                            if (g_fScriptSuccess)
                            {
                                gpGlobals->g.PlayerRoles->rgwMP[gpGlobals->rgParty[w].wPlayerRole] -=
                                    gpGlobals->g.lprgMagic[gpGlobals->g.rgObject[wMagic].magic.wMagicNumber].wCostMP;

                                //
                                // Check if we have run out of MP
                                //
                                if (gpGlobals->g.PlayerRoles->rgwMP[gpGlobals->rgParty[w].wPlayerRole] <
                                    gpGlobals->g.lprgMagic[gpGlobals->g.rgObject[wMagic].magic.wMagicNumber].wCostMP)
                                {
                                    //
                                    // Don't go further if run out of MP
                                    //
                                    wPlayer = MENUITEM_VALUE_CANCELLED;
                                }
                            }
                        }

                        break;
                    }
                    else if (PAL_GetKeyInput() & (kKeyLeft | kKeyUp))
                    {
                        if (wPlayer > 0)
                        {
                            wPlayer--;
                            break;
                        }
                    }
                    else if (PAL_GetKeyInput() & (kKeyRight | kKeyDown))
                    {
                        if (wPlayer < gpGlobals->wMaxPartyMemberIndex)
                        {
                            wPlayer++;
                            break;
                        }
                    }
                }
            }
        }

        //
        // Redraw the player info boxes
        //
        y = 45;

        for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
        {
            PAL_PlayerInfoBox(PAL_XY(y, 165), gpGlobals->rgParty[i].wPlayerRole);
            y += 78;
        }
    }
}

static void
PAL_InventoryMenu(
    void)
/*++
  Purpose:

    Show the inventory menu.

  Parameters:

    None.

  Return value:

    None.

--*/
{
    static unsigned short w = 0;

    MENUITEM rgMenuItem[2] =
        {
            // value  label                     enabled   pos
            {1, INVMENU_LABEL_EQUIP, true, PAL_XY(43, 73)},
            {2, INVMENU_LABEL_USE, true, PAL_XY(43, 73 + 18)},
        };

    PAL_CreateBox(PAL_XY(30, 60), 1, PAL_MenuTextMaxWidth(rgMenuItem, sizeof(rgMenuItem) / sizeof(MENUITEM)) - 1, 0, NULL);

    w = PAL_ReadMenu(NULL, rgMenuItem, 2, w - 1, MENUITEM_COLOR);

    switch (w)
    {
        case 1:
            PAL_GameEquipItem();
            break;

        case 2:
            PAL_GameUseItem();
            break;
    }
}

static void
PAL_InGameMenu_OnItemChange(
    unsigned short wCurrentItem)
/*++
  Purpose:

    Callback function when user selected another item in the in-game menu.

  Parameters:

    [IN]  wCurrentItem - current selected item.

  Return value:

    None.

--*/
{
    gpGlobals->iCurMainMenuItem = wCurrentItem - 1;
}

void PAL_InGameMenu(
    void)
/*++
  Purpose:

    Show the in-game main menu.

  Parameters:

    None.

  Return value:

    None.

--*/
{
    BOX *lpCashBox = NULL;
    BOX *lpMenuBox = NULL;
    unsigned short wReturnValue;
    unsigned char loop_flag = 1;

    // Fix render problem with shadow
    VIDEO_BackupScreen(gpScreen);

    //
    // Create menu items
    //
    MENUITEM rgMainMenuItem[4] = {
        // value   label                      enabled  pos
        {1, GAMEMENU_LABEL_STATUS, true, PAL_XY(16, 50)},
        {2, GAMEMENU_LABEL_MAGIC, true, PAL_XY(16, 50 + 18)},
        {3, GAMEMENU_LABEL_INVENTORY, true, PAL_XY(16, 50 + 36)},
        {4, GAMEMENU_LABEL_SYSTEM, true, PAL_XY(16, 50 + 54)},
    };

    //
    // Create the box.
    //
    PAL_CreateSingleLineBox(PAL_XY(0, 0), 5, &lpCashBox);

    //
    // Draw the text label.
    //
    PAL_DrawText(PAL_GetWord(CASH_LABEL), PAL_XY(10, 10), 0, false, false);

    //
    // Draw the cash amount.
    //
    PAL_DrawNumber(gpGlobals->dwCash, 6, PAL_XY(49, 14), kNumColorYellow, kNumAlignRight);

    //
    // Create the menu box.
    //
    // Fix render problem with shadow
    PAL_CreateBox(PAL_XY(3, 37), 3, PAL_MenuTextMaxWidth(rgMainMenuItem, 4) - 1, 0, &lpMenuBox);

    //
    // Process the menu
    //
    while (loop_flag)
    {
        wReturnValue = PAL_ReadMenu(PAL_InGameMenu_OnItemChange, rgMainMenuItem, 4,
                                    gpGlobals->iCurMainMenuItem, MENUITEM_COLOR);

        switch (wReturnValue)
        {
            case MENUITEM_VALUE_CANCELLED:
                loop_flag = 0;
                break;

            case 1:
                // Status
                PAL_PlayerStatus();
                loop_flag = 0;
                break;

            case 2:
                // Magic
                PAL_InGameMagicMenu();
                loop_flag = 0;
                break;

            case 3:
                // Inventory
                PAL_InventoryMenu();
                loop_flag = 0;
                break;

            case 4:
                // System
                if (PAL_SystemMenu())
                    loop_flag = 0;
                break;
        }
    }

    //
    // Remove the boxes.
    //
    PAL_DeleteBox(lpCashBox);
    PAL_DeleteBox(lpMenuBox);

    // Fix render problem with shadow
    VIDEO_RestoreScreen(gpScreen);
}

void PAL_PlayerStatus(
    void)
/*++
  Purpose:

    Show the player status.

  Parameters:

    None.

  Return value:

    None.

--*/
{
    unsigned char *bufImage = NULL;
    int labels0[] = {STATUS_LABEL_EXP, STATUS_LABEL_LEVEL, STATUS_LABEL_HP, STATUS_LABEL_MP};
    int labels1[] = {STATUS_LABEL_EXP_LAYOUT, STATUS_LABEL_LEVEL_LAYOUT, STATUS_LABEL_HP_LAYOUT, STATUS_LABEL_MP_LAYOUT};
    int labels[] = {STATUS_LABEL_ATTACKPOWER, STATUS_LABEL_MAGICPOWER, STATUS_LABEL_RESISTANCE, STATUS_LABEL_DEXTERITY, STATUS_LABEL_FLEERATE};
    int iCurrent = 0;
    int iPlayerRole;
    int i;
    int j;
    unsigned short w;

    bufImage = (unsigned char *)UTIL_malloc(bufImageSize);

    while (iCurrent >= 0 && iCurrent <= gpGlobals->wMaxPartyMemberIndex)
    {
        iPlayerRole = gpGlobals->rgParty[iCurrent].wPlayerRole;

        // Draw the background image
        RES_MKFDecompressChunk(&gpScreen->pixels, SCREEN_SIZE, STATUS_BACKGROUND_FBPNUM, Res_FBP);

        // Draw the image of player role
        if (RES_MKFReadChunk(bufImage, bufImageSize, gpGlobals->g.PlayerRoles->rgwAvatar[iPlayerRole], Res_RGM))
        {
            PAL_RLEBlitToSurface(bufImage, gpScreen, RoleImage);
        }

        // Draw the equipments
        for (i = 0; i < MAX_PLAYER_EQUIPMENTS; i++)
        {
            int offset;

            w = gpGlobals->g.PlayerRoles->rgwEquipment[i][iPlayerRole];

            if (w == 0)
            {
                continue;
            }

            // Draw the image
            if (RES_MKFReadChunk(bufImage, bufImageSize, gpGlobals->g.rgObject[w].item.wBitmap, Res_BALL))
            {
                PAL_RLEBlitToSurface(bufImage, gpScreen, PAL_XY_OFFSET(RoleEquipImageBoxes[i], 1, 1));
            }

            // Draw the text label
            offset = PAL_WordWidth(w) << 4;
            if (PAL_X(RoleEquipNames[i]) + offset > SCREEN_W)
            {
                offset = SCREEN_W - PAL_X(RoleEquipNames[i]) - offset;
            }
            else
            {
                offset = 0;
            }
            PAL_DrawText(PAL_GetWord(w), PAL_XY_OFFSET(RoleEquipNames[i], offset, 0), STATUS_COLOR_EQUIPMENT, true, false);
        }

        // Draw the text labels
        for (i = 0; i < sizeof(labels0) / sizeof(int); i++)
        {
            PAL_DrawText(PAL_GetWord(labels0[i]), RoleLabels[i], MENUITEM_COLOR, true, false);
        }
        for (i = 0; i < sizeof(labels) / sizeof(int); i++)
        {
            PAL_DrawText(PAL_GetWord(labels[i]), RoleStatusLabels[i], MENUITEM_COLOR, true, false);
        }

        PAL_DrawText(PAL_GetWord(gpGlobals->g.PlayerRoles->rgwName[iPlayerRole]),
                     RoleName, MENUITEM_COLOR_CONFIRMED, true, false);

        // Draw the stats
        if (RoleExpSlash != 0)
        {
            PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen, RoleExpSlash);
        }
        if (RoleHPSlash != 0)
        {
            PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen, RoleHPSlash);
        }
        if (RoleMPSlash != 0)
        {
            PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen, RoleMPSlash);
        }

        PAL_DrawNumber(gpGlobals->Exp.rgPrimaryExp[iPlayerRole].wExp, 5, RoleCurrExp, kNumColorYellow, kNumAlignRight);
        PAL_DrawNumber(gpGlobals->g.rgLevelUpExp[gpGlobals->g.PlayerRoles->rgwLevel[iPlayerRole]], 5, RoleNextExp, kNumColorCyan, kNumAlignRight);
        PAL_DrawNumber(gpGlobals->g.PlayerRoles->rgwLevel[iPlayerRole], 2, RoleLevel, kNumColorYellow, kNumAlignRight);
        PAL_DrawNumber(gpGlobals->g.PlayerRoles->rgwHP[iPlayerRole], 4, RoleCurHP, kNumColorYellow, kNumAlignRight);
        PAL_DrawNumber(gpGlobals->g.PlayerRoles->rgwMaxHP[iPlayerRole], 4, RoleMaxHP, kNumColorBlue, kNumAlignRight);
        PAL_DrawNumber(gpGlobals->g.PlayerRoles->rgwMP[iPlayerRole], 4, RoleCurMP, kNumColorYellow, kNumAlignRight);
        PAL_DrawNumber(gpGlobals->g.PlayerRoles->rgwMaxMP[iPlayerRole], 4, RoleMaxMP, kNumColorBlue, kNumAlignRight);

        PAL_DrawNumber(PAL_GetPlayerAttackStrength(iPlayerRole), 4, RoleStatusValues[0], kNumColorYellow, kNumAlignRight);
        PAL_DrawNumber(PAL_GetPlayerMagicStrength(iPlayerRole), 4, RoleStatusValues[1], kNumColorYellow, kNumAlignRight);
        PAL_DrawNumber(PAL_GetPlayerDefense(iPlayerRole), 4, RoleStatusValues[2], kNumColorYellow, kNumAlignRight);
        PAL_DrawNumber(PAL_GetPlayerDexterity(iPlayerRole), 4, RoleStatusValues[3], kNumColorYellow, kNumAlignRight);
        PAL_DrawNumber(PAL_GetPlayerFleeRate(iPlayerRole), 4, RoleStatusValues[4], kNumColorYellow, kNumAlignRight);

        // Draw all poisons
        for (i = j = 0; i < MAX_POISONS; i++)
        {
            w = gpGlobals->rgPoisonStatus[i][iCurrent].wPoisonID;

            if (w != 0 && gpGlobals->g.rgObject[w].poison.wPoisonLevel <= 3)
            {
                PAL_DrawText(PAL_GetWord(w), RolePoisonNames[j++], (unsigned char)(gpGlobals->g.rgObject[w].poison.wColor + 10), true, false);
            }
        }

        // Update the screen
        VIDEO_UpdateScreen(NULL);

        while (true)
        {
            PALKEY keys = UTIL_WaitKeys(1, kKeyMenu | kKeySearch | kKeyLeft | kKeyRight | kKeyUp | kKeyDown);

            if (keys & kKeyMenu)
            {
                iCurrent = -1;
                break;
            }
            else if (keys & (kKeyLeft | kKeyUp))
            {
                iCurrent--;
                break;
            }
            else if (keys & (kKeyRight | kKeyDown | kKeySearch))
            {
                iCurrent++;
                break;
            }
        }
    }
    UTIL_free(bufImage);
}

unsigned short
PAL_ItemUseMenu(
    unsigned short wItemToUse)
/*++
  Purpose:

    Show the use item menu.

  Parameters:

    [IN]  wItemToUse - the object ID of the item to use.

  Return value:

    The selected player to use the item onto.
    MENUITEM_VALUE_CANCELLED if user cancelled.

--*/
{
    unsigned char bColor;
    unsigned char *bufImage = (unsigned char *)UTIL_malloc(bufImageSize);
    static unsigned short sSelectedPlayer = 0;
    PAL_Rect rect = {110, 2, 200, 180};
    int i;

    while (true)
    {
        if (sSelectedPlayer > gpGlobals->wMaxPartyMemberIndex)
        {
            sSelectedPlayer = 0;
        }

        //
        // Draw the box
        //
        PAL_CreateBox(PAL_XY(110, 2), 7, 9, 0, NULL);

        //
        // Draw the stats of the selected player
        //
        PAL_DrawText(PAL_GetWord(STATUS_LABEL_LEVEL), PAL_XY(200, 16), ITEMUSEMENU_COLOR_STATLABEL, true, false);
        PAL_DrawText(PAL_GetWord(STATUS_LABEL_HP), PAL_XY(200, 34), ITEMUSEMENU_COLOR_STATLABEL, true, false);
        PAL_DrawText(PAL_GetWord(STATUS_LABEL_MP), PAL_XY(200, 52), ITEMUSEMENU_COLOR_STATLABEL, true, false);
        PAL_DrawText(PAL_GetWord(STATUS_LABEL_ATTACKPOWER), PAL_XY(200, 70), ITEMUSEMENU_COLOR_STATLABEL, true, false);
        PAL_DrawText(PAL_GetWord(STATUS_LABEL_MAGICPOWER), PAL_XY(200, 88), ITEMUSEMENU_COLOR_STATLABEL, true, false);
        PAL_DrawText(PAL_GetWord(STATUS_LABEL_RESISTANCE), PAL_XY(200, 106), ITEMUSEMENU_COLOR_STATLABEL, true, false);
        PAL_DrawText(PAL_GetWord(STATUS_LABEL_DEXTERITY), PAL_XY(200, 124), ITEMUSEMENU_COLOR_STATLABEL, true, false);
        PAL_DrawText(PAL_GetWord(STATUS_LABEL_FLEERATE), PAL_XY(200, 142), ITEMUSEMENU_COLOR_STATLABEL, true, false);

        i = gpGlobals->rgParty[sSelectedPlayer].wPlayerRole;

        PAL_DrawNumber(gpGlobals->g.PlayerRoles->rgwLevel[i], 4, PAL_XY(240, 20), kNumColorYellow, kNumAlignRight);

        PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen, PAL_XY(263, 38));
        PAL_DrawNumber(gpGlobals->g.PlayerRoles->rgwMaxHP[i], 4, PAL_XY(261, 40), kNumColorBlue, kNumAlignRight);
        PAL_DrawNumber(gpGlobals->g.PlayerRoles->rgwHP[i], 4, PAL_XY(240, 37), kNumColorYellow, kNumAlignRight);

        PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen, PAL_XY(263, 56));
        PAL_DrawNumber(gpGlobals->g.PlayerRoles->rgwMaxMP[i], 4, PAL_XY(261, 58), kNumColorBlue, kNumAlignRight);
        PAL_DrawNumber(gpGlobals->g.PlayerRoles->rgwMP[i], 4, PAL_XY(240, 55), kNumColorYellow, kNumAlignRight);

        PAL_DrawNumber(PAL_GetPlayerAttackStrength(i), 4, PAL_XY(240, 74), kNumColorYellow, kNumAlignRight);
        PAL_DrawNumber(PAL_GetPlayerMagicStrength(i), 4, PAL_XY(240, 92), kNumColorYellow, kNumAlignRight);
        PAL_DrawNumber(PAL_GetPlayerDefense(i), 4, PAL_XY(240, 110), kNumColorYellow, kNumAlignRight);
        PAL_DrawNumber(PAL_GetPlayerDexterity(i), 4, PAL_XY(240, 128), kNumColorYellow, kNumAlignRight);
        PAL_DrawNumber(PAL_GetPlayerFleeRate(i), 4, PAL_XY(240, 146), kNumColorYellow, kNumAlignRight);

        //
        // Draw the names of the players in the party
        //
        for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
        {
            if (i == sSelectedPlayer)
            {
                bColor = MENUITEM_COLOR_SELECTED_FIRST;
            }
            else
            {
                bColor = MENUITEM_COLOR;
            }

            PAL_DrawText(PAL_GetWord(gpGlobals->g.PlayerRoles->rgwName[gpGlobals->rgParty[i].wPlayerRole]), PAL_XY(125, 16 + 20 * i), bColor, true, false);
        }

        PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_ITEMBOX), gpScreen, PAL_XY(120, 80));

        i = PAL_GetItemAmount(wItemToUse);

        if (i > 0)
        {
            //
            // Draw the picture of the item
            //
            if (RES_MKFReadChunk(bufImage, bufImageSize, gpGlobals->g.rgObject[wItemToUse].item.wBitmap, Res_BALL))
            {
                PAL_RLEBlitToSurface(bufImage, gpScreen, PAL_XY(127, 88));
            }

            //
            // Draw the amount and label of the item
            //
            PAL_DrawText(PAL_GetWord(wItemToUse), PAL_XY(116, 143), STATUS_COLOR_EQUIPMENT, true, false);
            PAL_DrawNumber(i, 2, PAL_XY(170, 133), kNumColorCyan, kNumAlignRight);
        }

        //
        // Update the screen area
        //
        // VIDEO_UpdateScreen(&rect);

        while (true)
        {
            //
            // Redraw the selected item.
            //
            PAL_DrawText(
                PAL_GetWord(gpGlobals->g.PlayerRoles->rgwName[gpGlobals->rgParty[sSelectedPlayer].wPlayerRole]),
                PAL_XY(125, 16 + 20 * sSelectedPlayer), MENUITEM_COLOR_SELECTED, false, true);
            // Update the screen
            VIDEO_UpdateScreen(NULL);
            // Wait for any key
            if (UTIL_WaitKeys(FRAME_TIME, 0) != kKeyNone)
            {
                break;
            }
        }

        if (i <= 0)
        {
            UTIL_free(bufImage);
            return MENUITEM_VALUE_CANCELLED;
        }

        if (PAL_GetKeyInput() & (kKeyUp | kKeyLeft))
        {
            sSelectedPlayer--;
            if (sSelectedPlayer < 0)
            {
                sSelectedPlayer = gpGlobals->wMaxPartyMemberIndex;
            }
        }
        else if (PAL_GetKeyInput() & (kKeyDown | kKeyRight))
        {
            sSelectedPlayer++;
            if (sSelectedPlayer > gpGlobals->wMaxPartyMemberIndex)
            {
                sSelectedPlayer = 0;
            }
        }
        else if (PAL_GetKeyInput() & kKeyMenu)
        {
            break;
        }
        else if (PAL_GetKeyInput() & kKeySearch)
        {
            UTIL_free(bufImage);
            return gpGlobals->rgParty[sSelectedPlayer].wPlayerRole;
        }
    }

    UTIL_free(bufImage);
    return MENUITEM_VALUE_CANCELLED;
}

static void
PAL_BuyMenu_OnItemChange(
    unsigned short wCurrentItem)
/*++
  Purpose:

    Callback function which is called when player selected another item
    in the buy menu.

  Parameters:

    [IN]  wCurrentItem - current item on the menu, indicates the object ID of
                         the currently selected item.

  Return value:

    None.

--*/
{
    const PAL_Rect rect = {20, 8, 300, 175};
    int i, j, n, iPlayerID, x, y;
    unsigned char *bufImage;

    // Prepare item bakcground box pos
    x = 40;
    y = 8;

    if (__buymenu_firsttime_render)
        PAL_RLEBlitToSurfaceWithShadow(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_ITEMBOX), gpScreen, PAL_XY(x + 6, y + 6), true);
    //
    // Draw the picture of current selected item
    //
    PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_ITEMBOX), gpScreen,
                         PAL_XY(x, y));

    // Prepare item pos
    x = 48;
    y = 15;

    bufImage = (unsigned char *)UTIL_malloc(bufImageSize);
    if (RES_MKFReadChunk(bufImage, bufImageSize, gpGlobals->g.rgObject[wCurrentItem].item.wBitmap, Res_BALL))
    {
        PAL_RLEBlitToSurface(bufImage, gpScreen, PAL_XY(x, y));
    }
    UTIL_free(bufImage);

    // See how many of this item we have in the inventory
    n = 0;

    for (i = 0; i < MAX_INVENTORY; i++)
    {
        if (gpGlobals->rgInventory[i].wItem == 0)
        {
            break;
        }
        else if (gpGlobals->rgInventory[i].wItem == wCurrentItem)
        {
            n = gpGlobals->rgInventory[i].nAmount;
            break;
        }
    }

    for (i = 0; i < MAX_PLAYER_EQUIPMENTS; i++)
    {
        for (j = 0; j <= gpGlobals->wMaxPartyMemberIndex; j++)
        {
            iPlayerID = gpGlobals->rgParty[j].wPlayerRole;

            if (gpGlobals->g.PlayerRoles->rgwEquipment[i][iPlayerID] == wCurrentItem)
                n++;
        }
    }

    // Prepare inventory quantities pos
    x = 20;
    y = 100;
    //
    // Draw the amount of this item in the inventory
    //
    PAL_CreateSingleLineBoxWithShadow(PAL_XY(x, y), 5, NULL, (__buymenu_firsttime_render) ? 6 : 0);
    PAL_DrawText(PAL_GetWord(BUYMENU_LABEL_CURRENT), PAL_XY(x + 10, y + 10), 0, false, false);
    PAL_DrawNumber(n, 6, PAL_XY(x + 49, y + 15), kNumColorYellow, kNumAlignRight);

    //
    // Prepare inventory quantities pos
    //
    x = 20;
    y = 141;
    //
    // Draw the cash amount
    //
    PAL_CreateSingleLineBoxWithShadow(PAL_XY(x, y), 5, NULL, (__buymenu_firsttime_render) ? 6 : 0);
    PAL_DrawText(PAL_GetWord(CASH_LABEL), PAL_XY(x + 10, y + 10), 0, false, false);
    PAL_DrawNumber(gpGlobals->dwCash, 6, PAL_XY(x + 49, y + 15), kNumColorYellow, kNumAlignRight);

    // VIDEO_UpdateScreen(&rect);

    __buymenu_firsttime_render = false;
}

void PAL_BuyMenu(
    unsigned short wStoreNum)
/*++
  Purpose:

    Show the buy item menu.

  Parameters:

    [IN]  wStoreNum - number of the store to buy items from.

  Return value:

    None.

--*/
{
    MENUITEM rgMenuItem[MAX_STORE_ITEM];
    int i, y;
    unsigned short w;

    //
    // create the menu items
    //
    y = 21;

    for (i = 0; i < MAX_STORE_ITEM; i++)
    {
        if (gpGlobals->g.lprgStore[wStoreNum].rgwItems[i] == 0)
        {
            break;
        }

        rgMenuItem[i].wValue = gpGlobals->g.lprgStore[wStoreNum].rgwItems[i];
        rgMenuItem[i].wNumWord = gpGlobals->g.lprgStore[wStoreNum].rgwItems[i];
        rgMenuItem[i].fEnabled = true;
        rgMenuItem[i].pos = PAL_XY(150, y);

        y += 18;
    }

    //
    // Draw the box
    //
    PAL_CreateBox(PAL_XY(122, 8), 8, 8, 1, NULL);

    //
    // Draw the number of prices
    //
    for (y = 0; y < i; y++)
    {
        w = gpGlobals->g.rgObject[rgMenuItem[y].wValue].item.wPrice;
        PAL_DrawNumber(w, 6, PAL_XY(238, 26 + y * 18), kNumColorYellow, kNumAlignRight);
    }

    w = 0;
    __buymenu_firsttime_render = true;

    while (true)
    {
        w = PAL_ReadMenu(PAL_BuyMenu_OnItemChange, rgMenuItem, i, w, MENUITEM_COLOR);

        if (w == MENUITEM_VALUE_CANCELLED)
        {
            break;
        }

        if (gpGlobals->g.rgObject[w].item.wPrice <= gpGlobals->dwCash)
        {
            if (PAL_ConfirmMenu())
            {
                //
                // Player bought an item
                //
                gpGlobals->dwCash -= gpGlobals->g.rgObject[w].item.wPrice;
                PAL_AddItemToInventory(w, 1);
            }
        }

        //
        // Place the cursor to the current item on next loop
        //
        for (y = 0; y < i; y++)
        {
            if (w == rgMenuItem[y].wValue)
            {
                w = y;
                break;
            }
        }
    }
}

static void
PAL_SellMenu_OnItemChange(
    unsigned short wCurrentItem)
/*++
  Purpose:

    Callback function which is called when player selected another item
    in the sell item menu.

  Parameters:

    [IN]  wCurrentItem - current item on the menu, indicates the object ID of
                         the currently selected item.

  Return value:

    None.

--*/
{
    unsigned short x = 100;
    unsigned short y = 150;
    //
    // Draw the cash amount
    //
    PAL_CreateSingleLineBoxWithShadow(PAL_XY(x, y), 5, NULL, 0);
    PAL_DrawText(PAL_GetWord(CASH_LABEL), PAL_XY(x + 10, y + 10), 0, false, false);
    PAL_DrawNumber(gpGlobals->dwCash, 6, PAL_XY(x + 48, y + 15), kNumColorYellow, kNumAlignRight);

    x += 124;

    //
    // Draw the price
    //
    PAL_CreateSingleLineBoxWithShadow(PAL_XY(x, y), 5, NULL, 0);

    if (gpGlobals->g.rgObject[wCurrentItem].item.wFlags & kItemFlagSellable)
    {
        PAL_DrawText(PAL_GetWord(SELLMENU_LABEL_PRICE), PAL_XY(x + 10, y + 10), 0, false, false);
        PAL_DrawNumber(gpGlobals->g.rgObject[wCurrentItem].item.wPrice / 2, 6,
                       PAL_XY(x + 48, y + 15), kNumColorYellow, kNumAlignRight);
    }
}

void PAL_SellMenu(
    void)
/*++
  Purpose:

    Show the sell item menu.

  Parameters:

    None.

  Return value:

    None.

--*/
{
    unsigned short w;

    while (true)
    {
        w = PAL_ItemSelectMenu(PAL_SellMenu_OnItemChange, kItemFlagSellable);
        if (w == 0)
        {
            break;
        }

        if (PAL_ConfirmMenu())
        {
            if (PAL_AddItemToInventory(w, -1))
            {
                gpGlobals->dwCash += gpGlobals->g.rgObject[w].item.wPrice / 2;
            }
        }
    }
}

void PAL_EquipItemMenu(
    unsigned short wItem)
/*++
  Purpose:

    Show the menu which allow players to equip the specified item.

  Parameters:

    [IN]  wItem - the object ID of the item.

  Return value:

    None.

--*/
{
    unsigned short w;
    unsigned short iCurrentPlayer = 0;
    int i;
    unsigned char bColor;
    unsigned char *bufImage = (unsigned char *)UTIL_malloc(bufImageSize);
    gpGlobals->wLastUnequippedItem = wItem;

    while (true)
    {
        wItem = gpGlobals->wLastUnequippedItem;

        // Draw the background
        RES_MKFDecompressChunk(&gpScreen->pixels, SCREEN_SIZE, EQUIPMENU_BACKGROUND_FBPNUM, Res_FBP);

        // Draw the item picture
        if (RES_MKFReadChunk(bufImage, bufImageSize, gpGlobals->g.rgObject[wItem].item.wBitmap, Res_BALL))
        {
            PAL_RLEBlitToSurface(bufImage, gpScreen, PAL_XY_OFFSET(EquipImageBox, 8, 8));
        }

        // Draw the current equipment of the selected player
        w = gpGlobals->rgParty[iCurrentPlayer].wPlayerRole;
        for (i = 0; i < MAX_PLAYER_EQUIPMENTS; i++)
        {
            if (gpGlobals->g.PlayerRoles->rgwEquipment[i][w] != 0)
            {
                PAL_DrawText(PAL_GetWord(gpGlobals->g.PlayerRoles->rgwEquipment[i][w]),
                             EquipNames[i], MENUITEM_COLOR, true, false);
            }
        }

        // Draw the stats of the currently selected player
        PAL_DrawNumber(PAL_GetPlayerAttackStrength(w), 4, EquipStatusValues[0], kNumColorCyan, kNumAlignRight);
        PAL_DrawNumber(PAL_GetPlayerMagicStrength(w), 4, EquipStatusValues[1], kNumColorCyan, kNumAlignRight);
        PAL_DrawNumber(PAL_GetPlayerDefense(w), 4, EquipStatusValues[2], kNumColorCyan, kNumAlignRight);
        PAL_DrawNumber(PAL_GetPlayerDexterity(w), 4, EquipStatusValues[3], kNumColorCyan, kNumAlignRight);
        PAL_DrawNumber(PAL_GetPlayerFleeRate(w), 4, EquipStatusValues[4], kNumColorCyan, kNumAlignRight);

        // Draw a box for player selection
        PAL_CreateBox(EquipRoleListBox, gpGlobals->wMaxPartyMemberIndex, PAL_WordMaxWidth(36, 4) - 1, 0, NULL);

        // Draw the label of players
        for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
        {
            w = gpGlobals->rgParty[i].wPlayerRole;

            if (iCurrentPlayer == i)
            {
                if (gpGlobals->g.rgObject[wItem].item.wFlags & (kItemFlagEquipableByPlayerRole_First << w))
                {
                    bColor = MENUITEM_COLOR_SELECTED_FIRST;
                }
                else
                {
                    bColor = MENUITEM_COLOR_SELECTED_INACTIVE;
                }
            }
            else
            {
                if (gpGlobals->g.rgObject[wItem].item.wFlags & (kItemFlagEquipableByPlayerRole_First << w))
                {
                    bColor = MENUITEM_COLOR;
                }
                else
                {
                    bColor = MENUITEM_COLOR_INACTIVE;
                }
            }

            PAL_DrawText(PAL_GetWord(gpGlobals->g.PlayerRoles->rgwName[w]),
                         PAL_XY_OFFSET(EquipRoleListBox, 13, 13 + 18 * i), bColor, true, false);
        }

        // Draw the text label and amount of the item
        if (wItem != 0)
        {
            PAL_DrawText(PAL_GetWord(wItem), EquipItemName, MENUITEM_COLOR_CONFIRMED, true, false);
            PAL_DrawNumber(PAL_GetItemAmount(wItem), 2, EquipItemAmount, kNumColorCyan, kNumAlignRight);
        }

        // Update the screen
        // VIDEO_UpdateScreen(NULL);

        // Accept input
        PAL_ClearKeyState();

        while (true)
        {
            // Redraw the selected item if needed.
            w = gpGlobals->rgParty[iCurrentPlayer].wPlayerRole;

            if (gpGlobals->g.rgObject[wItem].item.wFlags & (kItemFlagEquipableByPlayerRole_First << w))
            {
                PAL_DrawText(PAL_GetWord(gpGlobals->g.PlayerRoles->rgwName[w]),
                             PAL_XY_OFFSET(EquipRoleListBox, 13, 13 + 18 * iCurrentPlayer), MENUITEM_COLOR_SELECTED, true, true);
            }

            // Update the screen
            VIDEO_UpdateScreen(NULL);
            // Wait for any key
            if (UTIL_WaitKeys(FRAME_TIME, 0) != kKeyNone)
            {
                break;
            }
        }

        if (wItem == 0)
        {
            // return;
            break;
        }

        if (PAL_GetKeyInput() & (kKeyUp | kKeyLeft))
        {
            if (iCurrentPlayer == 0)
                iCurrentPlayer = gpGlobals->wMaxPartyMemberIndex;
            else
                iCurrentPlayer--;
        }
        else if (PAL_GetKeyInput() & (kKeyDown | kKeyRight))
        {
            if (iCurrentPlayer == gpGlobals->wMaxPartyMemberIndex)
                iCurrentPlayer = 0;
            else
                iCurrentPlayer++;
        }
        else if (PAL_GetKeyInput() & kKeyMenu)
        {
            break;
        }
        else if (PAL_GetKeyInput() & kKeySearch)
        {
            w = gpGlobals->rgParty[iCurrentPlayer].wPlayerRole;

            if (gpGlobals->g.rgObject[wItem].item.wFlags & (kItemFlagEquipableByPlayerRole_First << w))
            {
                // Run the equip script
                gpGlobals->g.rgObject[wItem].item.wScriptOnEquip =
                    PAL_RunTriggerScript(gpGlobals->g.rgObject[wItem].item.wScriptOnEquip,
                                         gpGlobals->rgParty[iCurrentPlayer].wPlayerRole);
            }
        }
    }
    UTIL_free(bufImage);
}

void PAL_QuitGame(void)
{
    unsigned short wReturnValue = PAL_ConfirmMenu(); // No config menu available
    if (wReturnValue == 1 || wReturnValue == 2)
    {
        AUDIO_PlayMusic(0x00, false, 2);
        PAL_FadeOut(2);
        PAL_Shutdown(0);
    }
}
