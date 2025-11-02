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

#include "uibattle.h"
#include "battle.h"
#include "fight.h"
#include "global.h"
#include "input.h"
#include "itemmenu.h"
#include "magicmenu.h"
#include "palcommon.h"
#include "text.h"
#include "uigame.h"
#include "util.h"
#include "video.h"
#include <stdbool.h>

static int g_iCurMiscMenuItem = 0;
static int g_iCurSubMenuItem = 0;
extern BATTLE *g_Battle;

void PAL_PlayerInfoBox(
    unsigned int pos,
    unsigned short wPlayerRole)
/*++
  Purpose:

    Show the player info box.

  Parameters:

    [IN]  pos - the top-left corner position of the box.

    [IN]  wPlayerRole - the player role ID to be shown.

    [IN]  fUpdate - whether to update the screen area or not.

  Return value:

    None.

--*/
{
    // VIDEO_Rect rect;
    unsigned char bPoisonColor;
    int i, iPartyIndex;
    unsigned short wMaxLevel, w;

    const unsigned char rgStatusPos[kStatusAll][2] = {
        {35, 19}, // confused
        {44, 12}, // slow
        {54, 1},  // sleep
        {55, 20}, // silence
        {0, 0},   // puppet
        {0, 0},   // bravery
        {0, 0},   // protect
        {0, 0},   // haste
        {0, 0},   // dualattack
    };

    const unsigned short rgwStatusWord[kStatusAll] = {
        0x1D, // confused
        0x1B, // slow
        0x1C, // sleep
        0x1A, // silence
        0x00, // puppet
        0x00, // bravery
        0x00, // protect
        0x00, // haste
        0x00, // dualattack
    };

    const unsigned char rgbStatusColor[kStatusAll] = {
        0x5F, // confused
        0xBF, // slow
        0x0E, // sleep
        0x3C, // silence
        0x00, // puppet
        0x00, // bravery
        0x00, // protect
        0x00, // haste
        0x00, // dualattack
    };

    //
    // Draw the box
    //
    PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_PLAYERINFOBOX),
                         gpScreen, pos);

    //
    // Draw the player face
    //
    wMaxLevel = 0;
    bPoisonColor = 0xFF;

    for (iPartyIndex = 0; iPartyIndex <= gpGlobals->wMaxPartyMemberIndex; iPartyIndex++)
    {
        if (gpGlobals->rgParty[iPartyIndex].wPlayerRole == wPlayerRole)
        {
            break;
        }
    }

    if (iPartyIndex <= gpGlobals->wMaxPartyMemberIndex)
    {
        for (i = 0; i < MAX_POISONS; i++)
        {
            w = gpGlobals->rgPoisonStatus[i][iPartyIndex].wPoisonID;

            if (w != 0 &&
                gpGlobals->g.rgObject[w].poison.wPoisonLevel <= 3)
            {
                if (gpGlobals->g.rgObject[w].poison.wPoisonLevel >= wMaxLevel)
                {
                    wMaxLevel = gpGlobals->g.rgObject[w].poison.wPoisonLevel;
                    bPoisonColor = (unsigned char)(gpGlobals->g.rgObject[w].poison.wColor);
                }
            }
        }
    }

    if (gpGlobals->g.PlayerRoles->rgwHP[wPlayerRole] == 0)
    {
        //
        // Always use the black/white color for dead players
        // and do not use the time meter
        //
        bPoisonColor = 0;
    }

    if (bPoisonColor == 0xFF)
    {
        PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_PLAYERFACE_FIRST + wPlayerRole),
                             gpScreen, PAL_XY(PAL_X(pos) - 2, PAL_Y(pos) - 4));
    }
    else
    {
        PAL_RLEBlitMonoColor(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_PLAYERFACE_FIRST + wPlayerRole),
                             gpScreen, PAL_XY(PAL_X(pos) - 2, PAL_Y(pos) - 4), bPoisonColor, 0);
    }

    //
    // Draw the HP and MP value
    //
    PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen,
                         PAL_XY(PAL_X(pos) + 49, PAL_Y(pos) + 6));
    PAL_DrawNumber(gpGlobals->g.PlayerRoles->rgwMaxHP[wPlayerRole], 4,
                   PAL_XY(PAL_X(pos) + 47, PAL_Y(pos) + 8), kNumColorYellow, kNumAlignRight);
    PAL_DrawNumber(gpGlobals->g.PlayerRoles->rgwHP[wPlayerRole], 4,
                   PAL_XY(PAL_X(pos) + 26, PAL_Y(pos) + 5), kNumColorYellow, kNumAlignRight);

    PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen,
                         PAL_XY(PAL_X(pos) + 49, PAL_Y(pos) + 22));
    PAL_DrawNumber(gpGlobals->g.PlayerRoles->rgwMaxMP[wPlayerRole], 4,
                   PAL_XY(PAL_X(pos) + 47, PAL_Y(pos) + 24), kNumColorCyan, kNumAlignRight);
    PAL_DrawNumber(gpGlobals->g.PlayerRoles->rgwMP[wPlayerRole], 4,
                   PAL_XY(PAL_X(pos) + 26, PAL_Y(pos) + 21), kNumColorCyan, kNumAlignRight);

    //
    // Draw Statuses
    //
    if (gpGlobals->g.PlayerRoles->rgwHP[wPlayerRole] > 0)
    {
        for (i = 0; i < kStatusAll; i++)
        {
            if (gpGlobals->rgPlayerStatus[wPlayerRole][i] > 0 &&
                rgwStatusWord[i] != 0)
            {
                PAL_DrawText(PAL_GetWord(rgwStatusWord[i]),
                             PAL_XY(PAL_X(pos) + rgStatusPos[i][0], PAL_Y(pos) + rgStatusPos[i][1]),
                             rgbStatusColor[i], true, false);
            }
        }
    }

    //
    // Update the screen area if needed
    //
    // if (fUpdate)
    // {
    //     rect.x = PAL_X(pos) - 2;
    //     rect.y = PAL_Y(pos) - 4;
    //     rect.w = 77;
    //     rect.h = 3;
    //     VIDEO_UpdateScreen(&rect);
    // }
}

