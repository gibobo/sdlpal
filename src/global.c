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

#include "global.h"
#include "driver.h"
#include "palcommon.h"
#include "res.h"
#include "script.h"
#include "util.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

PALFILE gFiles[] = {
    [Res_FBP] = {"fbp.mkf", NULL},
    [Res_MGO] = {"mgo.mkf", NULL},
    [Res_BALL] = {"ball.mkf", NULL},
    [Res_DATA] = {"data.mkf", NULL},
    [Res_F] = {"f.mkf", NULL},
    [Res_FIRE] = {"fire.mkf", NULL},
    [Res_RGM] = {"rgm.mkf", NULL},
    [Res_SSS] = {"sss.mkf", NULL},
};

GLOBALVARS *gpGlobals = NULL;

int PAL_InitGlobals(void)
/*++
  Purpose:

    Initialize global data.

  Parameters:

    None.

  Return value:

    0 = success, -1 = error.

--*/
{
   // Open files
   char filename[256];
   unsigned char i;
   for (i = 0; i < Res_ALL; i++)
   {
      sprintf(filename, RESOURCE_PATH "/%s", gFiles[i].name);
      gFiles[i].fp = UTIL_fopen(filename, "rb");
   }
   return 0;
}

void PAL_FreeGlobals(void)
/*++
  Purpose:

    Free global data.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   // Close all opened files
   
   unsigned char i;
   for (i = 0; i < Res_ALL; i++)
   {
      UTIL_fclose(gFiles[i].fp);
   }

   // Free the game data
   if(gpGlobals)
   {
      UTIL_free(gpGlobals->g.lprgEventObject);
      UTIL_free(gpGlobals->g.lprgScriptEntry);
      UTIL_free(gpGlobals->g.lprgStore);
      UTIL_free(gpGlobals->g.lprgEnemy);
      UTIL_free(gpGlobals->g.lprgEnemyTeam);
      UTIL_free(gpGlobals->g.lprgMagic);
      UTIL_free(gpGlobals->g.lprgBattleField);
      UTIL_free(gpGlobals->g.lprgLevelUpMagic);
      UTIL_free(gpGlobals->g.PlayerRoles);
      memset(gpGlobals, 0, sizeof(GLOBALVARS));
      UTIL_free(gpGlobals);
   }

   gpGlobals = NULL;
}

static void PAL_InitGlobalGameData(void)
/*++
  Purpose:

    Initialize global game data.

  Parameters:

    None.

  Return value:

    None.

--*/
{
#define PAL_DOALLOCATE(fp, num, type, ptr, n) \
   if (ptr == NULL)                           \
   {                                          \
      int len = PAL_MKFGetChunkSize(num, fp); \
      ptr = (type *)UTIL_malloc(len);         \
      n = len / sizeof(type);                 \
      PAL_MKFReadChunk(ptr, len, num, fp);    \
   }

   // If the memory has not been allocated, allocate first.
   PAL_DOALLOCATE(gFiles[Res_SSS].fp, 0, EVENTOBJECT, gpGlobals->g.lprgEventObject, gpGlobals->g.nEventObject);
   PAL_DOALLOCATE(gFiles[Res_SSS].fp, 4, SCRIPTENTRY, gpGlobals->g.lprgScriptEntry, gpGlobals->g.nScriptEntry);
   PAL_DOALLOCATE(gFiles[Res_DATA].fp, 0, STORE, gpGlobals->g.lprgStore, gpGlobals->g.nStore);
   PAL_DOALLOCATE(gFiles[Res_DATA].fp, 1, ENEMY, gpGlobals->g.lprgEnemy, gpGlobals->g.nEnemy);
   PAL_DOALLOCATE(gFiles[Res_DATA].fp, 2, ENEMYTEAM, gpGlobals->g.lprgEnemyTeam, gpGlobals->g.nEnemyTeam);
   PAL_DOALLOCATE(gFiles[Res_DATA].fp, 4, MAGIC, gpGlobals->g.lprgMagic, gpGlobals->g.nMagic);
   PAL_DOALLOCATE(gFiles[Res_DATA].fp, 5, BATTLEFIELD, gpGlobals->g.lprgBattleField, gpGlobals->g.nBattleField);
   PAL_DOALLOCATE(gFiles[Res_DATA].fp, 6, LEVELUPMAGIC_ALL, gpGlobals->g.lprgLevelUpMagic, gpGlobals->g.nLevelUpMagic);
   PAL_DOALLOCATE(gFiles[Res_DATA].fp, 3, PLAYERROLES, gpGlobals->g.PlayerRoles, gpGlobals->g.nPlayerRoles);

   PAL_MKFReadChunk(gpGlobals->g.rgwBattleEffectIndex, sizeof(gpGlobals->g.rgwBattleEffectIndex), 11, gFiles[Res_DATA].fp);
   PAL_MKFReadChunk(gpGlobals->g.EnemyPos, sizeof(gpGlobals->g.EnemyPos), 13, gFiles[Res_DATA].fp);
   PAL_MKFReadChunk(gpGlobals->g.rgLevelUpExp, sizeof(gpGlobals->g.rgLevelUpExp), 14, gFiles[Res_DATA].fp);
#undef PAL_DOALLOCATE
}

