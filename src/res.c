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
#include "common.h"
#include "driver.h"
#include "global.h"
#include "palcommon.h"
#include "util.h"

typedef struct tagRESOURCES {
  unsigned char bLoadFlags;
  PALMAP *lpMap;                                              // current loaded map
  unsigned char **lppEventObjectSprites;                      // event object sprites
  int nEventObject;                                           // number of event objects
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
      for (i = 0; i < gpResources->nEventObject; i++)
      {
         UTIL_free(gpResources->lppEventObjectSprites[i]);
      }

      UTIL_free(gpResources->lppEventObjectSprites);

      gpResources->lppEventObjectSprites = NULL;
      gpResources->nEventObject = 0;
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

void PAL_InitResources(void)
/*++
  Purpose:

    Initialze the resource manager.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   gpResources = (RESOURCES *)UTIL_calloc(1, sizeof(RESOURCES));
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
   if (gpResources == NULL)
   {
      return;
   }

   gpResources->bLoadFlags |= bFlags;
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

   if (gpResources == NULL || gpResources->bLoadFlags == 0) {
      return;
   }

   // Load global data
   if (gpResources->bLoadFlags & kLoadGlobalData) {
      PAL_InitGameData(gpGlobals->bCurrentSaveSlot);
      AUDIO_PlayMusic(gpGlobals->wNumMusic, 1, 1);
   }

   // Load scene
   if (gpResources->bLoadFlags & kLoadScene) {
      void *fpMAP, *fpGOP;

      fpMAP = UTIL_fopen(RESOURCE_PATH "/map.mkf", "rb");
      fpGOP = UTIL_fopen(RESOURCE_PATH "/gop.mkf", "rb");

      if (gpGlobals->fEnteringScene) {
         gpGlobals->wScreenWave = 0;
         gpGlobals->sWaveProgression = 0;
      }

      // Free previous loaded scene (sprites and map)
      PAL_FreeEventObjectSprites();
      PAL_FreeMap(gpResources->lpMap);

      // Load map
      i = gpGlobals->wNumScene - 1;
      gpResources->lpMap = PAL_LoadMap(gpGlobals->g.rgScene[i].wMapNum,
                                       fpMAP, fpGOP);

      if (gpResources->lpMap == NULL) {
         UTIL_fclose(fpMAP);
         UTIL_fclose(fpGOP);

         TerminateOnError("PAL_LoadResources(): Fail to load map #%d (scene #%d) !",
                        gpGlobals->g.rgScene[i].wMapNum, gpGlobals->wNumScene);
      }

      // Load sprites
      index = gpGlobals->g.rgScene[i].wEventObjectIndex;
      gpResources->nEventObject = gpGlobals->g.rgScene[i + 1].wEventObjectIndex;
      gpResources->nEventObject -= index;

      if (gpResources->nEventObject > 0) {
         gpResources->lppEventObjectSprites = (unsigned char **)UTIL_calloc(gpResources->nEventObject, sizeof(unsigned char *));
      }

      for (i = 0; i < gpResources->nEventObject; i++, index++) {
         gpResources->lppEventObjectSprites[i] = NULL;
         if (PAL_MKFDecompressChunk(&gpResources->lppEventObjectSprites[i], 0, gpGlobals->g.lprgEventObject[index].wSpriteNum, gpGlobals->f.fpMGO) > 0) {
         gpGlobals->g.lprgEventObject[index].nSpriteFramesAuto = PAL_SpriteGetNumFrames(gpResources->lppEventObjectSprites[i]);
         }
      }

      gpGlobals->partyoffset = PAL_XY(160, 112);

      UTIL_fclose(fpGOP);
      UTIL_fclose(fpMAP);
   }

   // Load player sprites
   if (gpResources->bLoadFlags & kLoadPlayerSprite) {
      // Free previous loaded player sprites
      PAL_FreePlayerSprites();

      for (i = 0; i <= (short)gpGlobals->wMaxPartyMemberIndex; i++) {
         wPlayerID = gpGlobals->rgParty[i].wPlayerRole;
         assert(wPlayerID < MAX_PLAYER_ROLES);

         // Load player sprite
         wSpriteNum = gpGlobals->g.PlayerRoles.rgwSpriteNum[wPlayerID];

         PAL_MKFDecompressChunk(&gpResources->rglpPlayerSprite[i], 0, wSpriteNum,
                              gpGlobals->f.fpMGO);
      }

      for (i = 1; i <= gpGlobals->nFollower; i++) {
         // Load the follower sprite
         wSpriteNum = gpGlobals->rgParty[(short)gpGlobals->wMaxPartyMemberIndex + i].wPlayerRole;

         PAL_MKFDecompressChunk(&gpResources->rglpPlayerSprite[(short)gpGlobals->wMaxPartyMemberIndex + i], 0, wSpriteNum,
                              gpGlobals->f.fpMGO);
      }
   }

   // Clear all of the load flags
   gpResources->bLoadFlags = 0;
}

PALMAP *PAL_GetCurrentMap(void)
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

   return gpResources->lpMap;
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

   if (gpResources == NULL || wEventObjectID >= gpResources->nEventObject)
   {
      return NULL;
   }

   return gpResources->lppEventObjectSprites[wEventObjectID];
}