static int
PAL_BattleUIIsActionValid(
    BATTLEUIACTION ActionType)
/*++
  Purpose:

    Check if the specified action is valid.

  Parameters:

    [IN]  ActionType - the type of the action.

  Return value:

    true if the action is valid, false if not.

--*/
{
    unsigned short wPlayerRole;
    int i;

    wPlayerRole = gpGlobals->rgParty[g_Battle->UI.wCurPlayerIndex].wPlayerRole;

    switch (ActionType)
    {
        case kBattleUIActionAttack:
        case kBattleUIActionMisc:
            break;

        case kBattleUIActionMagic:
            if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSilence] != 0)
            {
                return false;
            }
            break;

        case kBattleUIActionCoopMagic:
            if (gpGlobals->wMaxPartyMemberIndex == 0)
            {
                return false;
            }
            {
                int healthyNumber = 0;
                for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
                    if (PAL_IsPlayerHealthy(gpGlobals->rgParty[i].wPlayerRole))
                        healthyNumber++;
                return PAL_IsPlayerHealthy(wPlayerRole) && healthyNumber > 1;
            }
            break;
    }

    return true;
}

static void
PAL_BattleUIDrawMiscMenu(
    unsigned short wCurrentItem,
    int fConfirmed)
/*++
  Purpose:

    Draw the misc menu.

  Parameters:

    [IN]  wCurrentItem - the current selected menu item.

    [IN]  fConfirmed - true if confirmed, false if not.

  Return value:

    None.

--*/
{
    int i;
    unsigned char bColor;

    MENUITEM rgMenuItem[] = {
        // value   label                     enabled   position
        {0, BATTLEUI_LABEL_AUTO, true, PAL_XY(16, 32)},
        {1, BATTLEUI_LABEL_INVENTORY, true, PAL_XY(16, 50)},
        {2, BATTLEUI_LABEL_DEFEND, true, PAL_XY(16, 68)},
        {3, BATTLEUI_LABEL_FLEE, true, PAL_XY(16, 86)},
        {4, BATTLEUI_LABEL_STATUS, true, PAL_XY(16, 104)}};

    //
    // Draw the box
    //
    PAL_CreateBox(PAL_XY(2, 20), 4, PAL_MenuTextMaxWidth(rgMenuItem, sizeof(rgMenuItem) / sizeof(MENUITEM)) - 1, 0, NULL);

    //
    // Draw the menu items
    //
    for (i = 0; i < 5; i++)
    {
        bColor = MENUITEM_COLOR;

        if (i == wCurrentItem)
        {
            if (fConfirmed)
            {
                bColor = MENUITEM_COLOR_CONFIRMED;
            }
            else
            {
                bColor = MENUITEM_COLOR_SELECTED;
            }
        }

        PAL_DrawText(PAL_GetWord(rgMenuItem[i].wNumWord), rgMenuItem[i].pos, bColor, true, false);
    }
}

static unsigned short
PAL_BattleUIMiscMenuUpdate(
    void)
/*++
  Purpose:

    Update the misc menu.

  Parameters:

    None.

  Return value:

    The selected item number. 0 if cancelled, 0xFFFF if not confirmed.

--*/
{
    //
    // Draw the menu
    //
    PAL_BattleUIDrawMiscMenu(g_iCurMiscMenuItem, false);

    //
    // Process inputs
    //
    if (PAL_GetKeyInput() & (kKeyUp | kKeyLeft))
    {
        g_iCurMiscMenuItem--;
        if (g_iCurMiscMenuItem < 0)
        {
            g_iCurMiscMenuItem = 4;
        }
    }
    else if (PAL_GetKeyInput() & (kKeyDown | kKeyRight))
    {
        g_iCurMiscMenuItem++;
        if (g_iCurMiscMenuItem > 4)
        {
            g_iCurMiscMenuItem = 0;
        }
    }
    else if (PAL_GetKeyInput() & kKeySearch)
    {
        return g_iCurMiscMenuItem + 1;
    }
    else if (PAL_GetKeyInput() & kKeyMenu)
    {
        return 0;
    }

    return 0xFFFF;
}

static unsigned short
PAL_BattleUIMiscItemSubMenuUpdate(
    void)
/*++
  Purpose:

    Update the item sub menu of the misc menu.

  Parameters:

    None.

  Return value:

    The selected item number. 0 if cancelled, 0xFFFF if not confirmed.

--*/
{
    int i;
    unsigned char bColor;

    MENUITEM rgMenuItem[] = {
        // value   label                      enabled   position
        {0, BATTLEUI_LABEL_USEITEM, true, PAL_XY(44, 62)},
        {1, BATTLEUI_LABEL_THROWITEM, true, PAL_XY(44, 80)},
    };

    //
    // Draw the menu
    //
    PAL_BattleUIDrawMiscMenu(1, true);
    PAL_CreateBox(PAL_XY(30, 50), 1, PAL_MenuTextMaxWidth(rgMenuItem, 2) - 1, 0, NULL);

    //
    // Draw the menu items
    //
    for (i = 0; i < 2; i++)
    {
        bColor = MENUITEM_COLOR;

        if (i == g_iCurSubMenuItem)
        {
            bColor = MENUITEM_COLOR_SELECTED;
        }

        PAL_DrawText(PAL_GetWord(rgMenuItem[i].wNumWord), rgMenuItem[i].pos, bColor, true, false);
    }

    //
    // Process inputs
    //
    if (PAL_GetKeyInput() & (kKeyUp | kKeyLeft))
    {
        g_iCurSubMenuItem = 0;
    }
    else if (PAL_GetKeyInput() & (kKeyDown | kKeyRight))
    {
        g_iCurSubMenuItem = 1;
    }
    else if (PAL_GetKeyInput() & kKeySearch)
    {
        return g_iCurSubMenuItem + 1;
    }
    else if (PAL_GetKeyInput() & kKeyMenu)
    {
        return 0;
    }

    return 0xFFFF;
}