static void PAL_LoadDefaultGame(void)
/*++
  Purpose:

    Load the default game data.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   unsigned int       i;
   // Load the default data from the game data files.
   PAL_MKFReadChunk(gpGlobals->g.lprgEventObject, gpGlobals->g.nEventObject * sizeof(EVENTOBJECT), 0, gFiles[Res_SSS].fp);
   PAL_MKFReadChunk(gpGlobals->g.rgScene, sizeof(gpGlobals->g.rgScene), 1, gFiles[Res_SSS].fp);
   PAL_MKFReadChunk(gpGlobals->g.rgObject, sizeof(gpGlobals->g.rgObject), 2, gFiles[Res_SSS].fp);
   PAL_MKFReadChunk(gpGlobals->g.PlayerRoles, gpGlobals->g.nPlayerRoles * sizeof(LEVELUPMAGIC_ALL), 3, gFiles[Res_DATA].fp);

   // Set some other default data.
   gpGlobals->dwCash = 0;
   gpGlobals->wNumMusic = 0;
   gpGlobals->wNumPalette = 0;
   gpGlobals->wNumScene = 1;
   gpGlobals->wCollectValue = 0;
   gpGlobals->fNightPalette = false;
   gpGlobals->wMaxPartyMemberIndex = 0;
   gpGlobals->viewport = PAL_XY(0, 0);
   gpGlobals->wLayer = 0;
   gpGlobals->nFollower = 0;
   gpGlobals->wChaseRange = 1;

   memset(gpGlobals->rgInventory, 0, sizeof(gpGlobals->rgInventory));
   memset(gpGlobals->rgPoisonStatus, 0, sizeof(gpGlobals->rgPoisonStatus));
   memset(gpGlobals->rgParty, 0, sizeof(gpGlobals->rgParty));
   memset(gpGlobals->rgTrail, 0, sizeof(gpGlobals->rgTrail));
   memset(&(gpGlobals->Exp), 0, sizeof(gpGlobals->Exp));

   for (i = 0; i < MAX_PLAYER_ROLES; i++)
   {
      gpGlobals->Exp.rgPrimaryExp[i].wLevel = gpGlobals->g.PlayerRoles->rgwLevel[i];
      gpGlobals->Exp.rgHealthExp[i].wLevel = gpGlobals->g.PlayerRoles->rgwLevel[i];
      gpGlobals->Exp.rgMagicExp[i].wLevel = gpGlobals->g.PlayerRoles->rgwLevel[i];
      gpGlobals->Exp.rgAttackExp[i].wLevel = gpGlobals->g.PlayerRoles->rgwLevel[i];
      gpGlobals->Exp.rgMagicPowerExp[i].wLevel = gpGlobals->g.PlayerRoles->rgwLevel[i];
      gpGlobals->Exp.rgDefenseExp[i].wLevel = gpGlobals->g.PlayerRoles->rgwLevel[i];
      gpGlobals->Exp.rgDexterityExp[i].wLevel = gpGlobals->g.PlayerRoles->rgwLevel[i];
      gpGlobals->Exp.rgFleeExp[i].wLevel = gpGlobals->g.PlayerRoles->rgwLevel[i];
   }

   gpGlobals->fEnteringScene = true;
}

typedef struct tagSAVEDGAME_COMMON {
    unsigned short             wSavedTimes;             // saved times
    unsigned short             wViewportX, wViewportY;  // viewport location
    unsigned short             nPartyMember;            // number of members in party
    unsigned short             wNumScene;               // scene number
    unsigned short             wPaletteOffset;
    unsigned short             wPartyDirection;         // party direction
    unsigned short             wNumMusic;               // music number
    unsigned short             wNumBattleMusic;         // battle music number
    unsigned short             wNumBattleField;         // battle field number
    unsigned short             wScreenWave;             // level of screen waving
    unsigned short             wBattleSpeed;            // battle speed
    unsigned short             wCollectValue;           // value of "collected" items
    unsigned short             wLayer;
    unsigned short             wChaseRange;
    unsigned short             wChasespeedChangeCycles;
    unsigned short             nFollower;
    unsigned short             rgwReserved2[3];         // unused
    unsigned int            dwCash;                  // amount of cash
    PARTY            rgParty[MAX_PLAYABLE_PLAYER_ROLES];       // player party
    TRAIL            rgTrail[MAX_PLAYABLE_PLAYER_ROLES];       // player trail
    ALLEXPERIENCE    Exp;                     // experience data
    PLAYERROLES      PlayerRoles;
    POISONSTATUS     rgPoisonStatus[MAX_POISONS][MAX_PLAYABLE_PLAYER_ROLES]; // poison status
    INVENTORY        rgInventory[MAX_INVENTORY];               // inventory status
    SCENE            rgScene[MAX_SCENES];
} SAVEDGAME_COMMON;

typedef struct tagSAVEDGAME_WIN {
    unsigned short             wSavedTimes;             // saved times
    unsigned short             wViewportX, wViewportY;  // viewport location
    unsigned short             nPartyMember;            // number of members in party
    unsigned short             wNumScene;               // scene number
    unsigned short             wPaletteOffset;
    unsigned short             wPartyDirection;         // party direction
    unsigned short             wNumMusic;               // music number
    unsigned short             wNumBattleMusic;         // battle music number
    unsigned short             wNumBattleField;         // battle field number
    unsigned short             wScreenWave;             // level of screen waving
    unsigned short             wBattleSpeed;            // battle speed
    unsigned short             wCollectValue;           // value of "collected" items
    unsigned short             wLayer;
    unsigned short             wChaseRange;
    unsigned short             wChasespeedChangeCycles;
    unsigned short             nFollower;
    unsigned short             rgwReserved2[3];         // unused
    unsigned int            dwCash;                  // amount of cash
    PARTY            rgParty[MAX_PLAYABLE_PLAYER_ROLES];       // player party
    TRAIL            rgTrail[MAX_PLAYABLE_PLAYER_ROLES];       // player trail
    ALLEXPERIENCE    Exp;                     // experience data
    PLAYERROLES      PlayerRoles;
    POISONSTATUS     rgPoisonStatus[MAX_POISONS][MAX_PLAYABLE_PLAYER_ROLES]; // poison status
    INVENTORY        rgInventory[MAX_INVENTORY];               // inventory status
    SCENE            rgScene[MAX_SCENES];
    OBJECT           rgObject[MAX_OBJECTS];
    EVENTOBJECT      rgEventObject[MAX_EVENT_OBJECTS];
} SAVEDGAME_WIN;

static int PAL_LoadGame_Common(int iSaveSlot, SAVEDGAME_COMMON *s, unsigned int size) {
   // Try to open the specified file
   char *save_path = (char *)UTIL_malloc(256);
   sprintf(save_path, RESOURCE_PATH "/%d.rpg", iSaveSlot);
   void *fp = UTIL_fopen_without_checking(save_path, "rb");
   UTIL_free(save_path);

   // Read all data from the file and close.
   unsigned int n = fp ? UTIL_fread(s, 1, size, fp) : 0;

   UTIL_fclose(fp);

   if (n < size - sizeof(EVENTOBJECT) * MAX_EVENT_OBJECTS) {
      return false;
   }

   // Get common data from the saved game struct.
   gpGlobals->viewport = PAL_XY(s->wViewportX, s->wViewportY);
   gpGlobals->wMaxPartyMemberIndex = min(max(s->nPartyMember, 0), MAX_PLAYERS_IN_PARTY - 1);
   gpGlobals->wNumScene = s->wNumScene;
   gpGlobals->fNightPalette = (s->wPaletteOffset != 0);
   gpGlobals->wPartyDirection = s->wPartyDirection;
   gpGlobals->wNumMusic = s->wNumMusic;
   gpGlobals->wNumBattleMusic = s->wNumBattleMusic;
   gpGlobals->wNumBattleField = s->wNumBattleField;
   gpGlobals->wScreenWave = s->wScreenWave;
   gpGlobals->sWaveProgression = 0;
   gpGlobals->wCollectValue = s->wCollectValue;
   gpGlobals->wLayer = s->wLayer;
   gpGlobals->wChaseRange = s->wChaseRange;
   gpGlobals->wChasespeedChangeCycles = s->wChasespeedChangeCycles;
   gpGlobals->nFollower = s->nFollower;
   gpGlobals->dwCash = s->dwCash;

   memcpy(gpGlobals->rgParty, s->rgParty, sizeof(PARTY) * MAX_PLAYABLE_PLAYER_ROLES);
   memcpy(gpGlobals->rgTrail, s->rgTrail, sizeof(TRAIL) * MAX_PLAYABLE_PLAYER_ROLES);
   gpGlobals->Exp = s->Exp;
   *gpGlobals->g.PlayerRoles = s->PlayerRoles;
   memset(gpGlobals->rgPoisonStatus, 0, sizeof(POISONSTATUS) * MAX_POISONS * MAX_PLAYABLE_PLAYER_ROLES);
   memcpy(gpGlobals->rgInventory, s->rgInventory, sizeof(INVENTORY) * MAX_INVENTORY);
   memcpy(gpGlobals->g.rgScene, s->rgScene, sizeof(SCENE) * MAX_SCENES);

   gpGlobals->fEnteringScene = false;

   PAL_CompressInventory();

   return true;
}

static int PAL_LoadGame_WIN(int iSaveSlot)
/*++
  Purpose:

    Load a saved game.

  Parameters:

    [IN]  szFileName - file name of saved game.

  Return value:

    0 if success, -1 if failed.

--*/
{
   SAVEDGAME_WIN *s = (SAVEDGAME_WIN *)UTIL_malloc(sizeof(SAVEDGAME_WIN));

   //
   // Get all the data from the saved game struct.
   //
   if (!PAL_LoadGame_Common(iSaveSlot, (SAVEDGAME_COMMON *)s, sizeof(SAVEDGAME_WIN)))
       return -1;

   memcpy(gpGlobals->g.rgObject, s->rgObject, sizeof(gpGlobals->g.rgObject));
   memcpy(gpGlobals->g.lprgEventObject, s->rgEventObject, sizeof(EVENTOBJECT) * MAX_EVENT_OBJECTS);

   UTIL_free(s);

   //
   // Success
   //
   return 0;
}

