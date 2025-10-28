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

#include "res.h"
#include "audio.h"
#include "driver.h"
#include "global.h"
#include "map.h"
#include "palcommon.h"
#include "resource.h"
#include "util.h"
#include <assert.h>
#include <stddef.h>

typedef struct tagRESOURCES
{
    unsigned char bLoadFlags;
    PALMAP *lpMap;                                              // current loaded map
    unsigned char **lppEventObjectSprites;                      // event object sprites
    int nEventObjectSprites;                                    // number of event objects
    unsigned char *rglpPlayerSprite[MAX_PLAYABLE_PLAYER_ROLES]; // player sprites
} RESOURCES;

static RESOURCES *gpResources = NULL;

static void
PAL_FreeEventObjectSprites(
    void)
/*++
  Purpose:

    Free all sprites of event objects on the scene.

  Parameters:

    None.

  Return value:

    None.

--*/
{
    int i;

    if (gpResources->lppEventObjectSprites != NULL)
    {
        for (i = 0; i < gpResources->nEventObjectSprites; i++)
        {
            UTIL_free(gpResources->lppEventObjectSprites[i]);
        }

        UTIL_free(gpResources->lppEventObjectSprites);

        gpResources->lppEventObjectSprites = NULL;
        gpResources->nEventObjectSprites = 0;
    }
}

static void
PAL_FreePlayerSprites(
    void)
/*++
  Purpose:

    Free all player sprites.

  Parameters:

    None.

  Return value:

    None.

--*/
{
    int i;

    for (i = 0; i < MAX_PLAYABLE_PLAYER_ROLES; i++)
    {
        UTIL_free(gpResources->rglpPlayerSprite[i]);
        gpResources->rglpPlayerSprite[i] = NULL;
    }
}

int PAL_InitResources(void)
/*++
  Purpose:

    Initialze the resource manager.

  Parameters:

    None.

  Return value:

    0 - Success
   -1 - Memory allocation failed

--*/
{
    gpResources = (RESOURCES *)UTIL_calloc(1, sizeof(RESOURCES));
    if (gpResources == NULL)
    {
        return -1; // Memory allocation failed
    }

    // Create the map instance.
    gpResources->lpMap = (PALMAP *)UTIL_malloc(sizeof(PALMAP));
    if (gpResources->lpMap == NULL)
    {
        UTIL_free(gpResources);
        return -2;
    }

    return 0; // Success
}

void PAL_FreeResources(
    void)
/*++
  Purpose:

    Free all loaded resources.

  Parameters:

    None.

  Return value:

    None.

--*/
{
    if (gpResources != NULL)
    {
        //
        // Free all loaded sprites
        //
        PAL_FreePlayerSprites();
        PAL_FreeEventObjectSprites();

        //
        // Free map
        //
        PAL_FreeMap(gpResources->lpMap);
        UTIL_free(gpResources->lpMap);

        //
        // Delete the instance
        //
        UTIL_free(gpResources);
    }

    gpResources = NULL;
}

void PAL_SetLoadFlags(
    unsigned char bFlags)
/*++
  Purpose:

    Set flags to load resources.

  Parameters:

    [IN]  bFlags - flags to be set.

  Return value:

    None.

--*/
{
    if (gpResources)
    {
        gpResources->bLoadFlags |= bFlags;
        PAL_LoadResources();
    }
}