void PAL_BattleUIShowText(
    const wchar_t *lpszText,
    unsigned short wDuration)
/*++
  Purpose:

    Show a text message in the battle.

  Parameters:

    [IN]  lpszText - the text message to be shown.

    [IN]  wDuration - the duration of the message, in milliseconds.

  Return value:

    None.

--*/
{
    if (UTIL_GetMilliseconds() < g_Battle->UI.dwMsgShowTime)
    {
        wcscpy(g_Battle->UI.szNextMsg, lpszText);
        g_Battle->UI.wNextMsgDuration = wDuration;
    }
    else
    {
        wcscpy(g_Battle->UI.szMsg, lpszText);
        g_Battle->UI.dwMsgShowTime = UTIL_GetMilliseconds() + wDuration;
    }
}

void PAL_BattleUIPlayerReady(
    unsigned short wPlayerIndex)
/*++
  Purpose:

    Start the action selection menu of the specified player.

  Parameters:

    [IN]  wPlayerIndex - the player index.

  Return value:

    None.

--*/
{
    g_Battle->UI.wCurPlayerIndex = wPlayerIndex;
    g_Battle->UI.state = kBattleUISelectMove;
    g_Battle->UI.wSelectedAction = 0;
    g_Battle->UI.MenuState = kBattleMenuMain;
}

static void
PAL_BattleUIUseItem(
    void)
/*++
  Purpose:

    Use an item in the battle UI.

  Parameters:

    None.

  Return value:

    None.

--*/
{
    unsigned short wSelectedItem;

    wSelectedItem = PAL_ItemSelectMenuUpdate();

    if (wSelectedItem != 0xFFFF)
    {
        if (wSelectedItem != 0)
        {
            g_Battle->UI.wActionType = kBattleActionUseItem;
            g_Battle->UI.wObjectID = wSelectedItem;

            if (gpGlobals->g.rgObject[wSelectedItem].item.wFlags & kItemFlagApplyToAll)
            {
                g_Battle->UI.state = kBattleUISelectTargetPlayerAll;
            }
            else
            {
                g_Battle->UI.iSelectedIndex = 0;
                g_Battle->UI.state = kBattleUISelectTargetPlayer;
            }
        }
        else
        {
            g_Battle->UI.MenuState = kBattleMenuMain;
        }
    }
}

static void
PAL_BattleUIThrowItem(
    void)
/*++
  Purpose:

    Throw an item in the battle UI.

  Parameters:

    None.

  Return value:

    None.

--*/
{
    unsigned short wSelectedItem = PAL_ItemSelectMenuUpdate();

    if (wSelectedItem != 0xFFFF)
    {
        if (wSelectedItem != 0)
        {
            g_Battle->UI.wActionType = kBattleActionThrowItem;
            g_Battle->UI.wObjectID = wSelectedItem;

            if (gpGlobals->g.rgObject[wSelectedItem].item.wFlags & kItemFlagApplyToAll)
            {
                g_Battle->UI.state = kBattleUISelectTargetEnemyAll;
            }
            else
            {
                g_Battle->UI.state = kBattleUISelectTargetEnemy;
                g_Battle->UI.iSelectedIndex = 0;
            }
        }
        else
        {
            g_Battle->UI.MenuState = kBattleMenuMain;
        }
    }
}

static unsigned short
PAL_BattleUIPickAutoMagic(
    unsigned short wPlayerRole,
    unsigned short wRandomRange)
/*++
  Purpose:

    Pick a magic for the specified player for automatic usage.

  Parameters:

    [IN]  wPlayerRole - the player role ID.

    [IN]  wRandomRange - the range of the magic power.

  Return value:

    The object ID of the selected magic. 0 for physical attack.

--*/
{
    unsigned short wMagic = 0, w, wMagicNum;
    int i, iMaxPower = 0, iPower;

    if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSilence] != 0)
    {
        return 0;
    }

    for (i = 0; i < MAX_PLAYER_MAGICS; i++)
    {
        w = gpGlobals->g.PlayerRoles->rgwMagic[i][wPlayerRole];
        if (w == 0)
        {
            continue;
        }

        wMagicNum = gpGlobals->g.rgObject[w].magic.wMagicNumber;

        //
        // skip if the magic is an ultimate move or not enough MP
        //
        if (gpGlobals->g.lprgMagic[wMagicNum].wCostMP == 1 ||
            gpGlobals->g.lprgMagic[wMagicNum].wCostMP > gpGlobals->g.PlayerRoles->rgwMP[wPlayerRole] ||
            (short)(gpGlobals->g.lprgMagic[wMagicNum].wBaseDamage) <= 0)
        {
            continue;
        }

        iPower = (short)(gpGlobals->g.lprgMagic[wMagicNum].wBaseDamage) +
                 RandomLong(0, wRandomRange);

        if (iPower > iMaxPower)
        {
            iMaxPower = iPower;
            wMagic = w;
        }
    }

    return wMagic;
}