static int PAL_LoadGame(int iSaveSlot)
{
    return PAL_LoadGame_WIN(iSaveSlot);
}

static void PAL_SaveGame_Common(int iSaveSlot, unsigned short wSavedTimes, SAVEDGAME_COMMON *s, unsigned int size)
{
   s->wSavedTimes = wSavedTimes;
   s->wViewportX = (unsigned short)PAL_X(gpGlobals->viewport);
   s->wViewportY = (unsigned short)PAL_Y(gpGlobals->viewport);
   s->nPartyMember = gpGlobals->wMaxPartyMemberIndex;
   s->wNumScene = gpGlobals->wNumScene;
   s->wPaletteOffset = (gpGlobals->fNightPalette ? 0x180 : 0);
   s->wPartyDirection = gpGlobals->wPartyDirection;
   s->wNumMusic = gpGlobals->wNumMusic;
   s->wNumBattleMusic = gpGlobals->wNumBattleMusic;
   s->wNumBattleField = gpGlobals->wNumBattleField;
   s->wScreenWave = gpGlobals->wScreenWave;
   s->wCollectValue = gpGlobals->wCollectValue;
   s->wLayer = gpGlobals->wLayer;
   s->wChaseRange = gpGlobals->wChaseRange;
   s->wChasespeedChangeCycles = gpGlobals->wChasespeedChangeCycles;
   s->nFollower = gpGlobals->nFollower;
   s->dwCash = gpGlobals->dwCash;
   s->wBattleSpeed = 2;

   memcpy(s->rgParty, gpGlobals->rgParty, sizeof(gpGlobals->rgParty));
   memcpy(s->rgTrail, gpGlobals->rgTrail, sizeof(gpGlobals->rgTrail));
   s->Exp = gpGlobals->Exp;
   s->PlayerRoles = *gpGlobals->g.PlayerRoles;
   memcpy(s->rgPoisonStatus, gpGlobals->rgPoisonStatus, sizeof(gpGlobals->rgPoisonStatus));
   memcpy(s->rgInventory, gpGlobals->rgInventory, sizeof(gpGlobals->rgInventory));
   memcpy(s->rgScene, gpGlobals->g.rgScene, sizeof(gpGlobals->g.rgScene));

   // Try writing to file
   void *fp = NULL;
   unsigned int i;
   char *save_path = (char *)UTIL_malloc(256);
   sprintf(save_path, RESOURCE_PATH "/%d.rpg", iSaveSlot);
   if (fp = UTIL_fopen(save_path, "wb")) {
      i = PAL_MKFGetChunkSize(0, gFiles[Res_SSS].fp);
      i += size - sizeof(EVENTOBJECT) * MAX_EVENT_OBJECTS;
      UTIL_fwrite(s, i, 1, fp);
   }
   UTIL_fclose(fp);
   UTIL_free(save_path);
}

static void
PAL_SaveGame_WIN(
    int iSaveSlot,
    unsigned short wSavedTimes)
/*++
  Purpose:

    Save the current game state to file.

  Parameters:

    [IN]  szFileName - file name of saved game.

  Return value:

    None.

--*/
{
   SAVEDGAME_WIN *s = (SAVEDGAME_WIN *)UTIL_malloc(sizeof(SAVEDGAME_WIN));

   //
   // Put all the data to the saved game struct.
   //
   memcpy(&s->rgObject, gpGlobals->g.rgObject, sizeof(gpGlobals->g.rgObject));
   memcpy(&s->rgEventObject, gpGlobals->g.lprgEventObject, sizeof(EVENTOBJECT) * MAX_EVENT_OBJECTS);

   PAL_SaveGame_Common(iSaveSlot, wSavedTimes, (SAVEDGAME_COMMON *)s, sizeof(SAVEDGAME_WIN));

   UTIL_free(s);
}

void
PAL_SaveGame(
   int            iSaveSlot,
   unsigned short           wSavedTimes
)
{
   PAL_SaveGame_WIN(iSaveSlot, wSavedTimes);
}

