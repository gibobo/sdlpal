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

#include "input.h"
#include "driver.h"
#include "main.h"
#include <string.h>

volatile PALINPUTSTATE g_InputState;

static int PAL_GetCurrDirection(void)
/*++
  Purpose:
    Get the current walking direction.

  Parameters:
    None.

  Return value:
    None.

--*/
{
    PALDIRECTION i;
    PALDIRECTION iCurrDir = kDirSouth;

    for (i = kDirSouth; i != kDirUnknown; i++)
        if (g_InputState.dwKeyOrder[iCurrDir] < g_InputState.dwKeyOrder[i])
            iCurrDir = i;

    if (g_InputState.dwKeyOrder[iCurrDir] == 0)
        iCurrDir = kDirUnknown;

    return iCurrDir;
}

void PAL_KeyDown(PALKEY key)
/*++
  Purpose:
    Called when user pressed a key.

  Parameters:
    [IN]  key - keycode of the pressed key.

  Return value:
    None.

--*/
{
    PALDIRECTION iCurrDir = kDirUnknown;
    if (key & kKeyDown)
        iCurrDir = kDirSouth;
    else if (key & kKeyLeft)
        iCurrDir = kDirWest;
    else if (key & kKeyUp)
        iCurrDir = kDirNorth;
    else if (key & kKeyRight)
        iCurrDir = kDirEast;
    g_InputState.dwKeyPress |= key;

    if (iCurrDir != kDirUnknown)
    {
        if (g_InputState.dwKeyOrder[iCurrDir] == 0 || g_InputState.dwKeyOrder[iCurrDir] != g_InputState.dwKeyMaxCount)
        {
            g_InputState.dwKeyOrder[iCurrDir] = ++g_InputState.dwKeyMaxCount;
            g_InputState.dir = PAL_GetCurrDirection();
        }
    }
}

void PAL_KeyUp(PALKEY key)
/*++
  Purpose:
    Called when user released a key.

  Parameters:
    [IN]  key - keycode of the released key.

  Return value:
    None.

--*/
{
    int iCurrDir = kDirUnknown;
    if (key & kKeyDown)
        iCurrDir = kDirSouth;
    else if (key & kKeyLeft)
        iCurrDir = kDirWest;
    else if (key & kKeyUp)
        iCurrDir = kDirNorth;
    else if (key & kKeyRight)
        iCurrDir = kDirEast;

    if (iCurrDir != kDirUnknown)
    {
        g_InputState.dwKeyOrder[iCurrDir] = 0;
        iCurrDir = PAL_GetCurrDirection();
        g_InputState.dwKeyMaxCount = (iCurrDir == kDirUnknown) ? 0 : g_InputState.dwKeyOrder[iCurrDir];
        g_InputState.dir = iCurrDir;
    }
}

void PAL_ClearKeyState(void)
/*++
  Purpose:
    Clear the record of pressed keys.

  Parameters:
    None.

  Return value:
    None.

--*/
{
    g_InputState.dwKeyPress = kKeyNone;
}

void PAL_InitInput(void)
/*++
  Purpose:
    Initialize the input subsystem.

  Parameters:
    None.

  Return value:
    None.

--*/
{
    memset((void *)&g_InputState, 0, sizeof(g_InputState));
    g_InputState.dir = kDirUnknown;
}

void PAL_ShutdownInput(void)
/*++
  Purpose:
    Shutdown the input subsystem.

  Parameters:
    None.

  Return value:
    None.

--*/
{
}

void PAL_ProcessEvent(void)
/*++
  Purpose:
    Process all events.

  Parameters:
    None.

  Return value:
    None.

--*/
{
    if (DRIVER_Process_Events() == -1)
        PAL_Shutdown(0);
}

void PAL_SetKeyInput(const PALKEY key)
{
    g_InputState.dwKeyPress = key;
}

PALKEY PAL_GetKeyInput(void)
{
    return g_InputState.dwKeyPress;
}

void PAL_SetDirInput(const PALDIRECTION dir)
{
    g_InputState.dir = dir;
}

PALDIRECTION PAL_GetDirInput(void)
{
    return g_InputState.dir;
}