void PAL_BattleUIUpdate(
    void)
/*++
  Purpose:

    Update the status of battle UI.

  Parameters:

    None.

  Return value:

    None.

--*/
{
    int i, j, x, y;
    unsigned short wPlayerRole = 0, w;
    static int s_iFrame = 0;

    struct
    {
        int iSpriteNum;
        unsigned int pos;
        BATTLEUIACTION action;
    } rgItems[] =
        {
            {SPRITENUM_BATTLEICON_ATTACK, PAL_XY(27, 140), kBattleUIActionAttack},
            {SPRITENUM_BATTLEICON_MAGIC, PAL_XY(0, 155), kBattleUIActionMagic},
            {SPRITENUM_BATTLEICON_COOPMAGIC, PAL_XY(54, 155), kBattleUIActionCoopMagic},
            {SPRITENUM_BATTLEICON_MISCMENU, PAL_XY(27, 170), kBattleUIActionMisc}};

    s_iFrame++;

    if (g_Battle->UI.fAutoAttack && !gpGlobals->fAutoBattle)
    {
        //
        // Draw the "auto attack" message if in the autoattack mode.
        //
        if (PAL_GetKeyInput() & kKeyMenu)
        {
            g_Battle->UI.fAutoAttack = false;
        }
        else
        {
            const wchar_t *itemText = PAL_GetWord(BATTLEUI_LABEL_AUTO);
            PAL_DrawText(itemText, PAL_XY(312 - PAL_TextWidth(itemText), 10),
                         MENUITEM_COLOR_CONFIRMED, true, false);
        }
    }

    if (gpGlobals->fAutoBattle)
    {
        PAL_BattlePlayerCheckReady();

        for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
        {
            if (g_Battle->rgPlayer[i].state == kFighterCom)
            {
                PAL_BattleUIPlayerReady(i);
                break;
            }
        }

        if (g_Battle->UI.state != kBattleUIWait)
        {
            w = PAL_BattleUIPickAutoMagic(gpGlobals->rgParty[g_Battle->UI.wCurPlayerIndex].wPlayerRole, 9999);

            if (w == 0)
            {
                g_Battle->UI.wActionType = kBattleActionAttack;
                g_Battle->UI.iSelectedIndex = PAL_BattleSelectAutoTarget();
            }
            else
            {
                g_Battle->UI.wActionType = kBattleActionMagic;
                g_Battle->UI.wObjectID = w;

                if (gpGlobals->g.rgObject[w].magic.wFlags & kMagicFlagApplyToAll)
                {
                    g_Battle->UI.iSelectedIndex = -1;
                }
                else
                {
                    g_Battle->UI.iSelectedIndex = PAL_BattleSelectAutoTarget();
                }
            }

            PAL_BattleCommitAction(false);
        }

        goto end;
    }

    if (PAL_GetKeyInput() & kKeyAuto)
    {
        g_Battle->UI.fAutoAttack = !g_Battle->UI.fAutoAttack;
        g_Battle->UI.MenuState = kBattleMenuMain;
    }

    if (g_Battle->Phase == kBattlePhasePerformAction)
    {
        goto end;
    }

    if (!g_Battle->UI.fAutoAttack)
    {
        //
        // Draw the player info boxes.
        //
        for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
        {
            wPlayerRole = gpGlobals->rgParty[i].wPlayerRole;
            PAL_PlayerInfoBox(PAL_XY(91 + 77 * i, 165), wPlayerRole);
        }
    }

    if (PAL_GetKeyInput() & kKeyStatus)
    {
        PAL_PlayerStatus();
        goto end;
    }

    if (g_Battle->UI.state != kBattleUIWait)
    {
        wPlayerRole = gpGlobals->rgParty[g_Battle->UI.wCurPlayerIndex].wPlayerRole;

        if (gpGlobals->g.PlayerRoles->rgwHP[wPlayerRole] == 0 &&
            gpGlobals->rgPlayerStatus[wPlayerRole][kStatusPuppet])
        {
            g_Battle->UI.wActionType = kBattleActionAttack;

            if (PAL_PlayerCanAttackAll(gpGlobals->rgParty[g_Battle->UI.wCurPlayerIndex].wPlayerRole))
            {
                g_Battle->UI.iSelectedIndex = -1;
            }
            else
            {
                g_Battle->UI.iSelectedIndex = PAL_BattleSelectAutoTarget();
            }

            PAL_BattleCommitAction(false);
            goto end; // don't go further
        }

        //
        // Cancel any actions if player is dead or sleeping.
        //
        if (gpGlobals->g.PlayerRoles->rgwHP[wPlayerRole] == 0 ||
            gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSleep] != 0 ||
            gpGlobals->rgPlayerStatus[wPlayerRole][kStatusParalyzed] != 0)
        {
            g_Battle->UI.wActionType = kBattleActionPass;
            PAL_BattleCommitAction(false);
            goto end; // don't go further
        }

        if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusConfused] != 0)
        {
            g_Battle->UI.wActionType = kBattleActionAttackMate;
            PAL_BattleCommitAction(false);
            goto end; // don't go further
        }

        if (g_Battle->UI.fAutoAttack)
        {
            g_Battle->UI.wActionType = kBattleActionAttack;

            if (PAL_PlayerCanAttackAll(gpGlobals->rgParty[g_Battle->UI.wCurPlayerIndex].wPlayerRole))
            {
                g_Battle->UI.iSelectedIndex = -1;
            }
            else
            {
                g_Battle->UI.iSelectedIndex = PAL_BattleSelectAutoTarget();
            }

            PAL_BattleCommitAction(false);
            goto end; // don't go further
        }

        //
        // Draw the arrow on the player's head.
        //
        i = SPRITENUM_BATTLE_ARROW_CURRENTPLAYER_RED;
        if (s_iFrame & 1)
        {
            i = SPRITENUM_BATTLE_ARROW_CURRENTPLAYER;
        }

        PAL_GetPlayerPos((unsigned char)g_Battle->UI.wCurPlayerIndex, &x, &y);
        x -= 8;
        y -= 74;

        PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, i), gpScreen, PAL_XY(x, y));
    }

    switch (g_Battle->UI.state)
    {
        case kBattleUIWait:
            if (!g_Battle->fEnemyCleared)
            {
                PAL_BattlePlayerCheckReady();

                for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
                {
                    if (g_Battle->rgPlayer[i].state == kFighterCom)
                    {
                        PAL_BattleUIPlayerReady(i);
                        break;
                    }
                }
            }
            break;

        case kBattleUISelectMove:
            //
            // Draw the icons
            //
            {
                if (g_Battle->UI.MenuState == kBattleMenuMain)
                {
                    if (PAL_GetDirInput() == kDirNorth)
                    {
                        g_Battle->UI.wSelectedAction = 0;
                    }
                    else if (PAL_GetDirInput() == kDirSouth)
                    {
                        g_Battle->UI.wSelectedAction = 3;
                    }
                    else if (PAL_GetDirInput() == kDirWest)
                    {
                        if (PAL_BattleUIIsActionValid(kBattleUIActionMagic))
                        {
                            g_Battle->UI.wSelectedAction = 1;
                        }
                    }
                    else if (PAL_GetDirInput() == kDirEast)
                    {
                        if (PAL_BattleUIIsActionValid(kBattleUIActionCoopMagic))
                        {
                            g_Battle->UI.wSelectedAction = 2;
                        }
                    }
                }

                if (!PAL_BattleUIIsActionValid(rgItems[g_Battle->UI.wSelectedAction].action))
                {
                    g_Battle->UI.wSelectedAction = 0;
                }

                for (i = 0; i < 4; i++)
                {
                    if (g_Battle->UI.wSelectedAction == i)
                    {
                        PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, rgItems[i].iSpriteNum),
                                             gpScreen, rgItems[i].pos);
                    }
                    else if (PAL_BattleUIIsActionValid(rgItems[i].action))
                    {
                        PAL_RLEBlitMonoColor(PAL_SpriteGetFrame(gpSpriteUI, rgItems[i].iSpriteNum),
                                             gpScreen, rgItems[i].pos, 0, -4);
                    }
                    else
                    {
                        PAL_RLEBlitMonoColor(PAL_SpriteGetFrame(gpSpriteUI, rgItems[i].iSpriteNum),
                                             gpScreen, rgItems[i].pos, 0x10, -4);
                    }
                }

                switch (g_Battle->UI.MenuState)
                {
                    case kBattleMenuMain:
                        if (PAL_GetKeyInput() & kKeySearch)
                        {
                            switch (g_Battle->UI.wSelectedAction)
                            {
                                case 0:
                                    //
                                    // Attack
                                    //
                                    g_Battle->UI.wActionType = kBattleActionAttack;
                                    if (PAL_PlayerCanAttackAll(gpGlobals->rgParty[g_Battle->UI.wCurPlayerIndex].wPlayerRole))
                                    {
                                        g_Battle->UI.state = kBattleUISelectTargetEnemyAll;
                                    }
                                    else
                                    {
                                        g_Battle->UI.state = kBattleUISelectTargetEnemy;
                                        g_Battle->UI.iSelectedIndex = 0;
                                    }
                                    break;

                                case 1:
                                    //
                                    // Magic
                                    //
                                    g_Battle->UI.MenuState = kBattleMenuMagicSelect;
                                    PAL_MagicSelectionMenuInit(wPlayerRole, true, 0);
                                    break;

                                case 2:
                                    //
                                    // Cooperative magic
                                    //
                                    w = gpGlobals->rgParty[g_Battle->UI.wCurPlayerIndex].wPlayerRole;
                                    w = PAL_GetPlayerCooperativeMagic(w);

                                    g_Battle->UI.wActionType = kBattleActionCoopMagic;
                                    g_Battle->UI.wObjectID = w;

                                    if (gpGlobals->g.rgObject[w].magic.wFlags & kMagicFlagUsableToEnemy)
                                    {
                                        if (gpGlobals->g.rgObject[w].magic.wFlags & kMagicFlagApplyToAll)
                                        {
                                            g_Battle->UI.state = kBattleUISelectTargetEnemyAll;
                                        }
                                        else
                                        {
                                            g_Battle->UI.state = kBattleUISelectTargetEnemy;
                                            g_Battle->UI.iSelectedIndex = 0;
                                        }
                                    }
                                    else
                                    {
                                        if (gpGlobals->g.rgObject[w].magic.wFlags & kMagicFlagApplyToAll)
                                        {
                                            g_Battle->UI.state = kBattleUISelectTargetPlayerAll;
                                        }
                                        else
                                        {
                                            g_Battle->UI.iSelectedIndex = 0;
                                            g_Battle->UI.state = kBattleUISelectTargetPlayer;
                                        }
                                    }
                                    break;

                                case 3:
                                    //
                                    // Misc menu
                                    //
                                    g_Battle->UI.MenuState = kBattleMenuMisc;
                                    //                  g_iCurMiscMenuItem = 0; //disabled due to not same as both original version
                                    break;
                            }
                        }
                        else if (PAL_GetKeyInput() & kKeyDefend)
                        {
                            g_Battle->UI.wActionType = kBattleActionDefend;
                            PAL_BattleCommitAction(false);
                        }
                        else if (PAL_GetKeyInput() & kKeyForce)
                        {
                            w = PAL_BattleUIPickAutoMagic(gpGlobals->rgParty[g_Battle->UI.wCurPlayerIndex].wPlayerRole, 60);

                            if (w == 0)
                            {
                                g_Battle->UI.wActionType = kBattleActionAttack;

                                if (PAL_PlayerCanAttackAll(gpGlobals->rgParty[g_Battle->UI.wCurPlayerIndex].wPlayerRole))
                                {
                                    g_Battle->UI.iSelectedIndex = -1;
                                }
                                else
                                {
                                    g_Battle->UI.iSelectedIndex = PAL_BattleSelectAutoTarget();
                                }
                            }
                            else
                            {
                                g_Battle->UI.wActionType = kBattleActionMagic;
                                g_Battle->UI.wObjectID = w;

                                if (gpGlobals->g.rgObject[w].magic.wFlags & kMagicFlagApplyToAll)
                                {
                                    g_Battle->UI.iSelectedIndex = -1;
                                }
                                else
                                {
                                    g_Battle->UI.iSelectedIndex = PAL_BattleSelectAutoTarget();
                                }
                            }

                            PAL_BattleCommitAction(false);
                        }
                        else if (PAL_GetKeyInput() & kKeyFlee)
                        {
                            g_Battle->UI.wActionType = kBattleActionFlee;
                            PAL_BattleCommitAction(false);
                        }
                        else if (PAL_GetKeyInput() & kKeyUseItem)
                        {
                            g_Battle->UI.MenuState = kBattleMenuUseItemSelect;
                            PAL_ItemSelectMenuInit(kItemFlagUsable);
                        }
                        else if (PAL_GetKeyInput() & kKeyThrowItem)
                        {
                            g_Battle->UI.MenuState = kBattleMenuThrowItemSelect;
                            PAL_ItemSelectMenuInit(kItemFlagThrowable);
                        }
                        else if (PAL_GetKeyInput() & kKeyRepeat)
                        {
                            PAL_BattleCommitAction(true);
                        }
                        else if (PAL_GetKeyInput() & kKeyMenu)
                        {
                            g_Battle->rgPlayer[g_Battle->UI.wCurPlayerIndex].state = kFighterWait;
                            g_Battle->UI.state = kBattleUIWait;

                            if (g_Battle->UI.wCurPlayerIndex > 0)
                            {
                                //
                                // Revert to the previous player
                                //
                                do
                                {
                                    g_Battle->rgPlayer[--g_Battle->UI.wCurPlayerIndex].state = kFighterWait;

                                    if (g_Battle->rgPlayer[g_Battle->UI.wCurPlayerIndex].action.ActionType == kBattleActionThrowItem)
                                    {
                                        for (i = 0; i < MAX_INVENTORY; i++)
                                        {
                                            if (gpGlobals->rgInventory[i].wItem ==
                                                g_Battle->rgPlayer[g_Battle->UI.wCurPlayerIndex].action.wActionID)
                                            {
                                                gpGlobals->rgInventory[i].nAmountInUse--;
                                                break;
                                            }
                                        }
                                    }
                                    else if (g_Battle->rgPlayer[g_Battle->UI.wCurPlayerIndex].action.ActionType == kBattleActionUseItem)
                                    {
                                        if (gpGlobals->g.rgObject[g_Battle->rgPlayer[g_Battle->UI.wCurPlayerIndex].action.wActionID].item.wFlags & kItemFlagConsuming)
                                        {
                                            for (i = 0; i < MAX_INVENTORY; i++)
                                            {
                                                if (gpGlobals->rgInventory[i].wItem ==
                                                    g_Battle->rgPlayer[g_Battle->UI.wCurPlayerIndex].action.wActionID)
                                                {
                                                    gpGlobals->rgInventory[i].nAmountInUse--;
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                } while (g_Battle->UI.wCurPlayerIndex > 0 &&
                                         (gpGlobals->g.PlayerRoles->rgwHP[gpGlobals->rgParty[g_Battle->UI.wCurPlayerIndex].wPlayerRole] == 0 ||
                                          gpGlobals->rgPlayerStatus[gpGlobals->rgParty[g_Battle->UI.wCurPlayerIndex].wPlayerRole][kStatusConfused] > 0 ||
                                          gpGlobals->rgPlayerStatus[gpGlobals->rgParty[g_Battle->UI.wCurPlayerIndex].wPlayerRole][kStatusSleep] > 0 ||
                                          gpGlobals->rgPlayerStatus[gpGlobals->rgParty[g_Battle->UI.wCurPlayerIndex].wPlayerRole][kStatusParalyzed] > 0));
                            }
                        }
                        break;

                    case kBattleMenuMagicSelect:
                        w = PAL_MagicSelectionMenuUpdate();

                        if (w != 0xFFFF)
                        {
                            g_Battle->UI.MenuState = kBattleMenuMain;

                            if (w != 0)
                            {
                                g_Battle->UI.wActionType = kBattleActionMagic;
                                g_Battle->UI.wObjectID = w;

                                if (gpGlobals->g.rgObject[w].magic.wFlags & kMagicFlagUsableToEnemy)
                                {
                                    if (gpGlobals->g.rgObject[w].magic.wFlags & kMagicFlagApplyToAll)
                                    {
                                        g_Battle->UI.state = kBattleUISelectTargetEnemyAll;
                                    }
                                    else
                                    {
                                        g_Battle->UI.state = kBattleUISelectTargetEnemy;
                                        g_Battle->UI.iSelectedIndex = 0;
                                    }
                                }
                                else
                                {
                                    if (gpGlobals->g.rgObject[w].magic.wFlags & kMagicFlagApplyToAll)
                                    {
                                        g_Battle->UI.state = kBattleUISelectTargetPlayerAll;
                                    }
                                    else
                                    {
                                        g_Battle->UI.iSelectedIndex = 0;
                                        g_Battle->UI.state = kBattleUISelectTargetPlayer;
                                    }
                                }
                            }
                        }
                        break;

                    case kBattleMenuUseItemSelect:
                        PAL_BattleUIUseItem();
                        break;

                    case kBattleMenuThrowItemSelect:
                        PAL_BattleUIThrowItem();
                        break;

                    case kBattleMenuMisc:
                        w = PAL_BattleUIMiscMenuUpdate();

                        if (w != 0xFFFF)
                        {
                            g_Battle->UI.MenuState = kBattleMenuMain;

                            switch (w)
                            {
                                case 2: // item
                                    g_Battle->UI.MenuState = kBattleMenuMiscItemSubMenu;
                                    //                  g_iCurSubMenuItem = 0; //disabled due to not same as both original version
                                    break;

                                case 3: // defend
                                    g_Battle->UI.wActionType = kBattleActionDefend;
                                    PAL_BattleCommitAction(false);
                                    break;

                                case 1: // auto
                                    g_Battle->UI.fAutoAttack = true;
                                    break;

                                case 4: // flee
                                    g_Battle->UI.wActionType = kBattleActionFlee;
                                    PAL_BattleCommitAction(false);
                                    break;

                                case 5: // status
                                    PAL_PlayerStatus();
                                    break;
                            }
                        }
                        break;

                    case kBattleMenuMiscItemSubMenu:
                        w = PAL_BattleUIMiscItemSubMenuUpdate();

                        if (w != 0xFFFF)
                        {
                            g_Battle->UI.MenuState = kBattleMenuMain;

                            switch (w)
                            {
                                case 1: // use
                                    g_Battle->UI.MenuState = kBattleMenuUseItemSelect;
                                    PAL_ItemSelectMenuInit(kItemFlagUsable);
                                    break;

                                case 2: // throw
                                    g_Battle->UI.MenuState = kBattleMenuThrowItemSelect;
                                    PAL_ItemSelectMenuInit(kItemFlagThrowable);
                                    break;
                            }
                        }
                        break;
                }
            }
            break;

        case kBattleUISelectTargetEnemy:
            x = -1;
            y = 0;

            for (i = 0; i <= g_Battle->wMaxEnemyIndex; i++)
            {
                if (g_Battle->rgEnemy[i].wObjectID != 0)
                {
                    x = i;
                    y++;
                }
            }

            if (x == -1)
            {
                g_Battle->UI.state = kBattleUISelectMove;
                break;
            }

            if (g_Battle->UI.wActionType == kBattleActionCoopMagic)
            {
                if (!PAL_BattleUIIsActionValid(kBattleUIActionCoopMagic))
                {
                    g_Battle->UI.state = kBattleUISelectMove;
                    break;
                }
            }

            //
            // Don't bother selecting when only 1 enemy left
            //
            if (y == 1)
            {
                if (g_Battle->UI.iSelectedIndex == -1)
                    g_Battle->UI.iSelectedIndex = x;
                else
                    for (g_Battle->UI.iSelectedIndex = 0; g_Battle->UI.iSelectedIndex < MAX_ENEMIES_IN_TEAM; g_Battle->UI.iSelectedIndex++)
                        if (g_Battle->rgEnemy[g_Battle->UI.iSelectedIndex].wObjectID != 0)
                            break;
                PAL_BattleCommitAction(false);
                break;
            }
            if (g_Battle->UI.iSelectedIndex > x)
            {
                g_Battle->UI.iSelectedIndex = x;
            }
            else if (g_Battle->UI.iSelectedIndex < 0)
            {
                g_Battle->UI.iSelectedIndex = 0;
            }

            for (i = 0; i <= x; i++)
            {
                if (g_Battle->rgEnemy[g_Battle->UI.iSelectedIndex].wObjectID != 0)
                {
                    break;
                }
                g_Battle->UI.iSelectedIndex++;
                g_Battle->UI.iSelectedIndex %= x + 1;
            }

            //
            // Highlight the selected enemy
            //
            if (s_iFrame & 1)
            {
                i = g_Battle->UI.iSelectedIndex;

                x = PAL_X(g_Battle->rgEnemy[i].pos);
                y = PAL_Y(g_Battle->rgEnemy[i].pos);

                x -= PAL_RLEGetWidth(PAL_SpriteGetFrame(g_Battle->rgEnemy[i].lpSprite, g_Battle->rgEnemy[i].wCurrentFrame)) / 2;
                y -= PAL_RLEGetHeight(PAL_SpriteGetFrame(g_Battle->rgEnemy[i].lpSprite, g_Battle->rgEnemy[i].wCurrentFrame));

                PAL_RLEBlitWithColorShift(PAL_SpriteGetFrame(g_Battle->rgEnemy[i].lpSprite, g_Battle->rgEnemy[i].wCurrentFrame),
                                          gpScreen, PAL_XY(x, y), 7);
            }

            if (PAL_GetKeyInput() & kKeyMenu)
            {
                g_Battle->UI.state = kBattleUISelectMove;
            }
            else if (PAL_GetKeyInput() & kKeySearch)
            {
                PAL_BattleCommitAction(false);
            }
            else if (PAL_GetKeyInput() & (kKeyLeft | kKeyDown))
            {
                g_Battle->UI.iSelectedIndex--;
                if (g_Battle->UI.iSelectedIndex < 0)
                    g_Battle->UI.iSelectedIndex = MAX_ENEMIES_IN_TEAM - 1;
                while (g_Battle->UI.iSelectedIndex != 0 &&
                       g_Battle->rgEnemy[g_Battle->UI.iSelectedIndex].wObjectID == 0)
                {
                    g_Battle->UI.iSelectedIndex--;
                    if (g_Battle->UI.iSelectedIndex < 0)
                        g_Battle->UI.iSelectedIndex = MAX_ENEMIES_IN_TEAM - 1;
                }
            }
            else if (PAL_GetKeyInput() & (kKeyRight | kKeyUp))
            {
                g_Battle->UI.iSelectedIndex++;
                if (g_Battle->UI.iSelectedIndex >= MAX_ENEMIES_IN_TEAM)
                    g_Battle->UI.iSelectedIndex = 0;
                while (g_Battle->UI.iSelectedIndex < MAX_ENEMIES_IN_TEAM &&
                       g_Battle->rgEnemy[g_Battle->UI.iSelectedIndex].wObjectID == 0)
                {
                    g_Battle->UI.iSelectedIndex++;
                    if (g_Battle->UI.iSelectedIndex >= MAX_ENEMIES_IN_TEAM)
                        g_Battle->UI.iSelectedIndex = 0;
                }
            }
            break;

        case kBattleUISelectTargetPlayer:
            //
            // Don't bother selecting when only 1 player is in the party
            //
            if (gpGlobals->wMaxPartyMemberIndex == 0)
            {
                g_Battle->UI.iSelectedIndex = 0;
                PAL_BattleCommitAction(false);
            }

            for (i = 0; i < 4; i++)
            {
                PAL_RLEBlitMonoColor(PAL_SpriteGetFrame(gpSpriteUI, rgItems[i].iSpriteNum),
                                     gpScreen, rgItems[i].pos, 0, -4);
            }

            j = SPRITENUM_BATTLE_ARROW_SELECTEDPLAYER;
            if (s_iFrame & 1)
            {
                j = SPRITENUM_BATTLE_ARROW_SELECTEDPLAYER_RED;
            }

            //
            // Draw arrows on the selected player
            //
            PAL_GetPlayerPos(g_Battle->UI.iSelectedIndex, &x, &y);
            x -= 8;
            y -= 67;

            PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, j), gpScreen, PAL_XY(x, y));

            if (PAL_GetKeyInput() & kKeyMenu)
            {
                g_Battle->UI.state = kBattleUISelectMove;
            }
            else if (PAL_GetKeyInput() & kKeySearch)
            {
                PAL_BattleCommitAction(false);
            }
            else if (PAL_GetKeyInput() & (kKeyLeft | kKeyDown))
            {
                if (g_Battle->UI.iSelectedIndex != 0)
                {
                    g_Battle->UI.iSelectedIndex--;
                }
                else
                {
                    g_Battle->UI.iSelectedIndex = gpGlobals->wMaxPartyMemberIndex;
                }
            }
            else if (PAL_GetKeyInput() & (kKeyRight | kKeyUp))
            {
                if (g_Battle->UI.iSelectedIndex < gpGlobals->wMaxPartyMemberIndex)
                {
                    g_Battle->UI.iSelectedIndex++;
                }
                else
                {
                    g_Battle->UI.iSelectedIndex = 0;
                }
            }

            break;

        case kBattleUISelectTargetEnemyAll:
            //
            // Don't bother selecting
            //
            g_Battle->UI.iSelectedIndex = (unsigned short)-1;
            PAL_BattleCommitAction(false);
            break;

        case kBattleUISelectTargetPlayerAll:
            //
            // Don't bother selecting
            //
            g_Battle->UI.iSelectedIndex = (unsigned short)-1;
            PAL_BattleCommitAction(false);
            break;
    }