void PAL_ReloadInNextTick(unsigned char iSaveSlot)
/*++
  Purpose:

    Reload the game IN NEXT TICK, avoid reentrant problems.

  Parameters:

    [IN]  iSaveSlot - Slot of saved game.

  Return value:

    None.

--*/
{
   if (gpGlobals == NULL)
      gpGlobals = (GLOBALVARS *)UTIL_malloc(sizeof(GLOBALVARS));
   gpGlobals->bCurrentSaveSlot = iSaveSlot;
   PAL_SetLoadFlags(kLoadGlobalData | kLoadScene | kLoadPlayerSprite);
   gpGlobals->fEnteringScene = true;
   gpGlobals->fNeedToFadeIn = true;
   gpGlobals->dwFrameNum = 0;
}

void
PAL_InitGameData(
   int         iSaveSlot
)
/*++
  Purpose:

    Initialize the game data (used when starting a new game or loading a saved game).

  Parameters:

    [IN]  iSaveSlot - Slot of saved game.

  Return value:

    None.

--*/
{
   PAL_InitGlobalGameData();

   gpGlobals->bCurrentSaveSlot = (unsigned char)iSaveSlot;

   //
   // try loading from the saved game file.
   //
   if (iSaveSlot == 0 || PAL_LoadGame(iSaveSlot) != 0)
   {
      //
      // Cannot load the saved game file. Load the defaults.
      //
      PAL_LoadDefaultGame();
   }

   gpGlobals->iCurInvMenuItem = 0;
   gpGlobals->fInBattle = false;

   memset(gpGlobals->rgPlayerStatus, 0, sizeof(gpGlobals->rgPlayerStatus));

   PAL_UpdateEquipments();
}

int
PAL_CountItem(
   unsigned short          wObjectID
)
/*++
 Purpose:

 Count the specified kind of item in the inventory AND in players' equipments.

 Parameters:

 [IN]  wObjectID - object number of the item.

 Return value:

 Counted value.

 --*/
{
    int          index;
    int          count;
    int          i,j,w;

    if (wObjectID == 0)
    {
        return false;
    }

    index = 0;
    count = 0;

    //
    // Search for the specified item in the inventory
    //
    while (index < MAX_INVENTORY)
    {
        if (gpGlobals->rgInventory[index].wItem == wObjectID)
        {
            count = gpGlobals->rgInventory[index].nAmount;
            break;
        }
        else if (gpGlobals->rgInventory[index].wItem == 0)
        {
            break;
        }
        index++;
    }

    for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
    {
        w = gpGlobals->rgParty[i].wPlayerRole;

        for (j = 0; j < MAX_PLAYER_EQUIPMENTS; j++)
        {
            if (gpGlobals->g.PlayerRoles->rgwEquipment[j][w] == wObjectID)
            {
                count++;
            }
        }
    }
    return count;
}

int
PAL_GetItemIndexToInventory(
   unsigned short          wObjectID,
   int          *index
)
/*++
  Purpose:

    Search for the specified item in the inventory.

  Parameters:

    [IN]  wObjectID - object number of the item.

    [IN]  index - use a pointer to receive the retrieved inventory index.

  Return value:

    true if found it, false if not found it.

--*/
{
   int         fFound = false;

   *index = 0;

   while (*index < MAX_INVENTORY)
   {
      if (gpGlobals->rgInventory[*index].wItem == wObjectID)
      {
         fFound = true;
         break;
      }
      else if (gpGlobals->rgInventory[*index].wItem == 0)
      {
         break;
      }
      (*index)++;
   }

   return fFound;
}

int
PAL_AddItemToInventory(
   unsigned short          wObjectID,
   int           iNum
)
/*++
  Purpose:

    Add or remove the specified kind of item in the inventory.

  Parameters:

    [IN]  wObjectID - object number of the item.

    [IN]  iNum - number to be added (positive value) or removed (negative value).

  Return value:

    true if succeeded, false if failed.

--*/
{
   int          index;
   int         fFound;

   if (wObjectID == 0)
   {
      return false;
   }

   if (iNum == 0)
   {
      iNum = 1;
   }

   index = 0;
   fFound = false;

   //
   // Search for the specified item in the inventory
   //
   fFound = PAL_GetItemIndexToInventory(wObjectID, &index);

   if (iNum > 0)
   {
      //
      // Add item
      //
      if (index >= MAX_INVENTORY)
      {
         //
         // inventory is full. cannot add item
         //
         return false;
      }

      if (fFound)
      {
         gpGlobals->rgInventory[index].nAmount += iNum;
         if (gpGlobals->rgInventory[index].nAmount > 99)
         {
            //
            // Maximum number is 99
            //
            gpGlobals->rgInventory[index].nAmount = 99;
         }
      }
      else
      {
         gpGlobals->rgInventory[index].wItem = wObjectID;
         if (iNum > 99)
         {
            iNum = 99;
         }
         gpGlobals->rgInventory[index].nAmount = iNum;
      }

      return true;
   }
   else
   {
      //
      // Remove item
      //
      if (fFound)
      {
         iNum *= -1;
         if (gpGlobals->rgInventory[index].nAmount < iNum)
         {
            //
            // This item has been run out
            //
            gpGlobals->rgInventory[index].nAmount = 0;
            return false;
         }

         gpGlobals->rgInventory[index].nAmount -= iNum;
         //
         /// Need process last item
         //
         if (gpGlobals->rgInventory[index].nAmount == 0 &&
             index == gpGlobals->iCurInvMenuItem &&
             index + 1 < MAX_INVENTORY &&
             gpGlobals->rgInventory[index + 1].nAmount <= 0 &&
             gpGlobals->iCurInvMenuItem > 0)
            gpGlobals->iCurInvMenuItem--;
         return true;
      }

      return false;
   }
}

int
PAL_GetItemAmount(
   unsigned short        wItem
)
/*++
  Purpose:

    Get the amount of the specified item in the inventory.

  Parameters:

    [IN]  wItem - the object ID of the item.

  Return value:

    The amount of the item in the inventory.

--*/
{
   int i;

   for (i = 0; i < MAX_INVENTORY; i++)
   {
      if (gpGlobals->rgInventory[i].wItem == 0)
      {
         break;
      }

      if (gpGlobals->rgInventory[i].wItem == wItem)
      {
         return gpGlobals->rgInventory[i].nAmount;
      }
   }

   return 0;
}