void PAL_LoadResources(void)
/*++
  Purpose:

    Load the game resources if needed.

  Parameters:

    None.

  Return value:

    None.

--*/
{
    int i, index;
    unsigned short wPlayerID, wSpriteNum;

    if (gpResources == NULL || gpResources->bLoadFlags == kLoadNone)
    {
        return;
    }

    // Load global data
    if (gpResources->bLoadFlags & kLoadGlobalData)
    {
        PAL_InitGameData(gpGlobals->bCurrentSaveSlot);
        AUDIO_PlayMusic(gpGlobals->wNumMusic, 1, 1);
    }

    // Load scene
    if (gpResources->bLoadFlags & kLoadScene)
    {
        if (gpGlobals->fEnteringScene)
        {
            gpGlobals->wScreenWave = 0;
            gpGlobals->sWaveProgression = 0;
        }

        // Free previous loaded scene (sprites and map)
        PAL_FreeEventObjectSprites();

        // Load map
        i = gpGlobals->wNumScene - 1;
        PAL_LoadMap(gpResources->lpMap, gpGlobals->g.rgScene[i].wMapNum);

        if (gpResources->lpMap == NULL)
        {
            TerminateOnError("%s() failed: cannot load map data (map #%d for scene #%d - file not found or corrupted)\n",
                             __func__, gpGlobals->g.rgScene[i].wMapNum, gpGlobals->wNumScene);
        }

        // Load sprites
        index = gpGlobals->g.rgScene[i].wEventObjectIndex;
        gpResources->nEventObjectSprites = gpGlobals->g.rgScene[i + 1].wEventObjectIndex;
        gpResources->nEventObjectSprites -= index;

        if (gpResources->nEventObjectSprites > 0)
        {
            gpResources->lppEventObjectSprites = (unsigned char **)UTIL_calloc(gpResources->nEventObjectSprites, sizeof(unsigned char *));
        }

        for (i = 0; i < gpResources->nEventObjectSprites; i++, index++)
        {
            gpResources->lppEventObjectSprites[i] = NULL;
            if (RES_MKFDecompressChunk(&gpResources->lppEventObjectSprites[i], 0, gpGlobals->g.lprgEventObject[index].wSpriteNum, Res_MGO))
                gpGlobals->g.lprgEventObject[index].nSpriteFramesAuto = PAL_SpriteGetNumFrames(gpResources->lppEventObjectSprites[i]);
        }
        gpGlobals->partyoffset = PAL_XY(160, 112);
    }

    // Load player sprites
    if (gpResources->bLoadFlags & kLoadPlayerSprite)
    {
        // Free previous loaded player sprites
        PAL_FreePlayerSprites();

        for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
        {
            wPlayerID = gpGlobals->rgParty[i].wPlayerRole;
            assert(wPlayerID < MAX_PLAYER_ROLES);

            // Load player sprite
            wSpriteNum = gpGlobals->g.PlayerRoles->rgwSpriteNum[wPlayerID];

            RES_MKFDecompressChunk(
                &gpResources->rglpPlayerSprite[i], 0, wSpriteNum, Res_MGO);
        }

        for (i = 1; i <= gpGlobals->nFollower; i++)
        {
            // Load the follower sprite
            wSpriteNum = gpGlobals->rgParty[gpGlobals->wMaxPartyMemberIndex + i].wPlayerRole;

            RES_MKFDecompressChunk(
                &gpResources->rglpPlayerSprite[gpGlobals->wMaxPartyMemberIndex + i], 0, wSpriteNum, Res_MGO);
        }
    }

    // Clear all of the load flags
    gpResources->bLoadFlags = kLoadNone;
}

void *PAL_GetCurrentMap(void)
/*++
  Purpose:

    Get the current loaded map.

  Parameters:

    None.

  Return value:

    Pointer to the current loaded map. NULL if no map is loaded.

--*/
{
    if (gpResources == NULL)
    {
        return NULL;
    }

    return (void *)gpResources->lpMap;
}

unsigned char *
PAL_GetPlayerSprite(
    unsigned char bPlayerIndex)
/*++
  Purpose:

    Get the player sprite.

  Parameters:

    [IN]  bPlayerIndex - index of player in party (starts from 0).

  Return value:

    Pointer to the player sprite.

--*/
{
    if (gpResources == NULL || bPlayerIndex > MAX_PLAYABLE_PLAYER_ROLES - 1)
    {
        return NULL;
    }

    return gpResources->rglpPlayerSprite[bPlayerIndex];
}

unsigned char *
PAL_GetEventObjectSprite(
    unsigned short wEventObjectID)
/*++
  Purpose:

    Get the sprite of the specified event object.

  Parameters:

    [IN]  wEventObjectID - the ID of event object.

  Return value:

    Pointer to the sprite.

--*/
{
    wEventObjectID -= gpGlobals->g.rgScene[gpGlobals->wNumScene - 1].wEventObjectIndex;
    wEventObjectID--;

    if (gpResources == NULL || wEventObjectID >= gpResources->nEventObjectSprites)
    {
        return NULL;
    }

    return gpResources->lppEventObjectSprites[wEventObjectID];
}