end:

    //
    // Draw the numbers
    //
    for (i = 0; i < BATTLEUI_MAX_SHOWNUM; i++)
    {
        if (g_Battle->UI.rgShowNum[i].wNum > 0)
        {
            int ticks = ((int)UTIL_GetMilliseconds() - (int)g_Battle->UI.rgShowNum[i].dwTime) / BATTLE_FRAME_TIME;
            if (ticks > 10)
            {
                g_Battle->UI.rgShowNum[i].wNum = 0;
            }
            else
            {
                PAL_DrawNumber(g_Battle->UI.rgShowNum[i].wNum, 5,
                               PAL_XY_OFFSET(g_Battle->UI.rgShowNum[i].pos, 0, -(int)ticks),
                               g_Battle->UI.rgShowNum[i].color, kNumAlignRight);
            }
        }
    }

    PAL_ClearKeyState();
}

void PAL_BattleUIShowNum(
    unsigned short wNum,
    unsigned int pos,
    NUMCOLOR color)
/*++
  Purpose:

    Show a number on battle screen (indicates HP/MP change).

  Parameters:

    [IN]  wNum - number to be shown.

    [IN]  pos - position of the number on the screen.

    [IN]  color - color of the number.

  Return value:

    None.

--*/
{
    int i;

    for (i = 0; i < BATTLEUI_MAX_SHOWNUM; i++)
    {
        if (g_Battle->UI.rgShowNum[i].wNum == 0)
        {
            g_Battle->UI.rgShowNum[i].wNum = wNum;
            g_Battle->UI.rgShowNum[i].pos = PAL_XY_OFFSET(pos, -15, 0);
            g_Battle->UI.rgShowNum[i].color = color;
            g_Battle->UI.rgShowNum[i].dwTime = UTIL_GetMilliseconds();
            break;
        }
    }
}