void
PAL_CompressInventory(
   void
)
/*++
  Purpose:

    Remove all the items in inventory which has a number of zero.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   int i, j;

   j = 0;

   for (i = 0; i < MAX_INVENTORY; i++)
   {
      //removed detect zero then break code, due to incompatible with save file hacked by palmod

      if (gpGlobals->rgInventory[i].nAmount > 0)
      {
         gpGlobals->rgInventory[j] = gpGlobals->rgInventory[i];
         j++;
      }
   }

   for (; j < MAX_INVENTORY; j++)
   {
      gpGlobals->rgInventory[j].nAmount = 0;
      gpGlobals->rgInventory[j].nAmountInUse = 0;
      gpGlobals->rgInventory[j].wItem = 0;
   }
}

int
PAL_IncreaseHPMP(
   unsigned short          wPlayerRole,
   short         sHP,
   short         sMP
)
/*++
  Purpose:

    Increase or decrease player's HP and/or MP.

  Parameters:

    [IN]  wPlayerRole - the number of player role.

    [IN]  sHP - number of HP to be increased (positive value) or decrased
                (negative value).

    [IN]  sMP - number of MP to be increased (positive value) or decrased
                (negative value).

  Return value:

    true if the operation is succeeded, false if not.

--*/
{
   int           fSuccess = false;
   unsigned short           wOrigHP = gpGlobals->g.PlayerRoles->rgwHP[wPlayerRole];
   unsigned short           wOrigMP = gpGlobals->g.PlayerRoles->rgwMP[wPlayerRole];

   //
   // Only care about alive players
   //
   if (gpGlobals->g.PlayerRoles->rgwHP[wPlayerRole] > 0)
   {
      //
      // change HP
      //
      gpGlobals->g.PlayerRoles->rgwHP[wPlayerRole] += sHP;

      if ((short)(gpGlobals->g.PlayerRoles->rgwHP[wPlayerRole]) < 0)
      {
         gpGlobals->g.PlayerRoles->rgwHP[wPlayerRole] = 0;
      }
      else if (gpGlobals->g.PlayerRoles->rgwHP[wPlayerRole] >
         gpGlobals->g.PlayerRoles->rgwMaxHP[wPlayerRole])
      {
         gpGlobals->g.PlayerRoles->rgwHP[wPlayerRole] =
            gpGlobals->g.PlayerRoles->rgwMaxHP[wPlayerRole];
      }

      //
      // Change MP
      //
      gpGlobals->g.PlayerRoles->rgwMP[wPlayerRole] += sMP;

      if ((short)(gpGlobals->g.PlayerRoles->rgwMP[wPlayerRole]) < 0)
      {
         gpGlobals->g.PlayerRoles->rgwMP[wPlayerRole] = 0;
      }
      else if (gpGlobals->g.PlayerRoles->rgwMP[wPlayerRole] >
         gpGlobals->g.PlayerRoles->rgwMaxMP[wPlayerRole])
      {
         gpGlobals->g.PlayerRoles->rgwMP[wPlayerRole] =
            gpGlobals->g.PlayerRoles->rgwMaxMP[wPlayerRole];
      }

      //
      // Avoid over treatment
      //
      if (wOrigHP != gpGlobals->g.PlayerRoles->rgwHP[wPlayerRole] ||
          wOrigMP != gpGlobals->g.PlayerRoles->rgwMP[wPlayerRole])
         fSuccess = true;
   }

   return fSuccess;
}

void
PAL_UpdateEquipments(
   void
)
/*++
  Purpose:

    Update the effects of all equipped items for all players.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   int      i, j;
   unsigned short     w;

   memset(gpGlobals->rgEquipmentEffect, 0, sizeof(PLAYERROLES)*(MAX_PLAYER_EQUIPMENTS + 1));

   for (i = 0; i < MAX_PLAYER_ROLES; i++)
   {
      for (j = 0; j < MAX_PLAYER_EQUIPMENTS; j++)
      {
         w = gpGlobals->g.PlayerRoles->rgwEquipment[j][i];

         if (w != 0)
         {
            gpGlobals->g.rgObject[w].item.wScriptOnEquip =
               PAL_RunTriggerScript(gpGlobals->g.rgObject[w].item.wScriptOnEquip, (unsigned short)i);
         }
      }
   }
}

void
PAL_RemoveEquipmentEffect(
   unsigned short         wPlayerRole,
   unsigned short         wEquipPart
)
/*++
  Purpose:

    Remove all the effects of the equipment for the player.

  Parameters:

    [IN]  wPlayerRole - the player role.

    [IN]  wEquipPart - the part of the equipment.

  Return value:

    None.

--*/
{
   unsigned short *p;
   int i, j;

   p = (unsigned short *)&gpGlobals->rgEquipmentEffect[wEquipPart]; // HACKHACK

   for (i = 0; i < sizeof(PLAYERROLES) / (sizeof(unsigned short) * MAX_PLAYER_ROLES); i++)
   {
      p[i * MAX_PLAYER_ROLES + wPlayerRole] = 0;
   }

   //
   // Reset some parameters to default when appropriate
   //
   if (wEquipPart == kBodyPartHand)
   {
      //
      // reset the dual attack status
      //
      gpGlobals->rgPlayerStatus[wPlayerRole][kStatusDualAttack] = 0;
   }
   else if (wEquipPart == kBodyPartWear)
   {
      //
      // Remove all poisons leveled 99
      //
      for (i = 0; i <= (short)gpGlobals->wMaxPartyMemberIndex; i++)
      {
         if (gpGlobals->rgParty[i].wPlayerRole == wPlayerRole)
         {
            wPlayerRole = i;
            break;
         }
      }

      if (i <= (short)gpGlobals->wMaxPartyMemberIndex)
      {
         j = 0;

         for (i = 0; i < MAX_POISONS; i++)
         {
            unsigned short w = gpGlobals->rgPoisonStatus[i][wPlayerRole].wPoisonID;

            if (w == 0)
            {
               break;
            }

            if (gpGlobals->g.rgObject[w].poison.wPoisonLevel < 99)
            {
               gpGlobals->rgPoisonStatus[j][wPlayerRole] =
                  gpGlobals->rgPoisonStatus[i][wPlayerRole];
               j++;
            }
         }

         while (j < MAX_POISONS)
         {
            gpGlobals->rgPoisonStatus[j][wPlayerRole].wPoisonID = 0;
            gpGlobals->rgPoisonStatus[j][wPlayerRole].wPoisonScript = 0;
            j++;
         }
      }
   }
}

void
PAL_AddPoisonForPlayer(
   unsigned short           wPlayerRole,
   unsigned short           wPoisonID
)
/*++
  Purpose:

    Add the specified poison to the player.

  Parameters:

    [IN]  wPlayerRole - the player role ID.

    [IN]  wPoisonID - the poison to be added.

  Return value:

    None.

--*/
{
   int         i, index;
   unsigned short        w;

   for (index = 0; index <= gpGlobals->wMaxPartyMemberIndex; index++)
   {
      if (gpGlobals->rgParty[index].wPlayerRole == wPlayerRole)
      {
         break;
      }
   }

   if (index > gpGlobals->wMaxPartyMemberIndex)
   {
      return; // don't go further
   }

   for (i = 0; i < MAX_POISONS; i++)
   {
      w = gpGlobals->rgPoisonStatus[i][index].wPoisonID;

      if (w == 0)
      {
         break;
      }

      if (w == wPoisonID)
      {
         return; // already poisoned
      }
   }

   if (i < MAX_POISONS)
   {
      gpGlobals->rgPoisonStatus[i][index].wPoisonID = wPoisonID;
      gpGlobals->rgPoisonStatus[i][index].wPoisonScript =
          PAL_RunTriggerScript(gpGlobals->g.rgObject[wPoisonID].poison.wPlayerScript, wPlayerRole);
   }
}

void
PAL_CurePoisonByKind(
   unsigned short           wPlayerRole,
   unsigned short           wPoisonID
)
/*++
  Purpose:

    Remove the specified poison from the player.

  Parameters:

    [IN]  wPlayerRole - the player role ID.

    [IN]  wPoisonID - the poison to be removed.

  Return value:

    None.

--*/
{
   int i, index;

   for (index = 0; index <= gpGlobals->wMaxPartyMemberIndex; index++)
   {
      if (gpGlobals->rgParty[index].wPlayerRole == wPlayerRole)
      {
         break;
      }
   }

   if (index > gpGlobals->wMaxPartyMemberIndex)
   {
      return; // don't go further
   }

   for (i = 0; i < MAX_POISONS; i++)
   {
      if (gpGlobals->rgPoisonStatus[i][index].wPoisonID == wPoisonID)
      {
         gpGlobals->rgPoisonStatus[i][index].wPoisonID = 0;
         gpGlobals->rgPoisonStatus[i][index].wPoisonScript = 0;
      }
   }
}

void
PAL_CurePoisonByLevel(
   unsigned short           wPlayerRole,
   unsigned short           wMaxLevel
)
/*++
  Purpose:

    Remove the poisons which have a maximum level of wMaxLevel from the player.

  Parameters:

    [IN]  wPlayerRole - the player role ID.

    [IN]  wMaxLevel - the maximum level of poisons to be removed.

  Return value:

    None.

--*/
{
   int        i, index;
   unsigned short       w;

   for (index = 0; index <= gpGlobals->wMaxPartyMemberIndex; index++)
   {
      if (gpGlobals->rgParty[index].wPlayerRole == wPlayerRole)
      {
         break;
      }
   }

   if (index > gpGlobals->wMaxPartyMemberIndex)
   {
      return; // don't go further
   }

   for (i = 0; i < MAX_POISONS; i++)
   {
      w = gpGlobals->rgPoisonStatus[i][index].wPoisonID;

      if (gpGlobals->g.rgObject[w].poison.wPoisonLevel <= wMaxLevel)
      {
         gpGlobals->rgPoisonStatus[i][index].wPoisonID = 0;
         gpGlobals->rgPoisonStatus[i][index].wPoisonScript = 0;
      }
   }
}

int
PAL_IsPlayerPoisonedByLevel(
   unsigned short           wPlayerRole,
   unsigned short           wMinLevel
)
/*++
  Purpose:

    Check if the player is poisoned by poisons at a minimum level of wMinLevel.

  Parameters:

    [IN]  wPlayerRole - the player role ID.

    [IN]  wMinLevel - the minimum level of poison.

  Return value:

    true if the player is poisoned by poisons at a minimum level of wMinLevel;
    false if not.

--*/
{
   int         i, index;
   unsigned short        w;

   for (index = 0; index <= gpGlobals->wMaxPartyMemberIndex; index++)
   {
      if (gpGlobals->rgParty[index].wPlayerRole == wPlayerRole)
      {
         break;
      }
   }

   if (index > gpGlobals->wMaxPartyMemberIndex)
   {
      return false; // don't go further
   }

   for (i = 0; i < MAX_POISONS; i++)
   {
      w = gpGlobals->rgPoisonStatus[i][index].wPoisonID;

      if (w == 0)
      {
         //
         // Skip empty PoisonID
         //
         continue;
      }

      w = gpGlobals->g.rgObject[w].poison.wPoisonLevel;

      if (w >= 99)
      {
         //
         // Ignore poisons which has a level of 99 (usually effect of equipment)
         //
         continue;
      }

      if (w >= wMinLevel)
      {
         return true;
      }
   }

   return false;
}

int
PAL_IsPlayerPoisonedByKind(
   unsigned short           wPlayerRole,
   unsigned short           wPoisonID
)
/*++
  Purpose:

    Check if the player is poisoned by the specified poison.

  Parameters:

    [IN]  wPlayerRole - the player role ID.

    [IN]  wPoisonID - the poison to be checked.

  Return value:

    true if player is poisoned by the specified poison;
    false if not.

--*/
{
   int i, index;

   for (index = 0; index <= gpGlobals->wMaxPartyMemberIndex; index++)
   {
      if (gpGlobals->rgParty[index].wPlayerRole == wPlayerRole)
      {
         break;
      }
   }

   if (index > gpGlobals->wMaxPartyMemberIndex)
   {
      return false; // don't go further
   }

   for (i = 0; i < MAX_POISONS; i++)
   {
      if (gpGlobals->rgPoisonStatus[i][index].wPoisonID == wPoisonID)
      {
         return true;
      }
   }

   return false;
}

unsigned short
PAL_GetPlayerAttackStrength(
   unsigned short           wPlayerRole
)
/*++
  Purpose:

    Get the player's attack strength, count in the effect of equipments.

  Parameters:

    [IN]  wPlayerRole - the player role ID.

  Return value:

    The total attack strength of the player.

--*/
{
   unsigned short       w;
   int        i;

   w = gpGlobals->g.PlayerRoles->rgwAttackStrength[wPlayerRole];

   for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
   {
      w += gpGlobals->rgEquipmentEffect[i].rgwAttackStrength[wPlayerRole];
   }

   return w;
}

unsigned short
PAL_GetPlayerMagicStrength(
   unsigned short           wPlayerRole
)
/*++
  Purpose:

    Get the player's magic strength, count in the effect of equipments.

  Parameters:

    [IN]  wPlayerRole - the player role ID.

  Return value:

    The total magic strength of the player.

--*/
{
   unsigned short       w;
   int        i;

   w = gpGlobals->g.PlayerRoles->rgwMagicStrength[wPlayerRole];

   for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
   {
      w += gpGlobals->rgEquipmentEffect[i].rgwMagicStrength[wPlayerRole];
   }

   return w;
}

unsigned short
PAL_GetPlayerDefense(
   unsigned short           wPlayerRole
)
/*++
  Purpose:

    Get the player's defense value, count in the effect of equipments.

  Parameters:

    [IN]  wPlayerRole - the player role ID.

  Return value:

    The total defense value of the player.

--*/
{
   unsigned short       w;
   int        i;

   w = gpGlobals->g.PlayerRoles->rgwDefense[wPlayerRole];

   for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
   {
      w += gpGlobals->rgEquipmentEffect[i].rgwDefense[wPlayerRole];
   }

   return w;
}

unsigned short
PAL_GetPlayerDexterity(
   unsigned short           wPlayerRole
)
/*++
  Purpose:

    Get the player's dexterity, count in the effect of equipments.

  Parameters:

    [IN]  wPlayerRole - the player role ID.

  Return value:

    The total dexterity of the player.

--*/
{
   unsigned short       w;
   int        i;

   w = gpGlobals->g.PlayerRoles->rgwDexterity[wPlayerRole];

   for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
   {
      w += gpGlobals->rgEquipmentEffect[i].rgwDexterity[wPlayerRole];
   }

   return w;
}

unsigned short
PAL_GetPlayerFleeRate(
   unsigned short           wPlayerRole
)
/*++
  Purpose:

    Get the player's flee rate, count in the effect of equipments.

  Parameters:

    [IN]  wPlayerRole - the player role ID.

  Return value:

    The total flee rate of the player.

--*/
{
   unsigned short       w;
   int        i;

   w = gpGlobals->g.PlayerRoles->rgwFleeRate[wPlayerRole];

   for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
   {
      w += gpGlobals->rgEquipmentEffect[i].rgwFleeRate[wPlayerRole];
   }

   return w;
}

unsigned short
PAL_GetPlayerPoisonResistance(
   unsigned short           wPlayerRole
)
/*++
  Purpose:

    Get the player's resistance to poisons, count in the effect of equipments.

  Parameters:

    [IN]  wPlayerRole - the player role ID.

  Return value:

    The total resistance to poisons of the player.

--*/
{
   unsigned short       w;
   int        i;

   w = gpGlobals->g.PlayerRoles->rgwPoisonResistance[wPlayerRole];

   for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
   {
      w += gpGlobals->rgEquipmentEffect[i].rgwPoisonResistance[wPlayerRole];
   }

   if (w > 100)
   {
      w = 100;
   }

   return w;
}

unsigned short
PAL_GetPlayerElementalResistance(
   unsigned short           wPlayerRole,
   int            iAttrib
)
/*++
  Purpose:

    Get the player's resistance to attributed magics, count in the effect
    of equipments.

  Parameters:

    [IN]  wPlayerRole - the player role ID.

    [IN]  iAttrib - the attribute of magics.

  Return value:

    The total resistance to the attributed magics of the player.

--*/
{
   unsigned short       w;
   int        i;

   w = gpGlobals->g.PlayerRoles->rgwElementalResistance[iAttrib][wPlayerRole];

   for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
   {
      w += gpGlobals->rgEquipmentEffect[i].rgwElementalResistance[iAttrib][wPlayerRole];
   }

   if (w > 100)
   {
      w = 100;
   }

   return w;
}

unsigned short
PAL_GetPlayerBattleSprite(
   unsigned short             wPlayerRole
)
/*++
  Purpose:

    Get player's battle sprite.

  Parameters:

    [IN]  wPlayerRole - the player role ID.

  Return value:

    Number of the player's battle sprite.

--*/
{
   int       i;
   unsigned short      w;

   w = gpGlobals->g.PlayerRoles->rgwSpriteNumInBattle[wPlayerRole];

   for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
   {
      if (gpGlobals->rgEquipmentEffect[i].rgwSpriteNumInBattle[wPlayerRole] != 0)
      {
         w = gpGlobals->rgEquipmentEffect[i].rgwSpriteNumInBattle[wPlayerRole];
      }
   }

   return w;
}

unsigned short
PAL_GetPlayerCooperativeMagic(
   unsigned short             wPlayerRole
)
/*++
  Purpose:

    Get player's cooperative magic.

  Parameters:

    [IN]  wPlayerRole - the player role ID.

  Return value:

    Object ID of the player's cooperative magic.

--*/
{
   int       i;
   unsigned short      w;

   w = gpGlobals->g.PlayerRoles->rgwCooperativeMagic[wPlayerRole];

   for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
   {
      if (gpGlobals->rgEquipmentEffect[i].rgwCooperativeMagic[wPlayerRole] != 0)
      {
         w = gpGlobals->rgEquipmentEffect[i].rgwCooperativeMagic[wPlayerRole];
      }
   }

   return w;
}

int
PAL_PlayerCanAttackAll(
   unsigned short        wPlayerRole
)
/*++
  Purpose:

    Check if the player can attack all of the enemies in one move.

  Parameters:

    [IN]  wPlayerRole - the player role ID.

  Return value:

    true if player can attack all of the enemies in one move, false if not.

--*/
{
   int       i;
   int      f;

   f = false;

   for (i = 0; i <= MAX_PLAYER_EQUIPMENTS; i++)
   {
      if (gpGlobals->rgEquipmentEffect[i].rgwAttackAll[wPlayerRole] != 0)
      {
         f = true;
         break;
      }
   }

   return f;
}

int
PAL_AddMagic(
   unsigned short           wPlayerRole,
   unsigned short           wMagic
)
/*++
  Purpose:

    Add a magic to the player.

  Parameters:

    [IN]  wPlayerRole - the player role ID.

    [IN]  wMagic - the object ID of the magic.

  Return value:

    true if succeeded, false if failed.

--*/
{
   int            i;

   for (i = 0; i < MAX_PLAYER_MAGICS; i++)
   {
      if (gpGlobals->g.PlayerRoles->rgwMagic[i][wPlayerRole] == wMagic)
      {
         //
         // already have this magic
         //
         return false;
      }
   }

   for (i = 0; i < MAX_PLAYER_MAGICS; i++)
   {
      if (gpGlobals->g.PlayerRoles->rgwMagic[i][wPlayerRole] == 0)
      {
         break;
      }
   }

   if (i >= MAX_PLAYER_MAGICS)
   {
      //
      // Not enough slots
      //
      return false;
   }

   gpGlobals->g.PlayerRoles->rgwMagic[i][wPlayerRole] = wMagic;
   return true;
}

void
PAL_RemoveMagic(
   unsigned short           wPlayerRole,
   unsigned short           wMagic
)
/*++
  Purpose:

    Remove a magic to the player.

  Parameters:

    [IN]  wPlayerRole - the player role ID.

    [IN]  wMagic - the object ID of the magic.

  Return value:

    None.

--*/
{
   int            i;

   for (i = 0; i < MAX_PLAYER_MAGICS; i++)
   {
      if (gpGlobals->g.PlayerRoles->rgwMagic[i][wPlayerRole] == wMagic)
      {
         gpGlobals->g.PlayerRoles->rgwMagic[i][wPlayerRole] = 0;
         break;
      }
   }
}

int
PAL_SetPlayerStatus(
   unsigned short         wPlayerRole,
   unsigned short         wStatusID,
   unsigned short         wNumRound
)
/*++
  Purpose:

    Set one of the statuses for the player.

  Parameters:

    [IN]  wPlayerRole - the player ID.

    [IN]  wStatusID - the status to be set.

    [IN]  wNumRound - the effective rounds of the status.

  Return value:

    None.

--*/
{
   int           fSuccess = true;

   switch (wStatusID)
   {
   case kStatusConfused:
   case kStatusSleep:
   case kStatusSilence:
   case kStatusParalyzed:
      //
      // for "bad" statuses, don't set the status when we already have it
      //
      if (gpGlobals->rgPlayerStatus[wPlayerRole][wStatusID] == 0)
      {
         gpGlobals->rgPlayerStatus[wPlayerRole][wStatusID] = wNumRound;
      }
      break;

   case kStatusPuppet:
      //
      // only allow dead players for "puppet" status
      //
      if (gpGlobals->g.PlayerRoles->rgwHP[wPlayerRole] == 0)
      {
         if (gpGlobals->rgPlayerStatus[wPlayerRole][wStatusID] < wNumRound)
         {
            gpGlobals->rgPlayerStatus[wPlayerRole][wStatusID] = wNumRound;
         }
      }
      else
      {
         fSuccess = false;
      }
      break;

   case kStatusBravery:
   case kStatusProtect:
   case kStatusDualAttack:
   case kStatusHaste:
      //
      // for "good" statuses, reset the status if the status to be set lasts longer
      //
      if (gpGlobals->g.PlayerRoles->rgwHP[wPlayerRole] != 0 &&
         gpGlobals->rgPlayerStatus[wPlayerRole][wStatusID] < wNumRound)
      {
         gpGlobals->rgPlayerStatus[wPlayerRole][wStatusID] = wNumRound;
      }
      break;

   default:
      assert(false);
      break;
   }

   return fSuccess;
}

void
PAL_RemovePlayerStatus(
   unsigned short         wPlayerRole,
   unsigned short         wStatusID
)
/*++
  Purpose:

    Remove one of the status for player.

  Parameters:

    [IN]  wPlayerRole - the player ID.

    [IN]  wStatusID - the status to be set.

  Return value:

    None.

--*/
{
   //
   // Don't remove effects of equipments
   //
   if (gpGlobals->rgPlayerStatus[wPlayerRole][wStatusID] <= 999)
   {
      gpGlobals->rgPlayerStatus[wPlayerRole][wStatusID] = 0;
   }
}

void
PAL_ClearAllPlayerStatus(
   void
)
/*++
  Purpose:

    Clear all player status.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   int      i, j;

   for (i = 0; i < MAX_PLAYER_ROLES; i++)
   {
      for (j = 0; j < kStatusAll; j++)
      {
         //
         // Don't remove effects of equipments
         //
         if (gpGlobals->rgPlayerStatus[i][j] <= 999)
         {
            gpGlobals->rgPlayerStatus[i][j] = 0;
         }
      }
   }
}

void
PAL_PlayerLevelUp(
   unsigned short          wPlayerRole,
   unsigned short          wNumLevel
)
/*++
  Purpose:

    Increase the player's level by wLevels.

  Parameters:

    [IN]  wPlayerRole - player role ID.

    [IN]  wNumLevel - number of levels to be increased.

  Return value:

    None.

--*/
{
   unsigned short          i;

   //
   // Add the level
   //
   gpGlobals->g.PlayerRoles->rgwLevel[wPlayerRole] += wNumLevel;
   if (gpGlobals->g.PlayerRoles->rgwLevel[wPlayerRole] > MAX_LEVELS)
   {
      gpGlobals->g.PlayerRoles->rgwLevel[wPlayerRole] = MAX_LEVELS;
   }

   for (i = 0; i < wNumLevel; i++)
   {
      //
      // Increase player's stats
      //
      gpGlobals->g.PlayerRoles->rgwMaxHP[wPlayerRole] += 10 + RandomLong(0, 7);
      gpGlobals->g.PlayerRoles->rgwMaxMP[wPlayerRole] += 8 + RandomLong(0, 5);
      gpGlobals->g.PlayerRoles->rgwAttackStrength[wPlayerRole] += 4 + RandomLong(0, 1);
      gpGlobals->g.PlayerRoles->rgwMagicStrength[wPlayerRole] += 4 + RandomLong(0, 1);
      gpGlobals->g.PlayerRoles->rgwDefense[wPlayerRole] += 2 + RandomLong(0, 1);
      gpGlobals->g.PlayerRoles->rgwDexterity[wPlayerRole] += 2 + RandomLong(0, 1);
      gpGlobals->g.PlayerRoles->rgwFleeRate[wPlayerRole] += 2;
   }

#define STAT_LIMIT(t) { if ((t) > 999) (t) = 999; }
   STAT_LIMIT(gpGlobals->g.PlayerRoles->rgwMaxHP[wPlayerRole]);
   STAT_LIMIT(gpGlobals->g.PlayerRoles->rgwMaxMP[wPlayerRole]);
   STAT_LIMIT(gpGlobals->g.PlayerRoles->rgwAttackStrength[wPlayerRole]);
   STAT_LIMIT(gpGlobals->g.PlayerRoles->rgwMagicStrength[wPlayerRole]);
   STAT_LIMIT(gpGlobals->g.PlayerRoles->rgwDefense[wPlayerRole]);
   STAT_LIMIT(gpGlobals->g.PlayerRoles->rgwDexterity[wPlayerRole]);
   STAT_LIMIT(gpGlobals->g.PlayerRoles->rgwFleeRate[wPlayerRole]);
#undef STAT_LIMIT

   //
   // Reset experience points to zero
   //
   gpGlobals->Exp.rgPrimaryExp[wPlayerRole].wExp = 0;
   gpGlobals->Exp.rgPrimaryExp[wPlayerRole].wLevel = gpGlobals->g.PlayerRoles->rgwLevel[wPlayerRole];
}
