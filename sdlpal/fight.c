/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
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

#include "fight.h"
#include "audio.h"
#include "battle.h"
#include "global.h"
#include "input.h"
#include "palcfg.h"
#include "palcommon.h"
#include "script.h"
#include "text.h"
#include "util.h"
#include "video.h"
#include "common.h"
#include <SDL_timer.h>

//#define INVINCIBLE 1
extern unsigned short g_rgPlayerPos[3][3][2];

int
PAL_IsPlayerDying(
   unsigned short        wPlayerRole
)
/*++
  Purpose:

    Check if the player is dying.

  Parameters:

    [IN]  wPlayerRole - the player role ID.

  Return value:

    TRUE if the player is dying, FALSE if not.

--*/
{
   return gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] <
      min(100, gpGlobals->g.PlayerRoles.rgwMaxHP[wPlayerRole] / 5);
}

int
PAL_IsPlayerHealthy(
   unsigned short     wPlayerRole
)
/*++
 Purpose:

 Check if the player is healthy.

 Parameters:

 [IN]  wPlayerRole - the player role ID.

 Return value:

 TRUE if the player is healthy, FALSE if not.

 --*/
{
   return !PAL_IsPlayerDying(wPlayerRole) &&
           gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSleep] == 0 &&
           gpGlobals->rgPlayerStatus[wPlayerRole][kStatusConfused] == 0 &&
           gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSilence] == 0 &&
           gpGlobals->rgPlayerStatus[wPlayerRole][kStatusParalyzed] == 0 &&
           gpGlobals->rgPlayerStatus[wPlayerRole][kStatusPuppet] == 0;
}

int
PAL_BattleSelectAutoTarget(
   void
)
{
   return PAL_BattleSelectAutoTargetFrom(0);
}

int
PAL_BattleSelectAutoTargetFrom(
   int begin
)
/*++
  Purpose:

    Pick an enemy target automatically.

  Parameters:

    [IN]  begin - the beginning target ID.

  Return value:

    The index of enemy. -1 if failed.

--*/
{
   int          i;
   int          count;

   i = g_Battle.UI.iPrevEnemyTarget;

   if (i >= 0 && i <= g_Battle.wMaxEnemyIndex &&
      g_Battle.rgEnemy[i].wObjectID != 0 &&
      g_Battle.rgEnemy[i].e.wHealth > 0)
   {
      return i;
   }

   for (count = 0, i = (begin >=0 ? begin : 0); count < MAX_ENEMIES_IN_TEAM; count++)
   {
      if (g_Battle.rgEnemy[i].wObjectID != 0 &&
         g_Battle.rgEnemy[i].e.wHealth > 0)
      {
         return i;
      }
      i = ( i + 1 ) % (g_Battle.wMaxEnemyIndex + 1);
   }

   return -1;
}

static short
PAL_CalcBaseDamage(
   unsigned short        wAttackStrength,
   unsigned short        wDefense
)
/*++
  Purpose:

    Calculate the base damage value of attacking.

  Parameters:

    [IN]  wAttackStrength - attack strength of attacker.

    [IN]  wDefense - defense value of inflictor.

  Return value:

    The base damage value of the attacking.

--*/
{
   short            sDamage;

   //
   // Formula courtesy of palxex and shenyanduxing
   //
   if (wAttackStrength > wDefense)
   {
      sDamage = (short)(wAttackStrength * 2 - wDefense * 1.6 + 0.5);
   }
   else if (wAttackStrength > wDefense * 0.6)
   {
      sDamage = (short)(wAttackStrength - wDefense * 0.6 + 0.5);
   }
   else
   {
      sDamage = 0;
   }

   return sDamage;
}

static short
PAL_CalcMagicDamage(
   unsigned short             wMagicStrength,
   unsigned short             wDefense,
   const unsigned short       rgwElementalResistance[NUM_MAGIC_ELEMENTAL],
   unsigned short             wPoisonResistance,
   unsigned short             wResistanceMultiplier,
   unsigned short             wMagicID
)
/*++
   Purpose:

     Calculate the damage of magic.

   Parameters:

     [IN]  wMagicStrength - magic strength of attacker.

     [IN]  wDefense - defense value of victim.

     [IN]  rgwElementalResistance - victim's resistance to the elemental magics.

     [IN]  wPoisonResistance - victim's resistance to poison.

     [IN]  wResistanceMultiplier - multiplier of resistance value.

     [IN]  wMagicID - object ID of the magic.

   Return value:

     The damage value of the magic attack.

--*/
{
   short           sDamage;
   unsigned short            wElem;

   wMagicID = gpGlobals->g.rgObject[wMagicID].magic.wMagicNumber;

   //
   // Formula courtesy of palxex and shenyanduxing
   //
   wMagicStrength = (unsigned short)(((float)wMagicStrength * RandomFloat(10, 11)) / 10.0f);

   sDamage = PAL_CalcBaseDamage(wMagicStrength, wDefense);
   sDamage /= 4;

   sDamage += gpGlobals->g.lprgMagic[wMagicID].wBaseDamage;

   if (gpGlobals->g.lprgMagic[wMagicID].wElemental != 0)
   {
      wElem = gpGlobals->g.lprgMagic[wMagicID].wElemental;

      if (wElem > NUM_MAGIC_ELEMENTAL)
      {
         sDamage = (short)((float)sDamage * (10.0f - ((float)wPoisonResistance / (float)wResistanceMultiplier)));
      }
      else if (wElem == 0)
      {
         sDamage *= 5;
      }
      else
      {
         sDamage = (short)((float)sDamage * (10.0f - ((float)rgwElementalResistance[wElem - 1] / (float)wResistanceMultiplier)));
      }

      sDamage /= 5;

      if (wElem <= NUM_MAGIC_ELEMENTAL)
      {
         sDamage *= 10 + gpGlobals->g.lprgBattleField[gpGlobals->wNumBattleField].rgsMagicEffect[wElem - 1];
         sDamage /= 10;
      }
   }

   return sDamage;
}

short
PAL_CalcPhysicalAttackDamage(
   unsigned short           wAttackStrength,
   unsigned short           wDefense,
   unsigned short           wAttackResistance
)
/*++
  Purpose:

    Calculate the damage value of physical attacking.

  Parameters:

    [IN]  wAttackStrength - attack strength of attacker.

    [IN]  wDefense - defense value of inflictor.

    [IN]  wAttackResistance - inflictor's resistance to physical attack.

  Return value:

    The damage value of the physical attacking.

--*/
{
   short             sDamage;

   sDamage = PAL_CalcBaseDamage(wAttackStrength, wDefense);
   if (wAttackResistance != 0)
   {
      sDamage /= wAttackResistance;
   }

   return sDamage;
}

static short
PAL_GetEnemyDexterity(
   unsigned short          wEnemyIndex
)
/*++
  Purpose:

    Get the dexterity value of the enemy.

  Parameters:

    [IN]  wEnemyIndex - the index of the enemy.

  Return value:

    The dexterity value of the enemy.

--*/
{
   short      s;

   assert(g_Battle.rgEnemy[wEnemyIndex].wObjectID != 0);

   s = (g_Battle.rgEnemy[wEnemyIndex].e.wLevel + 6) * 3;
   s += (short)g_Battle.rgEnemy[wEnemyIndex].e.wDexterity;

   return s;
}

static unsigned short
PAL_GetPlayerActualDexterity(
   unsigned short            wPlayerRole
)
/*++
  Purpose:

    Get player's actual dexterity value in battle.

  Parameters:

    [IN]  wPlayerRole - the player role ID.

  Return value:

    The player's actual dexterity value.

--*/
{
   unsigned short wDexterity = PAL_GetPlayerDexterity(wPlayerRole);

   if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusHaste] != 0)
   {
      wDexterity *= 3;
   }

   if (wDexterity > 999)
   {
      wDexterity = 999;
   }

   return wDexterity;
}

void
PAL_BattleDelay(
   unsigned short       wDuration,
   unsigned short       wObjectID,
   int       fUpdateGesture
)
/*++
  Purpose:

    Delay a while during battle.

  Parameters:

    [IN]  wDuration - Number of frames of the delay.

    [IN]  wObjectID - The object ID to be displayed during the delay.

    [IN]  fUpdateGesture - TRUE if update the gesture for enemies, FALSE if not.

  Return value:

    None.

--*/
{
   int    i, j;
   unsigned int  dwTime = SDL_GetTicks() + BATTLE_FRAME_TIME;

   for (i = 0; i < wDuration; i++)
   {
      if (fUpdateGesture)
      {
         //
         // Update the gesture of enemies.
         //
         for (j = 0; j <= g_Battle.wMaxEnemyIndex; j++)
         {
            if (g_Battle.rgEnemy[j].wObjectID == 0 ||
               g_Battle.rgEnemy[j].rgwStatus[kStatusSleep] != 0 ||
               g_Battle.rgEnemy[j].rgwStatus[kStatusParalyzed] != 0)
            {
               continue;
            }

            if (--g_Battle.rgEnemy[j].e.wIdleAnimSpeed == 0)
            {
               g_Battle.rgEnemy[j].wCurrentFrame++;
               g_Battle.rgEnemy[j].e.wIdleAnimSpeed =
                  gpGlobals->g.lprgEnemy[gpGlobals->g.rgObject[g_Battle.rgEnemy[j].wObjectID].enemy.wEnemyID].wIdleAnimSpeed;
            }

            if (g_Battle.rgEnemy[j].wCurrentFrame >= g_Battle.rgEnemy[j].e.wIdleFrames)
            {
               g_Battle.rgEnemy[j].wCurrentFrame = 0;
            }
         }
      }

      //
      // Wait for the time of one frame. Accept input here.
      //
      PAL_DelayUntil(dwTime);

      //
      // Set the time of the next frame.
      //
      dwTime = SDL_GetTicks() + BATTLE_FRAME_TIME;

      PAL_BattleMakeScene();
      VIDEO_CopyEntireSurface(g_Battle.lpSceneBuf, gpScreen);
      PAL_BattleUIUpdate();

      if (wObjectID != 0)
      {
         if (wObjectID == BATTLE_LABEL_ESCAPEFAIL) // HACKHACK
         {
            PAL_DrawText(PAL_GetWord(wObjectID), PAL_XY(130, 75), 15, TRUE, FALSE, FALSE);
         }
         else if ((short)wObjectID < 0)
         {
            PAL_DrawText(PAL_GetWord(-((short)wObjectID)), PAL_XY(170, 45), DESCTEXT_COLOR, TRUE, FALSE, FALSE);
         }
         else
         {
            PAL_DrawText(PAL_GetWord(wObjectID), PAL_XY(210, 50), 15, TRUE, FALSE, FALSE);
         }
      }

      VIDEO_UpdateScreen(NULL);
   }
}

static void
PAL_BattleBackupStat(
   void
)
/*++
  Purpose:

    Backup HP and MP values of all players and enemies.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   int          i;
   unsigned short         wPlayerRole;

   for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
   {
      if (g_Battle.rgEnemy[i].wObjectID == 0)
      {
         continue;
      }
      g_Battle.rgEnemy[i].wPrevHP = g_Battle.rgEnemy[i].e.wHealth;
   }

   for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
   {
      wPlayerRole = gpGlobals->rgParty[i].wPlayerRole;

      g_Battle.rgPlayer[i].wPrevHP =
         gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole];
      g_Battle.rgPlayer[i].wPrevMP =
         gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole];
   }
}

static int
PAL_BattleDisplayStatChange(
   void
)
/*++
  Purpose:

    Display the HP and MP changes of all players and enemies.

  Parameters:

    None.

  Return value:

    TRUE if there are any number displayed, FALSE if not.

--*/
{
   int      i, x, y;
   short    sDamage;
   unsigned short     wPlayerRole;
   int     f = FALSE;

   for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
   {
      if (g_Battle.rgEnemy[i].wObjectID == 0)
      {
         continue;
      }

      if (g_Battle.rgEnemy[i].wPrevHP != g_Battle.rgEnemy[i].e.wHealth)
      {
         //
         // Show the number of damage
         //
         sDamage = g_Battle.rgEnemy[i].e.wHealth - g_Battle.rgEnemy[i].wPrevHP;

         x = PAL_X(g_Battle.rgEnemy[i].pos) - 9;
         y = PAL_Y(g_Battle.rgEnemy[i].pos) - 115;

         if (y < 10)
         {
            y = 10;
         }

         if (sDamage < 0)
         {
            PAL_BattleUIShowNum((unsigned short)(-sDamage), PAL_XY(x, y), kNumColorBlue);
         }
         else
         {
            PAL_BattleUIShowNum((unsigned short)(sDamage), PAL_XY(x, y), kNumColorYellow);
         }

         f = TRUE;
      }
   }

   for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
   {
      wPlayerRole = gpGlobals->rgParty[i].wPlayerRole;

      if (g_Battle.rgPlayer[i].wPrevHP != gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole])
      {
         sDamage =
            gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] - g_Battle.rgPlayer[i].wPrevHP;

         x = PAL_X(g_Battle.rgPlayer[i].pos) - 9;
         y = PAL_Y(g_Battle.rgPlayer[i].pos) - 75;

         if (y < 10)
         {
            y = 10;
         }

         if (sDamage < 0)
         {
            PAL_BattleUIShowNum((unsigned short)(-sDamage), PAL_XY(x, y), kNumColorBlue);
         }
         else
         {
            PAL_BattleUIShowNum((unsigned short)(sDamage), PAL_XY(x, y), kNumColorYellow);
         }

         f = TRUE;
      }

      if (g_Battle.rgPlayer[i].wPrevMP != gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole])
      {
         sDamage =
            gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole] - g_Battle.rgPlayer[i].wPrevMP;

         x = PAL_X(g_Battle.rgPlayer[i].pos) - 9;
         y = PAL_Y(g_Battle.rgPlayer[i].pos) - 67;

         if (y < 10)
         {
            y = 10;
         }

         //
         // Only show MP increasing
         //
         if (sDamage > 0)
         {
            PAL_BattleUIShowNum((unsigned short)(sDamage), PAL_XY(x, y), kNumColorCyan);
         }

         f = TRUE;
      }
   }

   return f;
}

static void
PAL_BattlePostActionCheck(
   int      fCheckPlayers
)
/*++
  Purpose:

    Essential checks after an action is executed.

  Parameters:

    [IN]  fCheckPlayers - TRUE if check for players, FALSE if not.

  Return value:

    None.

--*/
{
   int      i, j;
   int     fFade = FALSE;
   int     fEnemyRemaining = FALSE;

   for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
   {
      if (g_Battle.rgEnemy[i].wObjectID == 0)
      {
         continue;
      }

      if ((short)(g_Battle.rgEnemy[i].e.wHealth) <= 0)
      {
         //
         // This enemy is KO'ed
         //
         g_Battle.iExpGained += g_Battle.rgEnemy[i].e.wExp;
         g_Battle.iCashGained += g_Battle.rgEnemy[i].e.wCash;

         AUDIO_PlaySound(g_Battle.rgEnemy[i].e.wDeathSound);
         g_Battle.rgEnemy[i].wObjectID = 0;
         if (g_Battle.rgEnemy[i].lpSprite)
             free(g_Battle.rgEnemy[i].lpSprite);
         g_Battle.rgEnemy[i].lpSprite = NULL;
         fFade = TRUE;

         continue;
      }

      fEnemyRemaining = TRUE;
   }

   if (!fEnemyRemaining)
   {
      g_Battle.fEnemyCleared = TRUE;
      g_Battle.UI.state = kBattleUIWait;
   }

   if (fCheckPlayers && !gpGlobals->fAutoBattle)
   {
      for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
      {
         unsigned short w = gpGlobals->rgParty[i].wPlayerRole, wName;

         if (gpGlobals->g.PlayerRoles.rgwHP[w] < g_Battle.rgPlayer[i].wPrevHP &&
            gpGlobals->g.PlayerRoles.rgwHP[w] == 0)
         {
            w = gpGlobals->g.PlayerRoles.rgwCoveredBy[w];

            for (j = 0; j <= gpGlobals->wMaxPartyMemberIndex; j++)
            {
               if (gpGlobals->rgParty[j].wPlayerRole == w)
               {
                  break;
               }
            }

            if (gpGlobals->g.PlayerRoles.rgwHP[w] > 0 &&
               gpGlobals->rgPlayerStatus[w][kStatusSleep] == 0 &&
               gpGlobals->rgPlayerStatus[w][kStatusParalyzed] == 0 &&
               gpGlobals->rgPlayerStatus[w][kStatusConfused] == 0 &&
               j <= gpGlobals->wMaxPartyMemberIndex)
            {
               wName = gpGlobals->g.PlayerRoles.rgwName[w];

               if (gpGlobals->g.rgObject[wName].player.wScriptOnFriendDeath != 0)
               {
                  PAL_BattleDelay(10, 0, TRUE);

                  PAL_BattleMakeScene();
                  VIDEO_CopyEntireSurface(g_Battle.lpSceneBuf, gpScreen);
                  VIDEO_UpdateScreen(NULL);

                  g_Battle.BattleResult = kBattleResultPause;

                  gpGlobals->g.rgObject[wName].player.wScriptOnFriendDeath =
                     PAL_RunTriggerScript(gpGlobals->g.rgObject[wName].player.wScriptOnFriendDeath, w);

                  g_Battle.BattleResult = kBattleResultOnGoing;

                  PAL_ClearKeyState();
                  goto end;
               }
            }
         }
      }

      for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
      {
         unsigned short w = gpGlobals->rgParty[i].wPlayerRole, wName;

         if (gpGlobals->rgPlayerStatus[w][kStatusSleep] != 0 ||
            gpGlobals->rgPlayerStatus[w][kStatusConfused] != 0)
         {
            continue;
         }

         if (gpGlobals->g.PlayerRoles.rgwHP[w] < g_Battle.rgPlayer[i].wPrevHP)
         {
            if (gpGlobals->g.PlayerRoles.rgwHP[w] > 0 && PAL_IsPlayerDying(w) &&
               g_Battle.rgPlayer[i].wPrevHP >= gpGlobals->g.PlayerRoles.rgwMaxHP[w] / 5)
            {
               unsigned short wCover = gpGlobals->g.PlayerRoles.rgwCoveredBy[w];

               if (gpGlobals->rgPlayerStatus[wCover][kStatusSleep] != 0 ||
                  gpGlobals->rgPlayerStatus[wCover][kStatusParalyzed] != 0 ||
                  gpGlobals->rgPlayerStatus[wCover][kStatusConfused] != 0)
               {
                  continue;
               }

               wName = gpGlobals->g.PlayerRoles.rgwName[w];

               AUDIO_PlaySound(gpGlobals->g.PlayerRoles.rgwDyingSound[w]);

               for (j = 0; j <= gpGlobals->wMaxPartyMemberIndex; j++)
               {
                  if (gpGlobals->rgParty[j].wPlayerRole == wCover)
                  {
                     break;
                  }
               }

               if (j > gpGlobals->wMaxPartyMemberIndex || gpGlobals->g.PlayerRoles.rgwHP[wCover] == 0)
               {
                  continue;
               }

               if (gpGlobals->g.rgObject[wName].player.wScriptOnDying != 0)
               {
                  PAL_BattleDelay(10, 0, TRUE);

                  PAL_BattleMakeScene();
                  VIDEO_CopyEntireSurface(g_Battle.lpSceneBuf, gpScreen);
                  VIDEO_UpdateScreen(NULL);

                  g_Battle.BattleResult = kBattleResultPause;

                  gpGlobals->g.rgObject[wName].player.wScriptOnDying =
                     PAL_RunTriggerScript(gpGlobals->g.rgObject[wName].player.wScriptOnDying, w);

                  g_Battle.BattleResult = kBattleResultOnGoing;
                  PAL_ClearKeyState();
               }

               goto end;
            }
         }
      }
   }

end:
   if (fFade)
   {
      VIDEO_BackupScreen(g_Battle.lpSceneBuf);
      PAL_BattleMakeScene();
      PAL_BattleFadeScene();
   }

   //
   // Fade out the summoned god
   //
   if (g_Battle.lpSummonSprite != NULL)
   {
      PAL_BattleUpdateFighters();
      PAL_BattleDelay(1, 0, FALSE);

      free(g_Battle.lpSummonSprite);
      g_Battle.lpSummonSprite = NULL;

      g_Battle.sBackgroundColorShift = 0;

      VIDEO_BackupScreen(g_Battle.lpSceneBuf);
      PAL_BattleMakeScene();
      PAL_BattleFadeScene();
   }
}

void
PAL_BattleUpdateFighters(
   void
)
/*++
  Purpose:

    Update players' and enemies' gestures and locations in battle.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   int        i;
   unsigned short       wPlayerRole;

   //
   // Update the gesture for all players
   //
   for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
   {
      wPlayerRole = gpGlobals->rgParty[i].wPlayerRole;

      if(!g_Battle.rgPlayer[i].fDefending)
         g_Battle.rgPlayer[i].pos = g_Battle.rgPlayer[i].posOriginal;
      g_Battle.rgPlayer[i].iColorShift = 0;

      if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] == 0)
      {
         if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusPuppet] == 0)
         {
            g_Battle.rgPlayer[i].wCurrentFrame = 2; // dead
         }
         else
         {
            g_Battle.rgPlayer[i].wCurrentFrame = 0; // puppet
         }
      }
      else
      {
         if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSleep] != 0 ||
            PAL_IsPlayerDying(wPlayerRole))
         {
            g_Battle.rgPlayer[i].wCurrentFrame = 1;
         }
         else if (g_Battle.rgPlayer[i].fDefending && !g_Battle.fEnemyCleared)
         {
            g_Battle.rgPlayer[i].wCurrentFrame = 3;
         }
         else
         {
            g_Battle.rgPlayer[i].wCurrentFrame = 0;
         }
      }
   }

   //
   // Update the gesture for all enemies
   //
   for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
   {
      if (g_Battle.rgEnemy[i].wObjectID == 0)
      {
         continue;
      }

      g_Battle.rgEnemy[i].pos = g_Battle.rgEnemy[i].posOriginal;
      g_Battle.rgEnemy[i].iColorShift = 0;

      if (g_Battle.rgEnemy[i].rgwStatus[kStatusSleep] > 0 ||
         g_Battle.rgEnemy[i].rgwStatus[kStatusParalyzed] > 0)
      {
         g_Battle.rgEnemy[i].wCurrentFrame = 0;
         continue;
      }

      if (--g_Battle.rgEnemy[i].e.wIdleAnimSpeed == 0)
      {
         g_Battle.rgEnemy[i].wCurrentFrame++;
         g_Battle.rgEnemy[i].e.wIdleAnimSpeed =
            gpGlobals->g.lprgEnemy[gpGlobals->g.rgObject[g_Battle.rgEnemy[i].wObjectID].enemy.wEnemyID].wIdleAnimSpeed;
      }

      if (g_Battle.rgEnemy[i].wCurrentFrame >= g_Battle.rgEnemy[i].e.wIdleFrames)
      {
         g_Battle.rgEnemy[i].wCurrentFrame = 0;
      }
   }
}

void
PAL_BattlePlayerCheckReady(
   void
)
/*++
  Purpose:

    Check if there are player who is ready.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   float   flMax = 0;
   int     iMax = 0, i;

   //
   // Start the UI for the fastest and ready player
   //
   for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
   {
      if (g_Battle.rgPlayer[i].state == kFighterCom ||
         (g_Battle.rgPlayer[i].state == kFighterAct && g_Battle.rgPlayer[i].action.ActionType == kBattleActionCoopMagic))
      {
         flMax = 0;
         break;
      }
      else if (g_Battle.rgPlayer[i].state == kFighterWait)
      {
         if (g_Battle.rgPlayer[i].flTimeMeter > flMax)
         {
            iMax = i;
            flMax = g_Battle.rgPlayer[i].flTimeMeter;
         }
      }
   }

   if (flMax >= 100.0f)
   {
      g_Battle.rgPlayer[iMax].state = kFighterCom;
      g_Battle.rgPlayer[iMax].fDefending = FALSE;
   }
}

void
PAL_BattleStartFrame(
   void
)
/*++
  Purpose:

    Called once per video frame in battle.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   int                      i, j;
   unsigned short                     wPlayerRole;
   unsigned short                     wDexterity;
   int                     fOnlyPuppet = TRUE;

   if (!g_Battle.fEnemyCleared)
   {
      PAL_BattleUpdateFighters();
   }

   //
   // Update the scene
   //
   PAL_BattleMakeScene();
   VIDEO_CopyEntireSurface(g_Battle.lpSceneBuf, gpScreen);

   //
   // Check if the battle is over
   //
   if (g_Battle.fEnemyCleared)
   {
      //
      // All enemies are cleared. Won the battle.
      //
      g_Battle.BattleResult = kBattleResultWon;
      AUDIO_PlaySound(0);
      return;
   }
   else
   {
      int fEnded = TRUE;

      for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
      {
         wPlayerRole = gpGlobals->rgParty[i].wPlayerRole;

         if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] != 0)
         {
            fOnlyPuppet = FALSE;
            fEnded = FALSE;
            break;
         }
         else if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusPuppet] != 0)
         {
            fOnlyPuppet = FALSE;
         }
      }

      if (fEnded)
      {
         //
         // All players are dead. Lost the battle.
         //
         g_Battle.BattleResult = kBattleResultLost;
         return;
      }
   }

   if (g_Battle.Phase == kBattlePhaseSelectAction)
   {
      if (g_Battle.UI.state == kBattleUIWait)
      {
         for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
         {
            wPlayerRole = gpGlobals->rgParty[i].wPlayerRole;

            //
            // Don't select action for this player if player is KO'ed,
            // sleeped, confused or paralyzed
            //
            if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] == 0 ||
               gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSleep] ||
               gpGlobals->rgPlayerStatus[wPlayerRole][kStatusConfused] ||
               gpGlobals->rgPlayerStatus[wPlayerRole][kStatusParalyzed])
            {
               continue;
            }

            //
            // Start the menu for the first player whose action is not
            // yet selected
            //
            if (g_Battle.rgPlayer[i].state == kFighterWait)
            {
               g_Battle.wMovingPlayerIndex = i;
               g_Battle.rgPlayer[i].state = kFighterCom;
               PAL_BattleUIPlayerReady(i);
               break;
            }
            else if (g_Battle.rgPlayer[i].action.ActionType == kBattleActionCoopMagic)
            {
               //
               // Skip other players if someone selected coopmagic
               //
               i = gpGlobals->wMaxPartyMemberIndex + 1;
               break;
            }
         }

         if (i > gpGlobals->wMaxPartyMemberIndex)
         {
            //
            // Backup all actions once not repeating.
            //
            if (!g_Battle.fRepeat)
            {
               for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
               {
                  g_Battle.rgPlayer[i].prevAction = g_Battle.rgPlayer[i].action;
               }
            }

            //
            // actions for all players are decided. fill in the action queue.
            //
            g_Battle.fRepeat = FALSE;
            g_Battle.fForce = FALSE;
            g_Battle.fFlee = FALSE;
            g_Battle.fPrevAutoAtk = g_Battle.UI.fAutoAttack;
            g_Battle.fPrevPlayerAutoAtk = FALSE;

            g_Battle.iCurAction = 0;

            for (i = 0; i < MAX_ACTIONQUEUE_ITEMS; i++)
            {
               g_Battle.ActionQueue[i].wIndex = 0xFFFF;
               g_Battle.ActionQueue[i].fIsSecond = FALSE;
               g_Battle.ActionQueue[i].wDexterity = 0xFFFF;
            }

            j = 0;

            //
            // Put all enemies into action queue
            //
            for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
            {
               if (g_Battle.rgEnemy[i].wObjectID == 0)
               {
                  continue;
               }

               g_Battle.ActionQueue[j].fIsEnemy = TRUE;
               g_Battle.ActionQueue[j].wIndex = i;
               g_Battle.ActionQueue[j].fIsSecond = FALSE;
               g_Battle.ActionQueue[j].wDexterity = PAL_GetEnemyDexterity(i);
               g_Battle.ActionQueue[j].wDexterity = (unsigned short)((float)g_Battle.ActionQueue[j].wDexterity * RandomFloat(0.9f, 1.1f));

               j++;

               if (g_Battle.rgEnemy[i].e.wDualMove)
               {
                  g_Battle.ActionQueue[j].fIsEnemy = TRUE;
                  g_Battle.ActionQueue[j].wIndex = i;
                  g_Battle.ActionQueue[j].fIsSecond = FALSE;
                  g_Battle.ActionQueue[j].wDexterity = PAL_GetEnemyDexterity(i);
                  g_Battle.ActionQueue[j].wDexterity = (unsigned short)((float)g_Battle.ActionQueue[j].wDexterity * RandomFloat(0.9f, 1.1f));

                  if(g_Battle.ActionQueue[j].wDexterity <= g_Battle.ActionQueue[j-1].wDexterity)
                      g_Battle.ActionQueue[j].fIsSecond = TRUE;
                  else
                      g_Battle.ActionQueue[j-1].fIsSecond = TRUE;

                  j++;
               }
            }

            //
            // Put all players into action queue
            //
            for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
            {
               wPlayerRole = gpGlobals->rgParty[i].wPlayerRole;

               g_Battle.ActionQueue[j].fIsEnemy = FALSE;
               g_Battle.ActionQueue[j].wIndex = i;

               if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] == 0 ||
                  gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSleep] > 0 ||
                  gpGlobals->rgPlayerStatus[wPlayerRole][kStatusParalyzed] > 0)
               {
                  //
                  // players who are unable to move should attack physically if recovered
                  // in the same turn
                  //
                  g_Battle.ActionQueue[j].wDexterity = 0;
                  g_Battle.rgPlayer[i].action.ActionType = kBattleActionAttack;
                  g_Battle.rgPlayer[i].action.wActionID = 0;
                  g_Battle.rgPlayer[i].state = kFighterAct;
               }
               else
               {
                  wDexterity = PAL_GetPlayerActualDexterity(wPlayerRole);

                  if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusConfused] > 0)
                  {
                     g_Battle.rgPlayer[i].action.ActionType = kBattleActionAttack;
                     g_Battle.rgPlayer[i].action.wActionID = 0; //avoid be deduced to autoattack
                     g_Battle.rgPlayer[i].state = kFighterAct;
                  }

                  switch (g_Battle.rgPlayer[i].action.ActionType)
                  {
                  case kBattleActionCoopMagic:
                     wDexterity *= 10;
                     break;

                  case kBattleActionDefend:
                     wDexterity *= 5;
                     break;

                  case kBattleActionMagic:
                     if ((gpGlobals->g.rgObject[g_Battle.rgPlayer[i].action.wActionID].magic.wFlags & kMagicFlagUsableToEnemy) == 0)
                     {
                        wDexterity *= 3;
                     }
                     break;

                  case kBattleActionFlee:
                     wDexterity /= 2;
                     break;

                  case kBattleActionUseItem:
                     wDexterity *= 3;
                     break;

                  default:
                     break;
                  }

                  if (PAL_IsPlayerDying(wPlayerRole))
                  {
                     wDexterity /= 2;
                  }

                  wDexterity = (unsigned short)((float)wDexterity * RandomFloat(0.9f, 1.1f));

                  g_Battle.ActionQueue[j].wDexterity = wDexterity;
               }

               j++;
            }

            //
            // Sort the action queue by dexterity value
            //
            for (i = 0; i < MAX_ACTIONQUEUE_ITEMS; i++)
            {
               for (j = i; j < MAX_ACTIONQUEUE_ITEMS; j++)
               {
                  if ((short)g_Battle.ActionQueue[i].wDexterity < (short)g_Battle.ActionQueue[j].wDexterity)
                  {
                     ACTIONQUEUE t = g_Battle.ActionQueue[i];
                     g_Battle.ActionQueue[i] = g_Battle.ActionQueue[j];
                     g_Battle.ActionQueue[j] = t;
                  }
               }
            }

            //
            // Perform the actions
            //
            g_Battle.Phase = kBattlePhasePerformAction;
         }
      }
   }
   else
   {
      //
      // Are all actions finished?
      //
      if (g_Battle.iCurAction >= MAX_ACTIONQUEUE_ITEMS ||
         g_Battle.ActionQueue[g_Battle.iCurAction].wDexterity == 0xFFFF)
      {
         for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
         {
            g_Battle.rgPlayer[i].fDefending = FALSE;
            //
            // Restore player pos from MANUAL defending
            //
            g_Battle.rgPlayer[i].pos = g_Battle.rgPlayer[i].posOriginal;
         }

         //
         // Run poison scripts
         //
         PAL_BattleBackupStat();

         for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
         {
            wPlayerRole = gpGlobals->rgParty[i].wPlayerRole;

            for (j = 0; j < MAX_POISONS; j++)
            {
               if (gpGlobals->rgPoisonStatus[j][i].wPoisonID != 0)
               {
                  gpGlobals->rgPoisonStatus[j][i].wPoisonScript =
                     PAL_RunTriggerScript(gpGlobals->rgPoisonStatus[j][i].wPoisonScript, wPlayerRole);
               }
            }

            //
            // Update statuses
            //
            for (j = 0; j < kStatusAll; j++)
            {
               if (gpGlobals->rgPlayerStatus[wPlayerRole][j] > 0)
               {
                  gpGlobals->rgPlayerStatus[wPlayerRole][j]--;
               }
            }
         }

         for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
         {
            for (j = 0; j < MAX_POISONS; j++)
            {
               if (g_Battle.rgEnemy[i].rgPoisons[j].wPoisonID != 0)
               {
                  g_Battle.rgEnemy[i].rgPoisons[j].wPoisonScript =
                     PAL_RunTriggerScript(g_Battle.rgEnemy[i].rgPoisons[j].wPoisonScript, (unsigned short)i);
               }
            }

            //
            // Update statuses
            //
            for (j = 0; j < kStatusAll; j++)
            {
               if (g_Battle.rgEnemy[i].rgwStatus[j] > 0)
               {
                  g_Battle.rgEnemy[i].rgwStatus[j]--;
               }
            }
         }

         PAL_BattlePostActionCheck(FALSE);
         if (PAL_BattleDisplayStatChange())
         {
            PAL_BattleDelay(8, 0, TRUE);
         }

         if (g_Battle.iHidingTime > 0)
         {
            if (--g_Battle.iHidingTime == 0)
            {
               VIDEO_BackupScreen(g_Battle.lpSceneBuf);
               PAL_BattleMakeScene();
               PAL_BattleFadeScene();
            }
         }

         if (g_Battle.iHidingTime == 0)
         {
            for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
            {
               if (g_Battle.rgEnemy[i].wObjectID == 0)
               {
                  continue;
               }

               g_Battle.rgEnemy[i].wScriptOnTurnStart =
                  PAL_RunTriggerScript(g_Battle.rgEnemy[i].wScriptOnTurnStart, i);
            }
         }

         //
         // Clear all item-using records
         //
         for (i = 0; i < MAX_INVENTORY; i++)
         {
            gpGlobals->rgInventory[i].nAmountInUse = 0;
         }

         //
         // Proceed to next turn...
         //
         g_Battle.Phase = kBattlePhaseSelectAction;
         g_Battle.fThisTurnCoop = FALSE;
      }
      else
      {
         i = g_Battle.ActionQueue[g_Battle.iCurAction].wIndex;

         if (g_Battle.ActionQueue[g_Battle.iCurAction].fIsEnemy)
         {
            if (g_Battle.iHidingTime == 0 && !fOnlyPuppet &&
               g_Battle.rgEnemy[i].wObjectID != 0)
            {
               g_Battle.rgEnemy[i].wScriptOnReady =
                  PAL_RunTriggerScript(g_Battle.rgEnemy[i].wScriptOnReady, i);

               g_Battle.fEnemyMoving = TRUE;
               PAL_BattleEnemyPerformAction(i);
               g_Battle.fEnemyMoving = FALSE;
            }
         }
         else if (g_Battle.rgPlayer[i].state == kFighterAct)
         {
            wPlayerRole = gpGlobals->rgParty[i].wPlayerRole;

            if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] == 0)
            {
               if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusPuppet] == 0)
               {
                  g_Battle.rgPlayer[i].action.ActionType = kBattleActionPass;
               }
            }
            else if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSleep] > 0 ||
               gpGlobals->rgPlayerStatus[wPlayerRole][kStatusParalyzed] > 0)
            {
               g_Battle.rgPlayer[i].action.ActionType = kBattleActionPass;
            }
            else if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusConfused] > 0)
            {
               g_Battle.rgPlayer[i].action.ActionType =
                  (PAL_IsPlayerDying(wPlayerRole) ? kBattleActionPass : kBattleActionAttackMate);
            }
            else if (g_Battle.rgPlayer[i].action.ActionType == kBattleActionAttack &&
               g_Battle.rgPlayer[i].action.wActionID != 0)
            {
               g_Battle.fPrevPlayerAutoAtk = TRUE;
            }
            else if (g_Battle.fPrevPlayerAutoAtk)
            {
               g_Battle.UI.wCurPlayerIndex = i;
               g_Battle.UI.iSelectedIndex = g_Battle.rgPlayer[i].action.sTarget;
               g_Battle.UI.wActionType = kBattleActionAttack;
               PAL_BattleCommitAction(FALSE);
            }

            //
            // Perform the action for this player.
            //
            g_Battle.wMovingPlayerIndex = i;
            PAL_BattlePlayerPerformAction(i);
         }

         g_Battle.iCurAction++;
      }
   }

   //
   // The R and F keys and Fleeing should affect all players
   //
   if (g_Battle.UI.MenuState == kBattleMenuMain &&
      g_Battle.UI.state == kBattleUISelectMove)
   {
      if (g_InputState.dwKeyPress & kKeyRepeat)
      {
         g_Battle.fRepeat = TRUE;
         g_Battle.UI.fAutoAttack = g_Battle.fPrevAutoAtk;
      }
      else if (g_InputState.dwKeyPress & kKeyForce)
      {
         g_Battle.fForce = TRUE;
      }
   }

   if (g_Battle.fRepeat)
   {
      g_InputState.dwKeyPress = kKeyRepeat;
   }
   else if (g_Battle.fForce)
   {
      g_InputState.dwKeyPress = kKeyForce;
   }
   else if (g_Battle.fFlee)
   {
      g_InputState.dwKeyPress = kKeyFlee;
   }

   //
   // Update the battle UI
   //
   PAL_BattleUIUpdate();


}

void
PAL_BattleCommitAction(
   int           fRepeat
)
/*++
  Purpose:

    Commit the action which the player decided.

  Parameters:

    [IN]  fRepeat - TRUE if repeat the last action.

  Return value:

    None.

--*/
{
   unsigned short      w;

   if (!fRepeat)
   {
      //clear action cache first; avoid cache pollution
      memset(&g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action,0,sizeof(g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action));
      g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.ActionType =
         g_Battle.UI.wActionType;
      g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.sTarget =
         (short)g_Battle.UI.iSelectedIndex;

      if (g_Battle.UI.wActionType == kBattleActionAttack)
      {
         g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.wActionID =
            (g_Battle.UI.fAutoAttack ? 1 : 0);
      }
      else
      {
         g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.wActionID =
            g_Battle.UI.wObjectID;
	  }
   }
   else
   {
      short target = g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.sTarget;
      g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action =
         g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].prevAction;
      g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.sTarget = target;

      if (g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.ActionType == kBattleActionPass)
      {
         g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.ActionType = kBattleActionAttack;
         g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.wActionID = 0;
         g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.sTarget = -1;
      }
   }

   //
   // Check if the action is valid
   //
   switch (g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.ActionType)
   {
   case kBattleActionMagic:
      w = g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.wActionID;
      w = gpGlobals->g.lprgMagic[gpGlobals->g.rgObject[w].magic.wMagicNumber].wCostMP;

      if (gpGlobals->g.PlayerRoles.rgwMP[gpGlobals->rgParty[g_Battle.UI.wCurPlayerIndex].wPlayerRole] < w)
      {
         w = g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.wActionID;
         w = gpGlobals->g.lprgMagic[gpGlobals->g.rgObject[w].magic.wMagicNumber].wType;
         if (w == kMagicTypeApplyToPlayer || w == kMagicTypeApplyToParty ||
            w == kMagicTypeTrance)
         {
            g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.ActionType = kBattleActionDefend;
         }
         else
         {
            g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.ActionType = kBattleActionAttack;
            if (g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.sTarget == -1)
            {
               g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.sTarget = 0;
            }
            g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.wActionID = 0;
         }
      }
      break;

   case kBattleActionUseItem:
      if ((gpGlobals->g.rgObject[g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.wActionID].item.wFlags & kItemFlagConsuming) == 0)
      {
         break;
      }

   case kBattleActionThrowItem:
      for (w = 0; w < MAX_INVENTORY; w++)
      {
         if (gpGlobals->rgInventory[w].wItem == g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].action.wActionID)
         {
            gpGlobals->rgInventory[w].nAmountInUse++;
            break;
         }
      }
      break;

   default:
      break;
   }

   if (g_Battle.UI.wActionType == kBattleActionFlee)
   {
      g_Battle.fFlee = TRUE;
   }

   g_Battle.rgPlayer[g_Battle.UI.wCurPlayerIndex].state = kFighterAct;
   g_Battle.UI.state = kBattleUIWait;

}

static void
PAL_BattleShowPlayerAttackAnim(
   unsigned short        wPlayerIndex,
   int        fCritical
)
/*++
  Purpose:

    Show the physical attack effect for player.

  Parameters:

    [IN]  wPlayerIndex - the index of the player.

    [IN]  fCritical - TRUE if this is a critical hit.

  Return value:

    None.

--*/
{
   unsigned short wPlayerRole = gpGlobals->rgParty[wPlayerIndex].wPlayerRole;
   short sTarget = g_Battle.rgPlayer[wPlayerIndex].action.sTarget;

   int index, i, j;
   int enemy_x = 0, enemy_y = 0, enemy_h = 0, x, y, dist = 0;

   unsigned int dwTime;

   if (sTarget != -1)
   {
      enemy_x = PAL_X(g_Battle.rgEnemy[sTarget].pos);
      enemy_y = PAL_Y(g_Battle.rgEnemy[sTarget].pos);

      enemy_h = PAL_RLEGetHeight(PAL_SpriteGetFrame(g_Battle.rgEnemy[sTarget].lpSprite, g_Battle.rgEnemy[sTarget].wCurrentFrame));

      if (sTarget >= 3)
      {
         dist = (sTarget - wPlayerIndex) * 8;
      }
   }
   else
   {
      enemy_x = 150;
      enemy_y = 100;
   }

   index = gpGlobals->g.rgwBattleEffectIndex[PAL_GetPlayerBattleSprite(wPlayerRole)][1];
   index *= 3;

   //
   // Play the attack voice
   //
   if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] > 0)
   {
      if (!fCritical)
      {
         AUDIO_PlaySound(gpGlobals->g.PlayerRoles.rgwAttackSound[wPlayerRole]);
      }
      else
      {
         AUDIO_PlaySound(gpGlobals->g.PlayerRoles.rgwCriticalSound[wPlayerRole]);
      }
   }

   //
   // Show the animation
   //
   x = enemy_x - dist + 64;
   y = enemy_y + dist + 20;

   g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 8;
   if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusDualAttack] > 0 && PAL_PlayerCanAttackAll(wPlayerRole))
   {
       x = g_rgPlayerPos[gpGlobals->wMaxPartyMemberIndex][wPlayerIndex][0] - 8;
       y = g_rgPlayerPos[gpGlobals->wMaxPartyMemberIndex][wPlayerIndex][1] - 4;

       if (g_Battle.rgPlayer[wPlayerIndex].fSecondAttack == FALSE)
       {
           g_Battle.rgPlayer[wPlayerIndex].pos = PAL_XY(x, y);
       }
       if (g_Battle.rgPlayer[wPlayerIndex].fSecondAttack == TRUE)
       {
           g_Battle.rgPlayer[wPlayerIndex].pos = PAL_XY(x - 12, y - 8);
       }
   }
   else
       g_Battle.rgPlayer[wPlayerIndex].pos = PAL_XY(x, y);

   PAL_BattleDelay(2, 0, TRUE);

   x -= 10;
   y -= 2;
   if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusDualAttack] > 0 && PAL_PlayerCanAttackAll(wPlayerRole))
   {
       x = g_rgPlayerPos[gpGlobals->wMaxPartyMemberIndex][wPlayerIndex][0] - 8;
       y = g_rgPlayerPos[gpGlobals->wMaxPartyMemberIndex][wPlayerIndex][1] - 4;

       if (g_Battle.rgPlayer[wPlayerIndex].fSecondAttack == FALSE)
       {
           g_Battle.rgPlayer[wPlayerIndex].pos = PAL_XY(x, y);
       }
       if (g_Battle.rgPlayer[wPlayerIndex].fSecondAttack == TRUE)
       {
           g_Battle.rgPlayer[wPlayerIndex].pos = PAL_XY(x - 12, y - 8);
       }
   }
   else
       g_Battle.rgPlayer[wPlayerIndex].pos = PAL_XY(x, y);

   PAL_BattleDelay(1, 0, TRUE);

   g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 9;
   x -= 16;
   y -= 4;

   AUDIO_PlaySound(gpGlobals->g.PlayerRoles.rgwWeaponSound[wPlayerRole]);

   x = enemy_x;
   y = enemy_y - enemy_h / 3 + 10;

   dwTime = SDL_GetTicks() + BATTLE_FRAME_TIME;

   for (i = 0; i < 3; i++)
   {
      const unsigned char* b = PAL_SpriteGetFrame(g_Battle.lpEffectSprite, index++);

      //
      // Wait for the time of one frame. Accept input here.
      //
      PAL_DelayUntil(dwTime);

      //
      // Set the time of the next frame.
      //
      dwTime = SDL_GetTicks() + BATTLE_FRAME_TIME;

      //
      // Update the gesture of enemies.
      //
      for (j = 0; j <= g_Battle.wMaxEnemyIndex; j++)
      {
         if (g_Battle.rgEnemy[j].wObjectID == 0 ||
            g_Battle.rgEnemy[j].rgwStatus[kStatusSleep] > 0 ||
            g_Battle.rgEnemy[j].rgwStatus[kStatusParalyzed] > 0)
         {
            continue;
         }

         if (--g_Battle.rgEnemy[j].e.wIdleAnimSpeed == 0)
         {
            g_Battle.rgEnemy[j].wCurrentFrame++;
            g_Battle.rgEnemy[j].e.wIdleAnimSpeed =
               gpGlobals->g.lprgEnemy[gpGlobals->g.rgObject[g_Battle.rgEnemy[j].wObjectID].enemy.wEnemyID].wIdleAnimSpeed;
         }

         if (g_Battle.rgEnemy[j].wCurrentFrame >= g_Battle.rgEnemy[j].e.wIdleFrames)
         {
            g_Battle.rgEnemy[j].wCurrentFrame = 0;
         }
      }

      PAL_BattleMakeScene();
      VIDEO_CopyEntireSurface(g_Battle.lpSceneBuf, gpScreen);

      if (PAL_PlayerCanAttackAll(wPlayerRole))
      {
          for (j = 0; j < MAX_ENEMIES_IN_TEAM; j++)
          {
              if (g_Battle.rgEnemy[j].wObjectID != 0)
              {
                  x = gpGlobals->g.EnemyPos.pos[j][g_Battle.wMaxEnemyIndex].x;
                  y = gpGlobals->g.EnemyPos.pos[j][g_Battle.wMaxEnemyIndex].y;
                  y += g_Battle.rgEnemy[j].e.wYPosOffset;

                  PAL_RLEBlitToSurface(b, gpScreen, PAL_XY(x - PAL_RLEGetWidth(b) / 2, y - PAL_RLEGetHeight(b)));
              }
          }
      }
      else
          PAL_RLEBlitToSurface(b, gpScreen, PAL_XY(x - PAL_RLEGetWidth(b) / 2, y - PAL_RLEGetHeight(b)));

      x -= 16;
      y += 16;

      PAL_BattleUIUpdate();

      if (i == 0)
      {
         if (sTarget == -1)
         {
            for (j = 0; j <= g_Battle.wMaxEnemyIndex; j++)
            {
               g_Battle.rgEnemy[j].iColorShift = 6;
            }
         }
         else
         {
            g_Battle.rgEnemy[sTarget].iColorShift = 6;
         }

         PAL_BattleDisplayStatChange();
         PAL_BattleBackupStat();
      }

      VIDEO_UpdateScreen(NULL);

      if (i == 1)
      {
         g_Battle.rgPlayer[wPlayerIndex].pos =
            PAL_XY(PAL_X(g_Battle.rgPlayer[wPlayerIndex].pos) + 2,
                   PAL_Y(g_Battle.rgPlayer[wPlayerIndex].pos) + 1);
      }
   }

   dist = 8;

   for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
   {
      g_Battle.rgEnemy[i].iColorShift = 0;
   }

   if (sTarget == -1)
   {
      for (i = 0; i < 3; i++)
      {
         for (j = 0; j <= g_Battle.wMaxEnemyIndex; j++)
         {
            x = PAL_X(g_Battle.rgEnemy[j].pos);
            y = PAL_Y(g_Battle.rgEnemy[j].pos);

            x -= dist;
//            y -= dist / 2;
            g_Battle.rgEnemy[j].pos = PAL_XY(x, y);
         }

         PAL_BattleDelay(1, 0, TRUE);
         dist /= -2;
      }
   }
   else
   {
      x = PAL_X(g_Battle.rgEnemy[sTarget].pos);
      y = PAL_Y(g_Battle.rgEnemy[sTarget].pos);

      for (i = 0; i < 3; i++)
      {
         x -= dist;
         dist /= -2;
         y += dist;
         g_Battle.rgEnemy[sTarget].pos = PAL_XY(x, y);

         PAL_BattleDelay(1, 0, TRUE);
      }
   }
}

static void
PAL_BattleShowPlayerUseItemAnim(
   unsigned short         wPlayerIndex,
   unsigned short         wObjectID,
   short        sTarget
)
/*++
  Purpose:

    Show the "use item" effect for player.

  Parameters:

    [IN]  wPlayerIndex - the index of the player.

    [IN]  wObjectID - the object ID of the item to be used.

    [IN]  sTarget - the target player of the action.

  Return value:

    None.

--*/
{
   int i, j;

   PAL_BattleDelay(4, 0, TRUE);

   g_Battle.rgPlayer[wPlayerIndex].pos =
      PAL_XY(PAL_X(g_Battle.rgPlayer[wPlayerIndex].pos) - 15,
             PAL_Y(g_Battle.rgPlayer[wPlayerIndex].pos) - 7);

   g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 5;

   AUDIO_PlaySound(28);

   for (i = 0; i <= 6; i++)
   {
      if (sTarget == -1)
      {
         for (j = 0; j <= gpGlobals->wMaxPartyMemberIndex; j++)
         {
            g_Battle.rgPlayer[j].iColorShift = i;
         }
      }
      else
      {
         g_Battle.rgPlayer[sTarget].iColorShift = i;
      }

      PAL_BattleDelay(1, wObjectID, TRUE);
   }

   for (i = 5; i >= 0; i--)
   {
      if (sTarget == -1)
      {
         for (j = 0; j <= gpGlobals->wMaxPartyMemberIndex; j++)
         {
            g_Battle.rgPlayer[j].iColorShift = i;
         }
      }
      else
      {
         g_Battle.rgPlayer[sTarget].iColorShift = i;
      }

      PAL_BattleDelay(1, wObjectID, TRUE);
   }
}

void
PAL_BattleShowPlayerPreMagicAnim(
   unsigned short         wPlayerIndex,
   int         fSummon
)
/*++
  Purpose:

    Show the effect for player before using a magic.

  Parameters:

    [IN]  wPlayerIndex - the index of the player.

    [IN]  fSummon - TRUE if player is using a summon magic.

  Return value:

    None.

--*/
{
   int   i, j;
   unsigned int dwTime = SDL_GetTicks();
   unsigned short  wPlayerRole = gpGlobals->rgParty[wPlayerIndex].wPlayerRole;

   for (i = 0; i < 4; i++)
   {
      g_Battle.rgPlayer[wPlayerIndex].pos =
         PAL_XY(PAL_X(g_Battle.rgPlayer[wPlayerIndex].pos) - (4 - i),
                PAL_Y(g_Battle.rgPlayer[wPlayerIndex].pos) - (4 - i) / 2);

      PAL_BattleDelay(1, 0, TRUE);
   }

   PAL_BattleDelay(2, 0, TRUE);

   g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 5;
   if (!gConfig.fIsWIN95)
   {
      AUDIO_PlaySound(gpGlobals->g.PlayerRoles.rgwMagicSound[wPlayerRole]);
   }

   if (!fSummon)
   {
      int x, y, index;

      x = PAL_X(g_Battle.rgPlayer[wPlayerIndex].pos);
      y = PAL_Y(g_Battle.rgPlayer[wPlayerIndex].pos);

      index = gpGlobals->g.rgwBattleEffectIndex[PAL_GetPlayerBattleSprite(wPlayerRole)][0];
      index *= 10;
      index += 15;
	  if (gConfig.fIsWIN95)
	  {
		  AUDIO_PlaySound(gpGlobals->g.PlayerRoles.rgwMagicSound[wPlayerRole]);
	  }
	  for (i = 0; i < 10; i++)
      {
         const unsigned char* b = PAL_SpriteGetFrame(g_Battle.lpEffectSprite, index++);

         //
         // Wait for the time of one frame. Accept input here.
         //
		 PAL_DelayUntil(dwTime);

         //
         // Set the time of the next frame.
         //
         dwTime = SDL_GetTicks() + BATTLE_FRAME_TIME;

         //
         // Update the gesture of enemies.
         //
         for (j = 0; j <= g_Battle.wMaxEnemyIndex; j++)
         {
            if (g_Battle.rgEnemy[j].wObjectID == 0 ||
               g_Battle.rgEnemy[j].rgwStatus[kStatusSleep] != 0 ||
               g_Battle.rgEnemy[j].rgwStatus[kStatusParalyzed] != 0)
            {
               continue;
            }

            if (--g_Battle.rgEnemy[j].e.wIdleAnimSpeed == 0)
            {
               g_Battle.rgEnemy[j].wCurrentFrame++;
               g_Battle.rgEnemy[j].e.wIdleAnimSpeed =
                  gpGlobals->g.lprgEnemy[gpGlobals->g.rgObject[g_Battle.rgEnemy[j].wObjectID].enemy.wEnemyID].wIdleAnimSpeed;
            }

            if (g_Battle.rgEnemy[j].wCurrentFrame >= g_Battle.rgEnemy[j].e.wIdleFrames)
            {
               g_Battle.rgEnemy[j].wCurrentFrame = 0;
            }
         }

         PAL_BattleMakeScene();
         VIDEO_CopyEntireSurface(g_Battle.lpSceneBuf, gpScreen);

         PAL_RLEBlitToSurface(b, gpScreen, PAL_XY(x - PAL_RLEGetWidth(b) / 2, y - PAL_RLEGetHeight(b)));

         PAL_BattleUIUpdate();

         VIDEO_UpdateScreen(NULL);
      }
   }

   PAL_BattleDelay(1, 0, TRUE);
}

static void
PAL_BattleShowPlayerDefMagicAnim(
   unsigned short         wPlayerIndex,
   unsigned short         wObjectID,
   short        sTarget
)
/*++
  Purpose:

    Show the defensive magic effect for player.

  Parameters:

    [IN]  wPlayerIndex - the index of the player.

    [IN]  wObjectID - the object ID of the magic to be used.

    [IN]  sTarget - the target player of the action.

  Return value:

    None.

--*/
{
   unsigned char *   lpSpriteEffect;
   int        l, iMagicNum, iEffectNum, n, i, j, x, y;
   unsigned int      dwTime = SDL_GetTicks();
   short      sLayerOffset;

   iMagicNum = gpGlobals->g.rgObject[wObjectID].magic.wMagicNumber;
   iEffectNum = gpGlobals->g.lprgMagic[iMagicNum].wEffect;

   l = PAL_MKFGetDecompressedSize(iEffectNum, gpGlobals->f.fpFIRE);
   if (l <= 0)
   {
      return;
   }

   lpSpriteEffect = (unsigned char *)UTIL_malloc(l);

   PAL_MKFDecompressChunk((unsigned char *)lpSpriteEffect, l, iEffectNum, gpGlobals->f.fpFIRE);

   n = PAL_SpriteGetNumFrames(lpSpriteEffect);

   g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 6;
   PAL_BattleDelay(1, 0, TRUE);

   for (i = 0; i < n; i++)
   {
      g_Battle.lpMagicBitmap = PAL_SpriteGetFrame(lpSpriteEffect, i);

      if (i == (gConfig.fIsWIN95 ? 0 : gpGlobals->g.lprgMagic[iMagicNum].wFireDelay))
      {
         AUDIO_PlaySound(gpGlobals->g.lprgMagic[iMagicNum].wSound);
      }

      //
      // Wait for the time of one frame. Accept input here.
      //
      PAL_DelayUntil(dwTime);

      //
      // Set the time of the next frame.
      //
      dwTime = SDL_GetTicks() +
         (gpGlobals->g.lprgMagic[iMagicNum].wSpeed + 5) * 10;

      //
      // Magic layers offset
      //
      sLayerOffset = (short)gpGlobals->g.lprgMagic[iMagicNum].rgSpecific.sLayerOffset;
      
      //
      // Unlocks the sprite sequence to add some image objects.
      //
      PAL_BattleSpriteAddUnlock();

      if (gpGlobals->g.lprgMagic[iMagicNum].wType == kMagicTypeApplyToParty)
      {
         assert(sTarget == -1);

         for (l = 0; l <= gpGlobals->wMaxPartyMemberIndex; l++)
         {
            x = PAL_X(g_Battle.rgPlayer[l].pos);
            y = PAL_Y(g_Battle.rgPlayer[l].pos);

            x += (short) gpGlobals->g.lprgMagic[iMagicNum].wXOffset;
            y += (short) gpGlobals->g.lprgMagic[iMagicNum].wYOffset;

            //
            // Add magic sprite to the sprite drawing sequence.
            //
            PAL_BattleAddSpriteObject(kBattleSpriteTypeMagic, iMagicNum, PAL_XY(x, y), sLayerOffset, FALSE);
         }
      }
      else if (gpGlobals->g.lprgMagic[iMagicNum].wType == kMagicTypeApplyToPlayer)
      {
         assert(sTarget != -1);

         x = PAL_X(g_Battle.rgPlayer[sTarget].pos);
         y = PAL_Y(g_Battle.rgPlayer[sTarget].pos);

         x += (short) gpGlobals->g.lprgMagic[iMagicNum].wXOffset;
         y += (short) gpGlobals->g.lprgMagic[iMagicNum].wYOffset;

         //
         // Add magic sprite to the sprite drawing sequence.
         //
         PAL_BattleAddSpriteObject(kBattleSpriteTypeMagic, iMagicNum, PAL_XY(x, y), sLayerOffset, FALSE);
      }
      else
      {
         assert(FALSE);
      }

      PAL_BattleMakeScene();
      VIDEO_CopyEntireSurface(g_Battle.lpSceneBuf, gpScreen);

      PAL_BattleUIUpdate();

      VIDEO_UpdateScreen(NULL);
   }

   free(lpSpriteEffect);

   for (i = 0; i < 6; i++)
   {
      if (gpGlobals->g.lprgMagic[iMagicNum].wType == kMagicTypeApplyToParty)
      {
         for (j = 0; j <= gpGlobals->wMaxPartyMemberIndex; j++)
         {
            g_Battle.rgPlayer[j].iColorShift = i;
         }
      }
      else
      {
         g_Battle.rgPlayer[sTarget].iColorShift = i;
      }

      PAL_BattleDelay(1, 0, TRUE);
   }

   for (i = 6; i >= 0; i--)
   {
      if (gpGlobals->g.lprgMagic[iMagicNum].wType == kMagicTypeApplyToParty)
      {
         for (j = 0; j <= gpGlobals->wMaxPartyMemberIndex; j++)
         {
            g_Battle.rgPlayer[j].iColorShift = i;
         }
      }
      else
      {
         g_Battle.rgPlayer[sTarget].iColorShift = i;
      }

      PAL_BattleDelay(1, 0, TRUE);
   }
}

static void
PAL_BattleShowPlayerOffMagicAnim(
   unsigned short         wPlayerIndex,
   unsigned short         wObjectID,
   short        sTarget,
   int         fSummon
)
/*++
  Purpose:

    Show the offensive magic animation for player.

  Parameters:

    [IN]  wPlayerIndex - the index of the player.

    [IN]  wObjectID - the object ID of the magic to be used.

    [IN]  sTarget - the target enemy of the action.

  Return value:

    None.

--*/
{
   unsigned char *   lpSpriteEffect;
   int        l, iMagicNum, iEffectNum, n, i, k, x, y, wave, blow;
   unsigned int      dwTime = SDL_GetTicks();
   short      sLayerOffset;

   iMagicNum = gpGlobals->g.rgObject[wObjectID].magic.wMagicNumber;
   iEffectNum = gpGlobals->g.lprgMagic[iMagicNum].wEffect;

   l = PAL_MKFGetDecompressedSize(iEffectNum, gpGlobals->f.fpFIRE);
   if (l <= 0)
   {
      return;
   }

   lpSpriteEffect = (unsigned char *)UTIL_malloc(l);

   PAL_MKFDecompressChunk((unsigned char *)lpSpriteEffect, l, iEffectNum, gpGlobals->f.fpFIRE);

   n = PAL_SpriteGetNumFrames(lpSpriteEffect);

   if (gConfig.fIsWIN95 && wPlayerIndex != (unsigned short)-1)
   {
      g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 6;
   }

   PAL_BattleDelay(1, 0, TRUE);

   l = n - gpGlobals->g.lprgMagic[iMagicNum].wFireDelay;
   l *= (short)gpGlobals->g.lprgMagic[iMagicNum].wEffectTimes;
   l += n;
   l += gpGlobals->g.lprgMagic[iMagicNum].wShake;

   wave = gpGlobals->wScreenWave;
   gpGlobals->wScreenWave += gpGlobals->g.lprgMagic[iMagicNum].wWave;

   if (gConfig.fIsWIN95 && !fSummon && gpGlobals->g.lprgMagic[iMagicNum].wSound != 0)
   {
      AUDIO_PlaySound(gpGlobals->g.lprgMagic[iMagicNum].wSound);
   }

   for (i = 0; i < l; i++)
   {
      const unsigned char* *b = &g_Battle.lpMagicBitmap;
	  if (!gConfig.fIsWIN95 && i == gpGlobals->g.lprgMagic[iMagicNum].wFireDelay && wPlayerIndex != (unsigned short)-1)
      {
         g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 6;
      }
      blow = ((g_Battle.iBlow > 0) ? RandomLong(0, g_Battle.iBlow) : RandomLong(g_Battle.iBlow, 0));

      for (k = 0; k <= g_Battle.wMaxEnemyIndex; k++)
      {
         if (g_Battle.rgEnemy[k].wObjectID == 0)
         {
            continue;
         }

         x = PAL_X(g_Battle.rgEnemy[k].pos) + blow;
         y = PAL_Y(g_Battle.rgEnemy[k].pos) + blow / 2;

         g_Battle.rgEnemy[k].pos = PAL_XY(x, y);
      }

      if (l - i > gpGlobals->g.lprgMagic[iMagicNum].wShake)
      {
         if (i < n)
         {
            k = i;
         }
         else
         {
            k = i - gpGlobals->g.lprgMagic[iMagicNum].wFireDelay;
            k %= n - gpGlobals->g.lprgMagic[iMagicNum].wFireDelay;
            k += gpGlobals->g.lprgMagic[iMagicNum].wFireDelay;
         }

         *b = PAL_SpriteGetFrame(lpSpriteEffect, k);

		 if (!gConfig.fIsWIN95 && (i - gpGlobals->g.lprgMagic[iMagicNum].wFireDelay) % n == 0)
         {
            AUDIO_PlaySound(gpGlobals->g.lprgMagic[iMagicNum].wSound);
         }
      }
      else
      {
         VIDEO_ShakeScreen(i, 3);
         *b = PAL_SpriteGetFrame(lpSpriteEffect, (l - gpGlobals->g.lprgMagic[iMagicNum].wShake - 1) % n);
      }
      //
      // Wait for the time of one frame. Accept input here.
      //
      PAL_DelayUntil(dwTime);

      //
      // Set the time of the next frame.
      //
      dwTime = SDL_GetTicks() +
         (gpGlobals->g.lprgMagic[iMagicNum].wSpeed + 5) * 10;

      //
      // Magic layers offset
      //
      sLayerOffset = (short)gpGlobals->g.lprgMagic[iMagicNum].rgSpecific.sLayerOffset;
      
      //
      // Unlocks the sprite sequence to add some image objects.
      //
      PAL_BattleSpriteAddUnlock();

      if (gpGlobals->g.lprgMagic[iMagicNum].wType == kMagicTypeNormal)
      {
         assert(sTarget != -1);

         x = PAL_X(g_Battle.rgEnemy[sTarget].pos);
         y = PAL_Y(g_Battle.rgEnemy[sTarget].pos);

         x += (short)gpGlobals->g.lprgMagic[iMagicNum].wXOffset;
         y += (short)gpGlobals->g.lprgMagic[iMagicNum].wYOffset;

         //
         // Add magic sprite to the sprite drawing sequence.
         //
         PAL_BattleAddSpriteObject(kBattleSpriteTypeMagic, iMagicNum, PAL_XY(x, y), sLayerOffset, FALSE);

         if (i == l - 1 && gpGlobals->wScreenWave < 9 &&
            gpGlobals->g.lprgMagic[iMagicNum].wKeepEffect == 0xFFFF)
         {
            PAL_RLEBlitToSurface(*b, g_Battle.lpBackground,
               PAL_XY(x - PAL_RLEGetWidth(*b) / 2, y - PAL_RLEGetHeight(*b)));
         }
      }
      else if (gpGlobals->g.lprgMagic[iMagicNum].wType == kMagicTypeAttackAll)
      {
         const int effectpos[MAX_BATTLE_MAGICSPRITE_ITEMS][2] = {{70, 140}, {100, 110}, {160, 100}};

         assert(sTarget == -1);

         for (k = 0; k < MAX_BATTLE_MAGICSPRITE_ITEMS; k++)
         {
            x = effectpos[k][0];
            y = effectpos[k][1];

            x += (short)gpGlobals->g.lprgMagic[iMagicNum].wXOffset;
            y += (short)gpGlobals->g.lprgMagic[iMagicNum].wYOffset;

            //
            // Add magic sprite to the sprite drawing sequence.
            //
            PAL_BattleAddSpriteObject(kBattleSpriteTypeMagic, iMagicNum, PAL_XY(x, y), sLayerOffset, FALSE);

            if (i == l - 1 && gpGlobals->wScreenWave < 9 &&
               gpGlobals->g.lprgMagic[iMagicNum].wKeepEffect == 0xFFFF)
            {
               PAL_RLEBlitToSurface(*b, g_Battle.lpBackground,
                  PAL_XY(x - PAL_RLEGetWidth(*b) / 2, y - PAL_RLEGetHeight(*b)));
            }
         }
      }
      else if (gpGlobals->g.lprgMagic[iMagicNum].wType == kMagicTypeAttackWhole ||
         gpGlobals->g.lprgMagic[iMagicNum].wType == kMagicTypeAttackField)
      {
         assert(sTarget == -1);

         if (gpGlobals->g.lprgMagic[iMagicNum].wType == kMagicTypeAttackWhole)
         {
            x = 120;
            y = 100;
         }
         else
         {
            x = 160;
            y = 200;
         }

         x += (short)gpGlobals->g.lprgMagic[iMagicNum].wXOffset;
         y += (short)gpGlobals->g.lprgMagic[iMagicNum].wYOffset;

         //
         // Add magic sprite to the sprite drawing sequence.
         //
         PAL_BattleAddSpriteObject(kBattleSpriteTypeMagic, iMagicNum, PAL_XY(x, y), sLayerOffset, FALSE);

         if (i == l - 1 && gpGlobals->wScreenWave < 9 &&
            gpGlobals->g.lprgMagic[iMagicNum].wKeepEffect == 0xFFFF)
         {
            PAL_RLEBlitToSurface(*b, g_Battle.lpBackground,
               PAL_XY(x - PAL_RLEGetWidth(*b) / 2, y - PAL_RLEGetHeight(*b)));
         }
      }
      else
      {
         assert(FALSE);
      }

      PAL_BattleMakeScene();
      VIDEO_CopyEntireSurface(g_Battle.lpSceneBuf, gpScreen);

      PAL_BattleUIUpdate();

      VIDEO_UpdateScreen(NULL);
   }

   gpGlobals->wScreenWave = wave;
   VIDEO_ShakeScreen(0, 0);

   free(lpSpriteEffect);

   for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
   {
      g_Battle.rgEnemy[i].pos = g_Battle.rgEnemy[i].posOriginal;
   }
}

static void
PAL_BattleShowEnemyMagicAnim(
   unsigned short         wEnemyIndex,
   unsigned short         wObjectID,
   short        sTarget
)
/*++
  Purpose:

    Show the offensive magic animation for enemy.

  Parameters:

    [IN]  wObjectID - the object ID of the magic to be used.

    [IN]  sTarget - the target player index of the action.

  Return value:

    None.

--*/
{
   unsigned char *   lpSpriteEffect;
   int        l, iMagicNum, iEffectNum, n, i, k, x, y, wave, blow;
   unsigned int      dwTime = SDL_GetTicks();
   short      sLayerOffset;

   iMagicNum = gpGlobals->g.rgObject[wObjectID].magic.wMagicNumber;
   iEffectNum = gpGlobals->g.lprgMagic[iMagicNum].wEffect;

   l = PAL_MKFGetDecompressedSize(iEffectNum, gpGlobals->f.fpFIRE);
   if (l <= 0)
   {
      return;
   }

   lpSpriteEffect = (unsigned char *)UTIL_malloc(l);

   PAL_MKFDecompressChunk((unsigned char *)lpSpriteEffect, l, iEffectNum, gpGlobals->f.fpFIRE);

   n = PAL_SpriteGetNumFrames(lpSpriteEffect);

   l = n - gpGlobals->g.lprgMagic[iMagicNum].wFireDelay;
   l *= (short)gpGlobals->g.lprgMagic[iMagicNum].wEffectTimes;
   l += n;
   l += gpGlobals->g.lprgMagic[iMagicNum].wShake;

   wave = gpGlobals->wScreenWave;
   gpGlobals->wScreenWave += gpGlobals->g.lprgMagic[iMagicNum].wWave;

   for (i = 0; i < l; i++)
   {
      const unsigned char* *b = &g_Battle.lpMagicBitmap;

      blow = ((g_Battle.iBlow > 0) ? RandomLong(0, g_Battle.iBlow) : RandomLong(g_Battle.iBlow, 0));

      for (k = 0; k <= gpGlobals->wMaxPartyMemberIndex; k++)
      {
         x = PAL_X(g_Battle.rgPlayer[k].pos) + blow;
         y = PAL_Y(g_Battle.rgPlayer[k].pos) + blow / 2;

         g_Battle.rgPlayer[k].pos = PAL_XY(x, y);
      }

      if (l - i > gpGlobals->g.lprgMagic[iMagicNum].wShake)
      {
         if (i < n)
         {
            k = i;
         }
         else
         {
            k = i - gpGlobals->g.lprgMagic[iMagicNum].wFireDelay;
            k %= n - gpGlobals->g.lprgMagic[iMagicNum].wFireDelay;
            k += gpGlobals->g.lprgMagic[iMagicNum].wFireDelay;
         }

         *b = PAL_SpriteGetFrame(lpSpriteEffect, k);

         if (i == (gConfig.fIsWIN95 ? 0 : gpGlobals->g.lprgMagic[iMagicNum].wFireDelay))
         {
            if(!gConfig.fIsWIN95 || g_Battle.rgEnemy[wEnemyIndex].e.wMagicSound >= 0)
            AUDIO_PlaySound(gpGlobals->g.lprgMagic[iMagicNum].wSound);
         }

         if (gpGlobals->g.lprgMagic[iMagicNum].wFireDelay > 0 &&
             i >= gpGlobals->g.lprgMagic[iMagicNum].wFireDelay &&
             i < gpGlobals->g.lprgMagic[iMagicNum].wFireDelay + g_Battle.rgEnemy[wEnemyIndex].e.wAttackFrames)
         {
             g_Battle.rgEnemy[wEnemyIndex].wCurrentFrame =
                i - gpGlobals->g.lprgMagic[iMagicNum].wFireDelay + g_Battle.rgEnemy[wEnemyIndex].e.wIdleFrames + g_Battle.rgEnemy[wEnemyIndex].e.wMagicFrames;
         }
      }
      else
      {
         VIDEO_ShakeScreen(i, 3);
         *b = PAL_SpriteGetFrame(lpSpriteEffect, (l - gpGlobals->g.lprgMagic[iMagicNum].wShake - 1) % n);
      }

      //
      // Wait for the time of one frame. Accept input here.
      //
      PAL_DelayUntil(dwTime);

      //
      // Set the time of the next frame.
      //
      dwTime = SDL_GetTicks() +
         (gpGlobals->g.lprgMagic[iMagicNum].wSpeed + 5) * 10;

      //
      // Magic layers offset
      //
      sLayerOffset = (short)gpGlobals->g.lprgMagic[iMagicNum].rgSpecific.sLayerOffset;
      
      //
      // Unlocks the sprite sequence to add some image objects.
      //
      PAL_BattleSpriteAddUnlock();

      if (gpGlobals->g.lprgMagic[iMagicNum].wType == kMagicTypeNormal)
      {
         assert(sTarget != -1);

         x = PAL_X(g_Battle.rgPlayer[sTarget].pos);
         y = PAL_Y(g_Battle.rgPlayer[sTarget].pos);

         x += (short)gpGlobals->g.lprgMagic[iMagicNum].wXOffset;
         y += (short)gpGlobals->g.lprgMagic[iMagicNum].wYOffset;

         //
         // Add magic sprite to the sprite drawing sequence.
         //
         PAL_BattleAddSpriteObject(kBattleSpriteTypeMagic, iMagicNum, PAL_XY(x, y), sLayerOffset, FALSE);

         if (i == l - 1 && gpGlobals->wScreenWave < 9 &&
            gpGlobals->g.lprgMagic[iMagicNum].wKeepEffect == 0xFFFF)
         {
            PAL_RLEBlitToSurface(*b, g_Battle.lpBackground,
               PAL_XY(x - PAL_RLEGetWidth(*b) / 2, y - PAL_RLEGetHeight(*b)));
         }
      }
      else if (gpGlobals->g.lprgMagic[iMagicNum].wType == kMagicTypeAttackAll)
      {
         const int effectpos[MAX_BATTLE_MAGICSPRITE_ITEMS][2] = {{180, 180}, {234, 170}, {270, 146}};

         assert(sTarget == -1);

         for (k = 0; k < MAX_BATTLE_MAGICSPRITE_ITEMS; k++)
         {
            x = effectpos[k][0];
            y = effectpos[k][1];

            x += (short)gpGlobals->g.lprgMagic[iMagicNum].wXOffset;
            y += (short)gpGlobals->g.lprgMagic[iMagicNum].wYOffset;

            //
            // Add magic sprite to the sprite drawing sequence.
            //
            PAL_BattleAddSpriteObject(kBattleSpriteTypeMagic, iMagicNum, PAL_XY(x, y), sLayerOffset, FALSE);

            if (i == l - 1 && gpGlobals->wScreenWave < 9 &&
               gpGlobals->g.lprgMagic[iMagicNum].wKeepEffect == 0xFFFF)
            {
               PAL_RLEBlitToSurface(*b, g_Battle.lpBackground,
                  PAL_XY(x - PAL_RLEGetWidth(*b) / 2, y - PAL_RLEGetHeight(*b)));
            }
         }
      }
      else if (gpGlobals->g.lprgMagic[iMagicNum].wType == kMagicTypeAttackWhole ||
         gpGlobals->g.lprgMagic[iMagicNum].wType == kMagicTypeAttackField)
      {
         assert(sTarget == -1);

         if (gpGlobals->g.lprgMagic[iMagicNum].wType == kMagicTypeAttackWhole)
         {
            x = 240;
            y = 150;
         }
         else
         {
            x = 160;
            y = 200;
         }

         x += (short)gpGlobals->g.lprgMagic[iMagicNum].wXOffset;
         y += (short)gpGlobals->g.lprgMagic[iMagicNum].wYOffset;

         //
         // Add magic sprite to the sprite drawing sequence.
         //
         PAL_BattleAddSpriteObject(kBattleSpriteTypeMagic, iMagicNum, PAL_XY(x, y), sLayerOffset, FALSE);

         if (i == l - 1 && gpGlobals->wScreenWave < 9 &&
            gpGlobals->g.lprgMagic[iMagicNum].wKeepEffect == 0xFFFF)
         {
            PAL_RLEBlitToSurface(*b, g_Battle.lpBackground,
               PAL_XY(x - PAL_RLEGetWidth(*b) / 2, y - PAL_RLEGetHeight(*b)));
         }
      }
      else
      {
         assert(FALSE);
      }

      PAL_BattleMakeScene();
      VIDEO_CopyEntireSurface(g_Battle.lpSceneBuf, gpScreen);

      PAL_BattleUIUpdate();

      VIDEO_UpdateScreen(NULL);
   }

   gpGlobals->wScreenWave = wave;
   VIDEO_ShakeScreen(0, 0);

   free(lpSpriteEffect);

   for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
   {
      g_Battle.rgPlayer[i].pos = g_Battle.rgPlayer[i].posOriginal;
   }
}

static void
PAL_BattleShowPlayerSummonMagicAnim(
   unsigned short         wPlayerIndex,
   unsigned short         wObjectID
)
/*++
  Purpose:

    Show the summon magic animation for player.

  Parameters:

    [IN]  wPlayerIndex - the index of the player.

    [IN]  wObjectID - the object ID of the magic to be used.

  Return value:

    None.

--*/
{
   int           i, j;
   unsigned short          wMagicNum = gpGlobals->g.rgObject[wObjectID].magic.wMagicNumber;
   unsigned short          wEffectMagicID = 0;
   unsigned int         dwTime = SDL_GetTicks();

   for (wEffectMagicID = 0; wEffectMagicID < MAX_OBJECTS; wEffectMagicID++)
   {
      if (gpGlobals->g.rgObject[wEffectMagicID].magic.wMagicNumber ==
         gpGlobals->g.lprgMagic[wMagicNum].wEffect)
      {
         break;
      }
   }

   assert(wEffectMagicID < MAX_OBJECTS);

   //
   // Sound should be played before magic begins
   //
   if (gConfig.fIsWIN95)
   {
	   AUDIO_PlaySound(gpGlobals->g.lprgMagic[wMagicNum].wSound);
   }

   //
   // Brighten the players
   //
   for (i = 1; i <= 10; i++)
   {
      for (j = 0; j <= gpGlobals->wMaxPartyMemberIndex; j++)
      {
         g_Battle.rgPlayer[j].iColorShift = i;
      }

      PAL_BattleDelay(1, wObjectID, TRUE);
   }

   VIDEO_BackupScreen(g_Battle.lpSceneBuf);

   //
   // Load the sprite of the summoned god
   //
   j = gpGlobals->g.lprgMagic[wMagicNum].rgSpecific.wSummonEffect + 10;
   i = PAL_MKFGetDecompressedSize(j, gpGlobals->f.fpF);

   if (g_Battle.lpSummonSprite)
      free(g_Battle.lpSummonSprite);
   g_Battle.lpSummonSprite = UTIL_malloc(i);

   PAL_MKFDecompressChunk(g_Battle.lpSummonSprite, i, j, gpGlobals->f.fpF);

   g_Battle.iSummonFrame = 0;
   g_Battle.posSummon = PAL_XY(240 + (short)(gpGlobals->g.lprgMagic[wMagicNum].wXOffset),
      165 + (short)(gpGlobals->g.lprgMagic[wMagicNum].wYOffset));
   g_Battle.sBackgroundColorShift = (short)(gpGlobals->g.lprgMagic[wMagicNum].wEffectTimes);
   g_Battle.fSummonColorShift = TRUE;

   //
   // Fade in the summoned god
   //
   PAL_BattleMakeScene();
   PAL_BattleFadeScene();

   g_Battle.fSummonColorShift = FALSE;

   //
   // Show the animation of the summoned god
   // TODO: There is still something missing here compared to the original game.
   //
   while (g_Battle.iSummonFrame < PAL_SpriteGetNumFrames(g_Battle.lpSummonSprite) - 1)
   {
      //
      // Wait for the time of one frame. Accept input here.
      //
      PAL_DelayUntil(dwTime);

      //
      // Set the time of the next frame.
      //
      dwTime = SDL_GetTicks() +
         (gpGlobals->g.lprgMagic[wMagicNum].wSpeed + 5) * 10;

      PAL_BattleMakeScene();
      VIDEO_CopyEntireSurface(g_Battle.lpSceneBuf, gpScreen);

      PAL_BattleUIUpdate();

      VIDEO_UpdateScreen(NULL);

      g_Battle.iSummonFrame++;
   }

   //
   // Show the actual magic effect
   //
   PAL_BattleShowPlayerOffMagicAnim((unsigned short)-1, wEffectMagicID, -1, TRUE);
}

static void
PAL_BattleShowPostMagicAnim(
   void
)
/*++
  Purpose:

    Show the post-magic animation.

  Parameters:

    None

  Return value:

    None.

--*/
{
   int         i, j, x, y, dist = 8;
   unsigned int     rgEnemyPosBak[MAX_ENEMIES_IN_TEAM];

   for (i = 0; i < MAX_ENEMIES_IN_TEAM; i++)
   {
      rgEnemyPosBak[i] = g_Battle.rgEnemy[i].pos;
   }

   for (i = 0; i < 3; i++)
   {
      for (j = 0; j <= g_Battle.wMaxEnemyIndex; j++)
      {
         if (g_Battle.rgEnemy[j].e.wHealth == g_Battle.rgEnemy[j].wPrevHP)
         {
            continue;
         }

         x = PAL_X(g_Battle.rgEnemy[j].pos);
         y = PAL_Y(g_Battle.rgEnemy[j].pos);

         x -= dist;
//         y -= dist / 2;

         g_Battle.rgEnemy[j].pos = PAL_XY(x, y);

         g_Battle.rgEnemy[j].iColorShift = ((i == 1) ? 6 : 0);
      }

      PAL_BattleDelay(1, 0, TRUE);
      dist /= -2;
   }

   for (i = 0; i < MAX_ENEMIES_IN_TEAM; i++)
   {
      g_Battle.rgEnemy[i].pos = rgEnemyPosBak[i];
   }

   PAL_BattleDelay(1, 0, TRUE);
}

static void
PAL_BattlePlayerValidateAction(
   unsigned short         wPlayerIndex
)
/*++
  Purpose:

    Validate player's action, fallback to other action when needed.

  Parameters:

    [IN]  wPlayerIndex - the index of the player.

  Return value:

    None.

--*/
{
   const unsigned short   wPlayerRole = gpGlobals->rgParty[wPlayerIndex].wPlayerRole;
   const unsigned short   wObjectID = g_Battle.rgPlayer[wPlayerIndex].action.wActionID;
   const short  sTarget = g_Battle.rgPlayer[wPlayerIndex].action.sTarget;
   int         fValid = TRUE, fToEnemy = FALSE;
   unsigned short         w;
   int          i;

   switch (g_Battle.rgPlayer[wPlayerIndex].action.ActionType)
   {
   case kBattleActionAttack:
      fToEnemy = TRUE;
      break;

   case kBattleActionPass:
      break;

   case kBattleActionDefend:
      break;

   case kBattleActionMagic:
      //
      // Make sure player actually has the magic to be used
      //
      for (i = 0; i < MAX_PLAYER_MAGICS; i++)
      {
         if (gpGlobals->g.PlayerRoles.rgwMagic[i][wPlayerRole] == wObjectID)
         {
            break; // player has this magic
         }
      }

      if (i >= MAX_PLAYER_MAGICS)
      {
         fValid = FALSE;
      }

      w = gpGlobals->g.rgObject[wObjectID].magic.wMagicNumber;

      if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSilence] > 0)
      {
         //
         // Player is silenced
         //
         fValid = FALSE;
      }

      if (gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole] <
         gpGlobals->g.lprgMagic[w].wCostMP)
      {
         //
         // No enough MP
         //
         fValid = FALSE;
      }

      //
      // Fallback to physical attack if player is using an offensive magic,
      // defend if player is using a defensive or healing magic
      //
      if (gpGlobals->g.rgObject[wObjectID].magic.wFlags & kMagicFlagUsableToEnemy)
      {
         if (!fValid)
         {
            g_Battle.rgPlayer[wPlayerIndex].action.ActionType = kBattleActionAttack;
            g_Battle.rgPlayer[wPlayerIndex].action.wActionID = 0;
         }
         else if (gpGlobals->g.rgObject[wObjectID].magic.wFlags & kMagicFlagApplyToAll)
         {
            g_Battle.rgPlayer[wPlayerIndex].action.sTarget = -1;
         }
         else if (sTarget == -1)
         {
            g_Battle.rgPlayer[wPlayerIndex].action.sTarget = PAL_BattleSelectAutoTargetFrom(g_Battle.rgPlayer[wPlayerIndex].action.sTarget);
         }

         fToEnemy = TRUE;
      }
      else
      {
         if (!fValid)
         {
            g_Battle.rgPlayer[wPlayerIndex].action.ActionType = kBattleActionDefend;
         }
         else if (gpGlobals->g.rgObject[wObjectID].magic.wFlags & kMagicFlagApplyToAll)
         {
            g_Battle.rgPlayer[wPlayerIndex].action.sTarget = -1;
         }
         else if (g_Battle.rgPlayer[wPlayerIndex].action.sTarget == -1)
         {
            g_Battle.rgPlayer[wPlayerIndex].action.sTarget = wPlayerIndex;
         }
      }
      break;

   case kBattleActionCoopMagic:
      fToEnemy = TRUE;

      {
         int iTotalHealthy = 0;
         for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
         {
            w = gpGlobals->rgParty[i].wPlayerRole;
            g_Battle.coopContributors[i] = PAL_IsPlayerHealthy(w);
            if( g_Battle.coopContributors[i] )
               iTotalHealthy ++;
         }
         if( iTotalHealthy <= 1 )
         {
            g_Battle.rgPlayer[wPlayerIndex].action.ActionType = kBattleActionAttack;
            g_Battle.rgPlayer[wPlayerIndex].action.wActionID = 0;
         }
      }

      if (g_Battle.rgPlayer[wPlayerIndex].action.ActionType == kBattleActionCoopMagic)
      {
         if (gpGlobals->g.rgObject[wObjectID].magic.wFlags & kMagicFlagApplyToAll)
         {
            g_Battle.rgPlayer[wPlayerIndex].action.sTarget = -1;
         }
         else if (sTarget == -1)
         {
            g_Battle.rgPlayer[wPlayerIndex].action.sTarget = PAL_BattleSelectAutoTargetFrom(g_Battle.rgPlayer[wPlayerIndex].action.sTarget);
         }
      }
      break;

   case kBattleActionFlee:
      break;

   case kBattleActionThrowItem:
      fToEnemy = TRUE;

      if (PAL_GetItemAmount(wObjectID) == 0)
      {
         g_Battle.rgPlayer[wPlayerIndex].action.ActionType = kBattleActionAttack;
         g_Battle.rgPlayer[wPlayerIndex].action.wActionID = 0;
      }
      else if (gpGlobals->g.rgObject[wObjectID].item.wFlags & kItemFlagApplyToAll)
      {
         g_Battle.rgPlayer[wPlayerIndex].action.sTarget = -1;
      }
      else if (g_Battle.rgPlayer[wPlayerIndex].action.sTarget == -1)
      {
         g_Battle.rgPlayer[wPlayerIndex].action.sTarget = PAL_BattleSelectAutoTargetFrom(g_Battle.rgPlayer[wPlayerIndex].action.sTarget);
      }
      break;

   case kBattleActionUseItem:
      if (PAL_GetItemAmount(wObjectID) == 0)
      {
         g_Battle.rgPlayer[wPlayerIndex].action.ActionType = kBattleActionDefend;
      }
      else if (gpGlobals->g.rgObject[wObjectID].item.wFlags & kItemFlagApplyToAll)
      {
         g_Battle.rgPlayer[wPlayerIndex].action.sTarget = -1;
      }
      else if (g_Battle.rgPlayer[wPlayerIndex].action.sTarget == -1)
      {
         g_Battle.rgPlayer[wPlayerIndex].action.sTarget = wPlayerIndex;
      }
      break;

   case kBattleActionAttackMate:
      if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusConfused] == 0)
      {
         //
         // Attack enemies instead if player is not confused
         //
         fToEnemy = TRUE;
         g_Battle.rgPlayer[wPlayerIndex].action.ActionType = kBattleActionAttack;
         g_Battle.rgPlayer[wPlayerIndex].action.wActionID = 0; //avoid be deduced to autoattack
      }
      else
      {
         for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
         {
            if (i != wPlayerIndex &&
               gpGlobals->g.PlayerRoles.rgwHP[gpGlobals->rgParty[i].wPlayerRole] != 0)
            {
               break;
            }
         }

         if (i > gpGlobals->wMaxPartyMemberIndex)
         {
            //
            // DISABLE Attack enemies if no one else is alive; since original version behaviour is not same
            //
//            fToEnemy = TRUE;
            g_Battle.rgPlayer[wPlayerIndex].action.ActionType = kBattleActionPass;
            g_Battle.rgPlayer[wPlayerIndex].action.wActionID = 0;
         }
      }
      break;
   }

   //
   // Check if player can attack all enemies at once, or attack one enemy
   //
   if (g_Battle.rgPlayer[wPlayerIndex].action.ActionType == kBattleActionAttack)
   {
      if (sTarget == -1)
      {
         if (!PAL_PlayerCanAttackAll(wPlayerRole))
         {
            g_Battle.rgPlayer[wPlayerIndex].action.sTarget = PAL_BattleSelectAutoTargetFrom(g_Battle.rgPlayer[wPlayerIndex].action.sTarget);
         }
      }
      else if (PAL_PlayerCanAttackAll(wPlayerRole))
      {
         g_Battle.rgPlayer[wPlayerIndex].action.sTarget = -1;
      }
   }

   if (fToEnemy && g_Battle.rgPlayer[wPlayerIndex].action.sTarget >= 0)
   {
      if (g_Battle.rgEnemy[g_Battle.rgPlayer[wPlayerIndex].action.sTarget].wObjectID == 0)
      {
         g_Battle.rgPlayer[wPlayerIndex].action.sTarget = PAL_BattleSelectAutoTargetFrom(g_Battle.rgPlayer[wPlayerIndex].action.sTarget);
         assert(g_Battle.rgPlayer[wPlayerIndex].action.sTarget >= 0);
      }
   }
}

static void
PAL_BattleCheckHidingEffect(
   void
)
/*++
  Purpose:

    Check if we should enter hiding state after using items or magics.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   if (g_Battle.iHidingTime < 0)
   {
      g_Battle.iHidingTime = -g_Battle.iHidingTime;
      VIDEO_BackupScreen(g_Battle.lpSceneBuf);
      PAL_BattleMakeScene();
      PAL_BattleFadeScene();
   }
}

int
FIGHT_DetectMagicTargetChange(
   unsigned short wMagicNum,
   int sTarget
)
{
   if(sTarget == -1 && (
                        gpGlobals->g.lprgMagic[wMagicNum].wType == kMagicTypeNormal
                        || gpGlobals->g.lprgMagic[wMagicNum].wType == kMagicTypeApplyToPlayer
                        || gpGlobals->g.lprgMagic[wMagicNum].wType == kMagicTypeTrance
                        ))
      sTarget = 0;
   
   if( sTarget != -1 && (
                         gpGlobals->g.lprgMagic[wMagicNum].wType == kMagicTypeAttackAll
                         || gpGlobals->g.lprgMagic[wMagicNum].wType == kMagicTypeAttackWhole
                         || gpGlobals->g.lprgMagic[wMagicNum].wType == kMagicTypeAttackField
                         || gpGlobals->g.lprgMagic[wMagicNum].wType == kMagicTypeApplyToParty
                         || gpGlobals->g.lprgMagic[wMagicNum].wType == kMagicTypeSummon
                         ))
      sTarget = -1;

   return sTarget;
}

void
PAL_BattlePlayerPerformAction(
   unsigned short         wPlayerIndex
)
/*++
  Purpose:

    Perform the selected action for a player.

  Parameters:

    [IN]  wPlayerIndex - the index of the player.

  Return value:

    None.

--*/
{
   short    sDamage;
   unsigned short     wPlayerRole = gpGlobals->rgParty[wPlayerIndex].wPlayerRole;
   short    sTarget;
   int      x, y;
   int      i, j, t;
   unsigned short     str, def, res, wObject, wMagicNum;
   int     fCritical;
   unsigned short     rgwCoopPos[3][2] = {{208, 157}, {234, 170}, {260, 183}};

   g_Battle.wMovingPlayerIndex = wPlayerIndex;
   g_Battle.iBlow = 0;

   short origTarget = g_Battle.rgPlayer[wPlayerIndex].action.sTarget;
   PAL_BattlePlayerValidateAction(wPlayerIndex);
   PAL_BattleBackupStat();

   sTarget = g_Battle.rgPlayer[wPlayerIndex].action.sTarget;

   switch (g_Battle.rgPlayer[wPlayerIndex].action.ActionType)
   {
   case kBattleActionAttack:
      if(g_Battle.fThisTurnCoop)
         break;

      if (sTarget != -1)
      {
         //
         // Attack one enemy
         //
         for (t = 0; t < (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusDualAttack] ? 2 : 1); t++)
         {
            str = PAL_GetPlayerAttackStrength(wPlayerRole);
            def = g_Battle.rgEnemy[sTarget].e.wDefense;
            def += (g_Battle.rgEnemy[sTarget].e.wLevel + 6) * 4;
            res = g_Battle.rgEnemy[sTarget].e.wPhysicalResistance;
            fCritical = FALSE;

            sDamage = PAL_CalcPhysicalAttackDamage(str, def, res);
            sDamage += RandomLong(1, 2);

            if (RandomLong(0, 5) == 0 ||
               gpGlobals->rgPlayerStatus[wPlayerRole][kStatusBravery] > 0)
            {
               //
               // Critical Hit
               //
               sDamage *= 3;
               fCritical = TRUE;
            }

            if (wPlayerRole == 0 && RandomLong(0, 11) == 0)
            {
               //
               // Bonus hit for Li Xiaoyao
               //
               sDamage *= 2;
               fCritical = TRUE;
            }

            sDamage = (short)(sDamage * RandomFloat(1, 1.125));

            if (sDamage <= 0)
            {
               sDamage = 1;
            }

            g_Battle.rgEnemy[sTarget].e.wHealth -= sDamage;

            if (t == 0)
            {
               g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 7;
               PAL_BattleDelay(4, 0, TRUE);
            }

            PAL_BattleShowPlayerAttackAnim(wPlayerIndex, fCritical);
         }
      }
      else
      {
         //
         // Attack all enemies
         //
         for (t = 0; t < (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusDualAttack] ? 2 : 1); t++)
         {
            int division = 1;
            const int index[MAX_ENEMIES_IN_TEAM] = {2, 1, 0, 4, 3};
            int x = 1;

            fCritical =
               (RandomLong(0, 5) == 0 || gpGlobals->rgPlayerStatus[wPlayerRole][kStatusBravery] > 0);

            if (t == 0)
            {
               g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 7;
               PAL_BattleDelay(4, 0, TRUE);
            }

            for (i = 0; i < MAX_ENEMIES_IN_TEAM; i++)
            {
               if (g_Battle.rgEnemy[index[i]].wObjectID == 0 ||
                  index[i] > g_Battle.wMaxEnemyIndex)
               {
                  continue;
               }

               str = PAL_GetPlayerAttackStrength(wPlayerRole);
               def = g_Battle.rgEnemy[index[i]].e.wDefense;
               def += (g_Battle.rgEnemy[index[i]].e.wLevel + 6) * 4;
               res = g_Battle.rgEnemy[index[i]].e.wPhysicalResistance;

               float sDamage = PAL_CalcPhysicalAttackDamage(str, def, res);

               if (fCritical)
               {
                  //
                  // Critical Hit
                  //
                  sDamage *= 3;
               }

               sDamage /= division;

               if (sDamage <= 0)
               {
                  sDamage = 1;
               }

               g_Battle.rgEnemy[index[i]].e.wHealth -= (unsigned short)sDamage;

               if (g_Battle.rgEnemy[index[i]].wObjectID != 0)
                    division *= 2;
            }

            if (t > 0)
            {
                if (x == 1)
                {
                    g_Battle.rgPlayer[wPlayerIndex].fSecondAttack = TRUE;
                    x--;
                }
                else
                {
                    g_Battle.rgPlayer[wPlayerIndex].fSecondAttack = FALSE;
                }
            }

            PAL_BattleShowPlayerAttackAnim(wPlayerIndex, fCritical);

            PAL_BattleDelay(4, 0, TRUE);
         }
      }

      g_Battle.rgPlayer[wPlayerIndex].fSecondAttack = FALSE;
      PAL_BattleUpdateFighters();
      PAL_BattleMakeScene();
      PAL_BattleDelay(3, 0, TRUE);

      gpGlobals->Exp.rgAttackExp[wPlayerRole].wCount++;
      gpGlobals->Exp.rgHealthExp[wPlayerRole].wCount += RandomLong(2, 3);
      break;

   case kBattleActionAttackMate:
      if(g_Battle.fThisTurnCoop)
         break;
      //
      // Check if there is someone else who is alive
      //
      for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
      {
         if (i == wPlayerIndex)
         {
            continue;
         }

         if (gpGlobals->g.PlayerRoles.rgwHP[gpGlobals->rgParty[i].wPlayerRole] > 0)
         {
            break;
         }
      }

      if (i <= gpGlobals->wMaxPartyMemberIndex)
      {
         //
         // Pick a target randomly
         //
         do
         {
            sTarget = RandomLong(0, gpGlobals->wMaxPartyMemberIndex);
         } while (sTarget == wPlayerIndex || gpGlobals->g.PlayerRoles.rgwHP[gpGlobals->rgParty[sTarget].wPlayerRole] == 0);

         for (j = 0; j < 2; j++)
         {
            g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 8;
            PAL_BattleDelay(1, 0, TRUE);

            g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 0;
            PAL_BattleDelay(1, 0, TRUE);
         }

         PAL_BattleDelay(2, 0, TRUE);

         x = PAL_X(g_Battle.rgPlayer[sTarget].pos) + 30;
         y = PAL_Y(g_Battle.rgPlayer[sTarget].pos) + 12;

         g_Battle.rgPlayer[wPlayerIndex].pos = PAL_XY(x, y);
         g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 8;
         PAL_BattleDelay(5, 0, TRUE);

         g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 9;
         AUDIO_PlaySound(gpGlobals->g.PlayerRoles.rgwWeaponSound[wPlayerRole]);

         str = PAL_GetPlayerAttackStrength(wPlayerRole);
         def = PAL_GetPlayerDefense(gpGlobals->rgParty[sTarget].wPlayerRole);
         if (g_Battle.rgPlayer[sTarget].fDefending)
         {
            def *= 2;
         }

         sDamage = PAL_CalcPhysicalAttackDamage(str, def, 2);
         if (gpGlobals->rgPlayerStatus[gpGlobals->rgParty[sTarget].wPlayerRole][kStatusProtect] > 0)
         {
            sDamage /= 2;
         }

         if (sDamage <= 0)
         {
            sDamage = 1;
         }

         if (sDamage > (short)gpGlobals->g.PlayerRoles.rgwHP[gpGlobals->rgParty[sTarget].wPlayerRole])
         {
            sDamage = gpGlobals->g.PlayerRoles.rgwHP[gpGlobals->rgParty[sTarget].wPlayerRole];
         }

         gpGlobals->g.PlayerRoles.rgwHP[gpGlobals->rgParty[sTarget].wPlayerRole] -= sDamage;

         g_Battle.rgPlayer[sTarget].pos =
            PAL_XY(PAL_X(g_Battle.rgPlayer[sTarget].pos) - 12,
                   PAL_Y(g_Battle.rgPlayer[sTarget].pos) - 6);
         PAL_BattleDelay(1, 0, TRUE);

         g_Battle.rgPlayer[sTarget].iColorShift = 6;
         PAL_BattleDelay(1, 0, TRUE);

         PAL_BattleDisplayStatChange();

         g_Battle.rgPlayer[sTarget].iColorShift = 0;
         PAL_BattleDelay(4, 0, TRUE);

         PAL_BattleUpdateFighters();
         PAL_BattleDelay(4, 0, TRUE);
      }

      break;

   case kBattleActionCoopMagic:
      g_Battle.fThisTurnCoop = TRUE;
      wObject = PAL_GetPlayerCooperativeMagic(gpGlobals->rgParty[wPlayerIndex].wPlayerRole);
      wMagicNum = gpGlobals->g.rgObject[wObject].magic.wMagicNumber;

      sTarget = FIGHT_DetectMagicTargetChange(wMagicNum, sTarget);

      if (gpGlobals->g.lprgMagic[wMagicNum].wType == kMagicTypeSummon)
      {
         PAL_BattleShowPlayerPreMagicAnim(wPlayerIndex, TRUE);
         PAL_BattleShowPlayerSummonMagicAnim((unsigned short)-1, wObject);
      }
      else
      {
         //
         // Sound should be played before action begins
         //
         AUDIO_PlaySound(29);

         for (i = 1; i <= 6; i++)
         {
            //
            // Update the position for the player who invoked the action
            //
            x = PAL_X(g_Battle.rgPlayer[wPlayerIndex].posOriginal) * (6 - i);
            y = PAL_Y(g_Battle.rgPlayer[wPlayerIndex].posOriginal) * (6 - i);

            x += rgwCoopPos[0][0] * i;
            y += rgwCoopPos[0][1] * i;

            x /= 6;
            y /= 6;

            g_Battle.rgPlayer[wPlayerIndex].pos = PAL_XY(x, y);

            //
            // Update the position for other players
            //
            t = 0;

            for (j = 0; j <= gpGlobals->wMaxPartyMemberIndex; j++)
            {
               if ((unsigned short)j == wPlayerIndex)
               {
                  continue;
               }

               t++;

               if( g_Battle.coopContributors[j] == FALSE )
                  continue;

               x = PAL_X(g_Battle.rgPlayer[j].posOriginal) * (6 - i);
               y = PAL_Y(g_Battle.rgPlayer[j].posOriginal) * (6 - i);

               x += rgwCoopPos[t][0] * i;
               y += rgwCoopPos[t][1] * i;

               x /= 6;
               y /= 6;

               g_Battle.rgPlayer[j].pos = PAL_XY(x, y);
            }

            PAL_BattleDelay(1, 0, TRUE);
         }

         for (i = gpGlobals->wMaxPartyMemberIndex; i >= 0; i--)
         {
            if ((unsigned short)i == wPlayerIndex)
            {
               continue;
            }
            if( g_Battle.coopContributors[i] == FALSE )
               continue;

            g_Battle.rgPlayer[i].wCurrentFrame = 5;

            PAL_BattleDelay(3, 0, TRUE);
         }

         g_Battle.rgPlayer[wPlayerIndex].iColorShift = 6;
         g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 5;
         PAL_BattleDelay(5, 0, TRUE);

         g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 6;
         g_Battle.rgPlayer[wPlayerIndex].iColorShift = 0;
         PAL_BattleDelay(3, 0, TRUE);

         PAL_BattleShowPlayerOffMagicAnim((unsigned short)-1, wObject, sTarget, FALSE);
      }

      for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
      {
         if( g_Battle.coopContributors[i] == FALSE )
            continue;

         gpGlobals->g.PlayerRoles.rgwHP[gpGlobals->rgParty[i].wPlayerRole] -=
            gpGlobals->g.lprgMagic[wMagicNum].wCostMP;

         if ((short)(gpGlobals->g.PlayerRoles.rgwHP[gpGlobals->rgParty[i].wPlayerRole]) <= 0)
         {
            gpGlobals->g.PlayerRoles.rgwHP[gpGlobals->rgParty[i].wPlayerRole] = 1;
         }

         //
         // Reset the time meter for everyone when using coopmagic
         //
         g_Battle.rgPlayer[i].state = kFighterWait;
      }

      PAL_BattleBackupStat(); // so that "damages" to players won't be shown

      str = 0;

      for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
      {
         if( g_Battle.coopContributors[i] == FALSE )
            continue;

         str += PAL_GetPlayerAttackStrength(gpGlobals->rgParty[i].wPlayerRole);
         str += PAL_GetPlayerMagicStrength(gpGlobals->rgParty[i].wPlayerRole);
      }

      str /= 4;

      //
      // Inflict damage to enemies
      //
      if (sTarget == -1)
      {
         //
         // Attack all enemies
         //
         for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
         {
            if (g_Battle.rgEnemy[i].wObjectID == 0)
            {
               continue;
            }

            def = g_Battle.rgEnemy[i].e.wDefense;
            def += (g_Battle.rgEnemy[i].e.wLevel + 6) * 4;

            sDamage = PAL_CalcMagicDamage(str, def,
               g_Battle.rgEnemy[i].e.wElemResistance, g_Battle.rgEnemy[i].e.wPoisonResistance, 1, wObject);

            if (sDamage <= 0)
            {
               sDamage = 1;
            }

            g_Battle.rgEnemy[i].e.wHealth -= sDamage;
         }
      }
      else
      {
         //
         // Attack one enemy
         //
         def = g_Battle.rgEnemy[sTarget].e.wDefense;
         def += (g_Battle.rgEnemy[sTarget].e.wLevel + 6) * 4;

         sDamage = PAL_CalcMagicDamage(str, def,
            g_Battle.rgEnemy[sTarget].e.wElemResistance, g_Battle.rgEnemy[sTarget].e.wPoisonResistance, 1,  wObject);

         if (sDamage <= 0)
         {
            sDamage = 1;
         }

         g_Battle.rgEnemy[sTarget].e.wHealth -= sDamage;
      }

      PAL_BattleDisplayStatChange();
      PAL_BattleShowPostMagicAnim();
      PAL_BattleDelay(5, 0, TRUE);

      if (gpGlobals->g.lprgMagic[wMagicNum].wType != kMagicTypeSummon)
      {
         PAL_BattlePostActionCheck(FALSE);

         //
         // Move all players back to the original position
         //
         for (i = 1; i <= 6; i++)
         {
            //
            // Update the position for the player who invoked the action
            //
            x = PAL_X(g_Battle.rgPlayer[wPlayerIndex].posOriginal) * i;
            y = PAL_Y(g_Battle.rgPlayer[wPlayerIndex].posOriginal) * i;

            x += rgwCoopPos[0][0] * (6 - i);
            y += rgwCoopPos[0][1] * (6 - i);

            x /= 6;
            y /= 6;

            g_Battle.rgPlayer[wPlayerIndex].pos = PAL_XY(x, y);

            //
            // Update the position for other players
            //
            t = 0;

            for (j = 0; j <= gpGlobals->wMaxPartyMemberIndex; j++)
            {
               if( g_Battle.coopContributors[j] == FALSE )
                  continue;

               g_Battle.rgPlayer[j].wCurrentFrame = 0;

               if ((unsigned short)j == wPlayerIndex)
               {
                  continue;
               }

               t++;

               x = PAL_X(g_Battle.rgPlayer[j].posOriginal) * i;
               y = PAL_Y(g_Battle.rgPlayer[j].posOriginal) * i;

               x += rgwCoopPos[t][0] * (6 - i);
               y += rgwCoopPos[t][1] * (6 - i);

               x /= 6;
               y /= 6;

               g_Battle.rgPlayer[j].pos = PAL_XY(x, y);
            }

            PAL_BattleDelay(1, 0, TRUE);
         }
      }
      break;

   case kBattleActionDefend:
      if(g_Battle.fThisTurnCoop)
         break;

      g_Battle.rgPlayer[wPlayerIndex].fDefending = TRUE;
      gpGlobals->Exp.rgDefenseExp[wPlayerRole].wCount += 2;
      break;

   case kBattleActionFlee:
      if(g_Battle.fThisTurnCoop)
         break;

      str = PAL_GetPlayerFleeRate(wPlayerRole);
      def = 0;

      for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
      {
         if (g_Battle.rgEnemy[i].wObjectID == 0)
         {
            continue;
         }

         def += (short)(g_Battle.rgEnemy[i].e.wDexterity);
         def += (g_Battle.rgEnemy[i].e.wLevel + 6) * 4;
      }

      if ((short)def < 0)
      {
         def = 0;
      }

      if (str >= RandomLong(0, def) && !g_Battle.fIsBoss)
      {
         //
         // Successful escape
         //
         PAL_BattlePlayerEscape();
      }
      else
      {
         //
         // Failed escape
         //
         g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 0;

         for (i = 0; i < 3; i++)
         {
            x = PAL_X(g_Battle.rgPlayer[wPlayerIndex].pos) + 4;
            y = PAL_Y(g_Battle.rgPlayer[wPlayerIndex].pos) + 2;

            g_Battle.rgPlayer[wPlayerIndex].pos = PAL_XY(x, y);

            PAL_BattleDelay(1, 0, TRUE);
         }

         g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 1;
         PAL_BattleDelay(8, BATTLE_LABEL_ESCAPEFAIL, TRUE);

         gpGlobals->Exp.rgFleeExp[wPlayerRole].wCount += 2;
      }
      break;

   case kBattleActionMagic:
      if(g_Battle.fThisTurnCoop)
         break;

      wObject = g_Battle.rgPlayer[wPlayerIndex].action.wActionID;
      wMagicNum = gpGlobals->g.rgObject[wObject].magic.wMagicNumber;

      sTarget = FIGHT_DetectMagicTargetChange(wMagicNum, sTarget);

      PAL_BattleShowPlayerPreMagicAnim(wPlayerIndex,
         (gpGlobals->g.lprgMagic[wMagicNum].wType == kMagicTypeSummon));

      if (!gpGlobals->fAutoBattle)
      {
         gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole] -= gpGlobals->g.lprgMagic[wMagicNum].wCostMP;
         if ((short)(gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole]) < 0)
         {
            gpGlobals->g.PlayerRoles.rgwMP[wPlayerRole] = 0;
         }
      }

      if (gpGlobals->g.lprgMagic[wMagicNum].wType == kMagicTypeApplyToPlayer ||
         gpGlobals->g.lprgMagic[wMagicNum].wType == kMagicTypeApplyToParty ||
         gpGlobals->g.lprgMagic[wMagicNum].wType == kMagicTypeTrance)
      {
         //
         // Using a defensive magic
         //
         unsigned short w = 0;

         if (g_Battle.rgPlayer[wPlayerIndex].action.sTarget != -1)
         {
            w = gpGlobals->rgParty[g_Battle.rgPlayer[wPlayerIndex].action.sTarget].wPlayerRole;
         }
         else if (gpGlobals->g.lprgMagic[wMagicNum].wType == kMagicTypeTrance)
         {
            w = wPlayerRole;
         }

         gpGlobals->g.rgObject[wObject].magic.wScriptOnUse =
            PAL_RunTriggerScript(gpGlobals->g.rgObject[wObject].magic.wScriptOnUse, wPlayerRole);

         if (g_fScriptSuccess)
         {
            PAL_BattleShowPlayerDefMagicAnim(wPlayerIndex, wObject, sTarget);

            gpGlobals->g.rgObject[wObject].magic.wScriptOnSuccess =
               PAL_RunTriggerScript(gpGlobals->g.rgObject[wObject].magic.wScriptOnSuccess, w);

            if (g_fScriptSuccess)
            {
               if (gpGlobals->g.lprgMagic[wMagicNum].wType == kMagicTypeTrance)
               {
                  for (i = 0; i < 6; i++)
                  {
                     g_Battle.rgPlayer[wPlayerIndex].iColorShift = i * 2;
                     PAL_BattleDelay(1, 0, TRUE);
                  }

                  VIDEO_BackupScreen(g_Battle.lpSceneBuf);
                  PAL_LoadBattleSprites();

                  g_Battle.rgPlayer[wPlayerIndex].iColorShift = 0;

                  PAL_BattleMakeScene();
                  PAL_BattleFadeScene();
               }
            }
         }
      }
      else
      {
         //
         // Using an offensive magic
         //
         gpGlobals->g.rgObject[wObject].magic.wScriptOnUse =
            PAL_RunTriggerScript(gpGlobals->g.rgObject[wObject].magic.wScriptOnUse, wPlayerRole);

         if (g_fScriptSuccess)
         {
            if (gpGlobals->g.lprgMagic[wMagicNum].wType == kMagicTypeSummon)
            {
               PAL_BattleShowPlayerSummonMagicAnim(wPlayerIndex, wObject);
            }
            else
            {
               PAL_BattleShowPlayerOffMagicAnim(wPlayerIndex, wObject, sTarget, FALSE);
            }

            gpGlobals->g.rgObject[wObject].magic.wScriptOnSuccess =
               PAL_RunTriggerScript(gpGlobals->g.rgObject[wObject].magic.wScriptOnSuccess, (unsigned short)sTarget);

            //
            // Inflict damage to enemies
            //
            if ((short)(gpGlobals->g.lprgMagic[wMagicNum].wBaseDamage) > 0)
            {
               if (sTarget == -1)
               {
                  //
                  // Attack all enemies
                  //
                  for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
                  {
                     if (g_Battle.rgEnemy[i].wObjectID == 0)
                     {
                        continue;
                     }

                     str = PAL_GetPlayerMagicStrength(wPlayerRole);
                     def = g_Battle.rgEnemy[i].e.wDefense;
                     def += (g_Battle.rgEnemy[i].e.wLevel + 6) * 4;

                     sDamage = PAL_CalcMagicDamage(str, def,
                        g_Battle.rgEnemy[i].e.wElemResistance, g_Battle.rgEnemy[i].e.wPoisonResistance, 1, wObject);

                     if (sDamage <= 0)
                     {
                        sDamage = 1;
                     }

                     g_Battle.rgEnemy[i].e.wHealth -= sDamage;
                  }
               }
               else
               {
                  //
                  // Attack one enemy
                  //
                  str = PAL_GetPlayerMagicStrength(wPlayerRole);
                  def = g_Battle.rgEnemy[sTarget].e.wDefense;
                  def += (g_Battle.rgEnemy[sTarget].e.wLevel + 6) * 4;

                  sDamage = PAL_CalcMagicDamage(str, def,
                     g_Battle.rgEnemy[sTarget].e.wElemResistance, g_Battle.rgEnemy[sTarget].e.wPoisonResistance, 1, wObject);

                  if (sDamage <= 0)
                  {
                     sDamage = 1;
                  }

                  g_Battle.rgEnemy[sTarget].e.wHealth -= sDamage;
               }
            }
         }
      }

      PAL_BattleDisplayStatChange();
      PAL_BattleShowPostMagicAnim();
      PAL_BattleDelay(5, 0, TRUE);

      PAL_BattleCheckHidingEffect();

      gpGlobals->Exp.rgMagicExp[wPlayerRole].wCount += RandomLong(2, 3);
      gpGlobals->Exp.rgMagicPowerExp[wPlayerRole].wCount++;
      break;

   case kBattleActionThrowItem:
      if(g_Battle.fThisTurnCoop)
         break;
      wObject = g_Battle.rgPlayer[wPlayerIndex].action.wActionID;

      for (i = 0; i < 4; i++)
      {
         g_Battle.rgPlayer[wPlayerIndex].pos =
            PAL_XY(PAL_X(g_Battle.rgPlayer[wPlayerIndex].pos) - (4 - i),
                   PAL_Y(g_Battle.rgPlayer[wPlayerIndex].pos) - (4 - i) / 2);

         PAL_BattleDelay(1, 0, TRUE);
      }

      PAL_BattleDelay(2, wObject, TRUE);

      g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 5;
      AUDIO_PlaySound(gpGlobals->g.PlayerRoles.rgwMagicSound[wPlayerRole]);

      PAL_BattleDelay(8, wObject, TRUE);

      g_Battle.rgPlayer[wPlayerIndex].wCurrentFrame = 6;
      PAL_BattleDelay(2, wObject, TRUE);

      //
      // Run the script
      //
      gpGlobals->g.rgObject[wObject].item.wScriptOnThrow =
         PAL_RunTriggerScript(gpGlobals->g.rgObject[wObject].item.wScriptOnThrow, (unsigned short)sTarget);

      //
      // Remove the thrown item from inventory
      //
      PAL_AddItemToInventory(wObject, -1);

      PAL_BattleDisplayStatChange();
      PAL_BattleDelay(4, 0, TRUE);
      PAL_BattleUpdateFighters();
      PAL_BattleDelay(4, 0, TRUE);

      PAL_BattleCheckHidingEffect();

      break;

   case kBattleActionUseItem:
      if(g_Battle.fThisTurnCoop)
         break;

      wObject = g_Battle.rgPlayer[wPlayerIndex].action.wActionID;

      PAL_BattleShowPlayerUseItemAnim(wPlayerIndex, wObject, sTarget);

      //
      // Run the script
      //
      gpGlobals->g.rgObject[wObject].item.wScriptOnUse =
         PAL_RunTriggerScript(gpGlobals->g.rgObject[wObject].item.wScriptOnUse,
            (sTarget == -1) ? 0xFFFF : gpGlobals->rgParty[sTarget].wPlayerRole);

      //
      // Remove the item if the item is consuming
      //
      if (gpGlobals->g.rgObject[wObject].item.wFlags & kItemFlagConsuming)
      {
         PAL_AddItemToInventory(wObject, -1);
      }

      PAL_BattleCheckHidingEffect();

      PAL_BattleUpdateFighters();
      PAL_BattleDisplayStatChange();
      PAL_BattleDelay(8, 0, TRUE);
      break;

   case kBattleActionPass:
      break;
   }

   //
   // Revert this player back to waiting state.
   //
   g_Battle.rgPlayer[wPlayerIndex].state = kFighterWait;
   g_Battle.rgPlayer[wPlayerIndex].flTimeMeter = 0;

   PAL_BattlePostActionCheck(FALSE);
   
   //
   // Revert target slot of this player 
   //
   g_Battle.rgPlayer[wPlayerIndex].action.sTarget = origTarget;

}

static int
PAL_BattleEnemySelectEnemyTargetIndex(
   void
)
/*++
 Purpose:

 Select a attackable enemy randomly.

 Parameters:

 None.

 Return value:

 None.

 --*/
{
   int i;

   i = RandomLong(0, g_Battle.wMaxEnemyIndex);

   while (g_Battle.rgEnemy[i].wObjectID == 0 || g_Battle.rgEnemy[i].e.wHealth == 0)
   {
      i = RandomLong(0, g_Battle.wMaxEnemyIndex);
   }

   return i;
}

static int
PAL_BattleEnemySelectTargetIndex(
   void
)
/*++
  Purpose:

    Select a attackable player randomly.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   int i;

   i = RandomLong(0, gpGlobals->wMaxPartyMemberIndex);

   while (gpGlobals->g.PlayerRoles.rgwHP[gpGlobals->rgParty[i].wPlayerRole] == 0)
   {
      i = RandomLong(0, gpGlobals->wMaxPartyMemberIndex);
   }

   return i;
}

void
PAL_BattleEnemyPerformAction(
   unsigned short         wEnemyIndex
)
/*++
  Purpose:

    Perform the selected action for a player.

  Parameters:

    [IN]  wEnemyIndex - the index of the player.

  Return value:

    None.

--*/
{
   int        str, def, iCoverIndex, i, x, y, ex, ey, iSound;
   unsigned short       rgwElementalResistance[NUM_MAGIC_ELEMENTAL];
   unsigned short       wPlayerRole, w, wMagic, wMagicNum;
   short      sTarget, sDamage;
   int       fAutoDefend = FALSE, rgfMagAutoDefend[MAX_PLAYERS_IN_PARTY];

   PAL_BattleBackupStat();
   g_Battle.iBlow = 0;

   sTarget = PAL_BattleEnemySelectTargetIndex();
   wPlayerRole = gpGlobals->rgParty[sTarget].wPlayerRole;
   wMagic = g_Battle.rgEnemy[wEnemyIndex].e.wMagic;

   if (g_Battle.rgEnemy[wEnemyIndex].rgwStatus[kStatusSleep] > 0 ||
      g_Battle.rgEnemy[wEnemyIndex].rgwStatus[kStatusParalyzed] > 0 ||
      g_Battle.iHidingTime > 0)
   {
      //
      // Do nothing
      //
      goto end;
   }
   else if (g_Battle.rgEnemy[wEnemyIndex].rgwStatus[kStatusConfused] > 0)
   {
      int  iTarget = PAL_BattleEnemySelectEnemyTargetIndex();
      if( iTarget == wEnemyIndex )
         goto end;
      int  iX = PAL_X(g_Battle.rgEnemy[iTarget].pos);
      int  iY = PAL_Y(g_Battle.rgEnemy[iTarget].pos);
      for (i = 0; i < 3; i++)
      {
         x = PAL_X(g_Battle.rgEnemy[wEnemyIndex].pos);
         y = PAL_Y(g_Battle.rgEnemy[wEnemyIndex].pos);

         x += iX;
         y += iY;

         x /= 2;
         y /= 2;

         g_Battle.rgEnemy[wEnemyIndex].pos = PAL_XY(x, y);

         PAL_BattleDelay(1, 0, TRUE);
      }

      unsigned int dwTime = SDL_GetTicks() + BATTLE_FRAME_TIME;
      x = (PAL_X(g_Battle.rgEnemy[wEnemyIndex].pos) + PAL_X(g_Battle.rgEnemy[iTarget].pos)) / 2;
      y = PAL_Y(g_Battle.rgEnemy[iTarget].pos) - PAL_RLEGetHeight(PAL_SpriteGetFrame(g_Battle.rgEnemy[iTarget].lpSprite, 0)) / 3 + 10;
      for( i=9; i<12; i++ )
      {
         const unsigned char* b = PAL_SpriteGetFrame(g_Battle.lpEffectSprite, i);

         PAL_DelayUntil(dwTime);
         dwTime = SDL_GetTicks() + BATTLE_FRAME_TIME;

         PAL_BattleMakeScene();
         VIDEO_CopyEntireSurface(g_Battle.lpSceneBuf, gpScreen);

         PAL_RLEBlitToSurface(b, gpScreen, PAL_XY(x - PAL_RLEGetWidth(b) / 2, y - PAL_RLEGetHeight(b)));

         PAL_BattleUIUpdate();

         VIDEO_UpdateScreen(NULL);
      }

      int str = (short)g_Battle.rgEnemy[wEnemyIndex].e.wAttackStrength;
      str += (g_Battle.rgEnemy[wEnemyIndex].e.wLevel + 6) * 6;
      int def = (short)g_Battle.rgEnemy[iTarget].e.wDefense;
      def += (g_Battle.rgEnemy[iTarget].e.wLevel + 6) * 4;
      sDamage = PAL_CalcBaseDamage(str, def)*2/g_Battle.rgEnemy[iTarget].e.wPhysicalResistance;

      if (sDamage <= 0)
      {
         sDamage = 1;
      }

      g_Battle.rgEnemy[iTarget].e.wHealth -= sDamage;

      PAL_BattleDisplayStatChange();
      PAL_BattleShowPostMagicAnim();
      PAL_BattleDelay(5, 0, TRUE);

      g_Battle.rgEnemy[wEnemyIndex].pos = g_Battle.rgEnemy[wEnemyIndex].posOriginal;
      PAL_BattleDelay(2, 0, TRUE);

      PAL_BattlePostActionCheck(FALSE);
   }
   else if (wMagic != 0 &&
      RandomLong(0, 9) < g_Battle.rgEnemy[wEnemyIndex].e.wMagicRate &&
      g_Battle.rgEnemy[wEnemyIndex].rgwStatus[kStatusSilence] == 0)
   {
      //
      // Magical attack
      //
      if (wMagic == 0xFFFF)
      {
         //
         // Do nothing
         //
         goto end;
      }

      wMagicNum = gpGlobals->g.rgObject[wMagic].magic.wMagicNumber;

      str = (short)g_Battle.rgEnemy[wEnemyIndex].e.wMagicStrength;
      str += (g_Battle.rgEnemy[wEnemyIndex].e.wLevel + 6) * 6;
      if (str < 0)
      {
         str = 0;
      }

      ex = PAL_X(g_Battle.rgEnemy[wEnemyIndex].pos);
      ey = PAL_Y(g_Battle.rgEnemy[wEnemyIndex].pos);

      ex += 12;
      ey += 6;

      g_Battle.rgEnemy[wEnemyIndex].pos = PAL_XY(ex, ey);
      PAL_BattleDelay(1, 0, FALSE);

      ex += 4;
      ey += 2;

      g_Battle.rgEnemy[wEnemyIndex].pos = PAL_XY(ex, ey);
      PAL_BattleDelay(1, 0, FALSE);

      AUDIO_PlaySound(g_Battle.rgEnemy[wEnemyIndex].e.wMagicSound);

      for (i = 0; i < g_Battle.rgEnemy[wEnemyIndex].e.wMagicFrames; i++)
      {
         g_Battle.rgEnemy[wEnemyIndex].wCurrentFrame =
            g_Battle.rgEnemy[wEnemyIndex].e.wIdleFrames + i;
         PAL_BattleDelay(g_Battle.rgEnemy[wEnemyIndex].e.wActWaitFrames, 0, FALSE);
      }

      if (g_Battle.rgEnemy[wEnemyIndex].e.wMagicFrames == 0)
      {
         PAL_BattleDelay(1, 0, FALSE);
      }

      if (gpGlobals->g.lprgMagic[wMagicNum].wFireDelay == 0)
      {
         for (i = 0; i <= g_Battle.rgEnemy[wEnemyIndex].e.wAttackFrames; i++)
         {
            g_Battle.rgEnemy[wEnemyIndex].wCurrentFrame =
               i - 1 + g_Battle.rgEnemy[wEnemyIndex].e.wIdleFrames + g_Battle.rgEnemy[wEnemyIndex].e.wMagicFrames;
            PAL_BattleDelay(g_Battle.rgEnemy[wEnemyIndex].e.wActWaitFrames, 0, FALSE);
         }
      }

      if (gpGlobals->g.lprgMagic[wMagicNum].wType != kMagicTypeNormal)
      {
         sTarget = -1;

         for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
         {
            w = gpGlobals->rgParty[i].wPlayerRole;

            if (gpGlobals->rgPlayerStatus[w][kStatusSleep] == 0 &&
               gpGlobals->rgPlayerStatus[w][kStatusParalyzed] == 0 &&
               gpGlobals->rgPlayerStatus[w][kStatusConfused] == 0 &&
               RandomLong(0, 2) == 0 &&
               gpGlobals->g.PlayerRoles.rgwHP[w] != 0)
            {
               rgfMagAutoDefend[i] = TRUE;
               g_Battle.rgPlayer[i].wCurrentFrame = 3;
            }
            else
            {
               rgfMagAutoDefend[i] = FALSE;
            }
         }
      }
      else if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSleep] == 0 &&
         gpGlobals->rgPlayerStatus[wPlayerRole][kStatusParalyzed] == 0 &&
         gpGlobals->rgPlayerStatus[wPlayerRole][kStatusConfused] == 0 &&
         RandomLong(0, 2) == 0)
      {
         fAutoDefend = TRUE;
         g_Battle.rgPlayer[sTarget].wCurrentFrame = 3;
      }

//      PAL_BattleDelay(12, (unsigned short)(-((short)wMagic)), FALSE);

      gpGlobals->g.rgObject[wMagic].magic.wScriptOnUse =
         PAL_RunTriggerScript(gpGlobals->g.rgObject[wMagic].magic.wScriptOnUse, wPlayerRole);

      if (g_fScriptSuccess)
      {
         PAL_BattleShowEnemyMagicAnim(wEnemyIndex, wMagic, sTarget);

         gpGlobals->g.rgObject[wMagic].magic.wScriptOnSuccess =
            PAL_RunTriggerScript(gpGlobals->g.rgObject[wMagic].magic.wScriptOnSuccess, wPlayerRole);
      }

      if ((short)(gpGlobals->g.lprgMagic[wMagicNum].wBaseDamage) > 0)
      {
         if (sTarget == -1)
         {
            //
            // damage all players
            //
            for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
            {
               w = gpGlobals->rgParty[i].wPlayerRole;
               if (gpGlobals->g.PlayerRoles.rgwHP[w] == 0)
               {
                  //
                  // skip dead players
                  //
                  continue;
               }

               def = PAL_GetPlayerDefense(w);

               for (x = 0; x < NUM_MAGIC_ELEMENTAL; x++)
               {
                  rgwElementalResistance[x] =
                     100 + PAL_GetPlayerElementalResistance(w, x);
               }

               sDamage = PAL_CalcMagicDamage(str, def, rgwElementalResistance,
                  100 + PAL_GetPlayerPoisonResistance(w), 20, wMagic);

               sDamage /= ((g_Battle.rgPlayer[i].fDefending ? 2 : 1) *
                  ((gpGlobals->rgPlayerStatus[w][kStatusProtect] > 0) ? 2 : 1)) +
                  (rgfMagAutoDefend[i] ? 1 : 0);

               if (sDamage > gpGlobals->g.PlayerRoles.rgwHP[w])
               {
                  sDamage = gpGlobals->g.PlayerRoles.rgwHP[w];
               }

#ifndef INVINCIBLE
               gpGlobals->g.PlayerRoles.rgwHP[w] -= sDamage;
#endif

               if (gpGlobals->g.PlayerRoles.rgwHP[w] == 0)
               {
                  AUDIO_PlaySound(gpGlobals->g.PlayerRoles.rgwDeathSound[w]);
               }
            }
         }
         else
         {
            //
            // damage one player
            //
            def = PAL_GetPlayerDefense(wPlayerRole);

            for (x = 0; x < NUM_MAGIC_ELEMENTAL; x++)
            {
               rgwElementalResistance[x] =
                  100 + PAL_GetPlayerElementalResistance(wPlayerRole, x);
            }

            sDamage = PAL_CalcMagicDamage(str, def, rgwElementalResistance,
               100 + PAL_GetPlayerPoisonResistance(wPlayerRole), 20, wMagic);

            sDamage /= ((g_Battle.rgPlayer[sTarget].fDefending ? 2 : 1) *
               ((gpGlobals->rgPlayerStatus[wPlayerRole][kStatusProtect] > 0) ? 2 : 1)) +
               (fAutoDefend ? 1 : 0);

            if (sDamage > gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole])
            {
               sDamage = gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole];
            }

#ifndef INVINCIBLE
            gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] -= sDamage;
#endif

            if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] == 0)
            {
               AUDIO_PlaySound(gpGlobals->g.PlayerRoles.rgwDeathSound[wPlayerRole]);
            }
         }
      }

      if (!gpGlobals->fAutoBattle)
      {
         PAL_BattleDisplayStatChange();
      }

      for (i = 0; i < 5; i++)
      {
         if (sTarget == -1)
         {
            for (x = 0; x <= gpGlobals->wMaxPartyMemberIndex; x++)
            {
               if (g_Battle.rgPlayer[x].wPrevHP ==
                  gpGlobals->g.PlayerRoles.rgwHP[gpGlobals->rgParty[x].wPlayerRole])
               {
                  //
                  // Skip unaffected players
                  //
                  continue;
               }

               g_Battle.rgPlayer[x].wCurrentFrame = 4;
               if (i > 0)
               {
                  g_Battle.rgPlayer[x].pos =
                     PAL_XY(PAL_X(g_Battle.rgPlayer[x].pos) + (8 >> i),
                            PAL_Y(g_Battle.rgPlayer[x].pos) + (4 >> i));
               }
               g_Battle.rgPlayer[x].iColorShift = ((i < 3) ? 6 : 0);
            }
         }
         else
         {
            g_Battle.rgPlayer[sTarget].wCurrentFrame = 4;
            if (i > 0)
            {
               g_Battle.rgPlayer[sTarget].pos =
                  PAL_XY(PAL_X(g_Battle.rgPlayer[sTarget].pos) + (8 >> i),
                         PAL_Y(g_Battle.rgPlayer[sTarget].pos) + (4 >> i));
            }
            g_Battle.rgPlayer[sTarget].iColorShift = ((i < 3) ? 6 : 0);
         }

         PAL_BattleDelay(1, 0, FALSE);
      }

      g_Battle.rgEnemy[wEnemyIndex].wCurrentFrame = 0;
      g_Battle.rgEnemy[wEnemyIndex].pos = g_Battle.rgEnemy[wEnemyIndex].posOriginal;

      PAL_BattleDelay(1, 0, FALSE);
      PAL_BattleUpdateFighters();

      PAL_BattlePostActionCheck(TRUE);
      PAL_BattleDelay(8, 0, TRUE);
   }
   else
   {
      //
      // Physical attack
      //
      unsigned short wFrameBak = g_Battle.rgPlayer[sTarget].wCurrentFrame;

      str = (short)g_Battle.rgEnemy[wEnemyIndex].e.wAttackStrength;
      str += (g_Battle.rgEnemy[wEnemyIndex].e.wLevel + 6) * 6;
      if (str < 0)
      {
         str = 0;
      }

      def = PAL_GetPlayerDefense(wPlayerRole);

      if (g_Battle.rgPlayer[sTarget].fDefending)
      {
         def *= 2;
      }

      if(gConfig.fIsWIN95 && g_Battle.ActionQueue[g_Battle.iCurAction].fIsSecond && g_Battle.rgEnemy[wEnemyIndex].e.wMagic == 0)
          AUDIO_PlaySound(g_Battle.rgEnemy[wEnemyIndex].e.wMagicSound);
      else
      AUDIO_PlaySound(g_Battle.rgEnemy[wEnemyIndex].e.wAttackSound);

      iCoverIndex = -1;

      fAutoDefend = (RandomLong(0, 16) >= 10);

      //
      // Check if the inflictor should be protected
      //
      if ((PAL_IsPlayerDying(wPlayerRole) ||
         gpGlobals->rgPlayerStatus[wPlayerRole][kStatusConfused] > 0 ||
         gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSleep] > 0 ||
         gpGlobals->rgPlayerStatus[wPlayerRole][kStatusParalyzed] > 0) && fAutoDefend)
      {
         w = gpGlobals->g.PlayerRoles.rgwCoveredBy[wPlayerRole];

         for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
         {
            if (gpGlobals->rgParty[i].wPlayerRole == w)
            {
               iCoverIndex = i;
               break;
            }
         }

         if (iCoverIndex != -1)
         {
            if (PAL_IsPlayerDying(gpGlobals->rgParty[iCoverIndex].wPlayerRole) ||
               gpGlobals->rgPlayerStatus[gpGlobals->rgParty[iCoverIndex].wPlayerRole][kStatusConfused] > 0 ||
               gpGlobals->rgPlayerStatus[gpGlobals->rgParty[iCoverIndex].wPlayerRole][kStatusSleep] > 0 ||
               gpGlobals->rgPlayerStatus[gpGlobals->rgParty[iCoverIndex].wPlayerRole][kStatusParalyzed] > 0)
            {
               iCoverIndex = -1;
            }
         }
      }

      //
      // If no one can cover the inflictor and inflictor is in a
      // bad status, don't evade
      //
      if (iCoverIndex == -1 &&
         (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusConfused] > 0 ||
         gpGlobals->rgPlayerStatus[wPlayerRole][kStatusSleep] > 0 ||
         gpGlobals->rgPlayerStatus[wPlayerRole][kStatusParalyzed] > 0))
      {
         fAutoDefend = FALSE;
      }

      for (i = 0; i < g_Battle.rgEnemy[wEnemyIndex].e.wMagicFrames; i++)
      {
         g_Battle.rgEnemy[wEnemyIndex].wCurrentFrame =
            g_Battle.rgEnemy[wEnemyIndex].e.wIdleFrames + i;
         PAL_BattleDelay(2, 0, FALSE);
      }

      for (i = 0; i < 3 - g_Battle.rgEnemy[wEnemyIndex].e.wMagicFrames; i++)
      {
         x = PAL_X(g_Battle.rgEnemy[wEnemyIndex].pos) - 2;
         y = PAL_Y(g_Battle.rgEnemy[wEnemyIndex].pos) - 1;
         g_Battle.rgEnemy[wEnemyIndex].pos = PAL_XY(x, y);
         PAL_BattleDelay(1, 0, FALSE);
      }
	  if (!gConfig.fIsWIN95 || g_Battle.rgEnemy[wEnemyIndex].e.wActionSound != 0)
      {
         AUDIO_PlaySound(g_Battle.rgEnemy[wEnemyIndex].e.wActionSound);
      }
      PAL_BattleDelay(1, 0, FALSE);

      ex = PAL_X(g_Battle.rgPlayer[sTarget].pos) - 44;
      ey = PAL_Y(g_Battle.rgPlayer[sTarget].pos) - 16;

      iSound = g_Battle.rgEnemy[wEnemyIndex].e.wCallSound;

      if (iCoverIndex != -1)
      {
         iSound = gpGlobals->g.PlayerRoles.rgwCoverSound[gpGlobals->rgParty[iCoverIndex].wPlayerRole];

         g_Battle.rgPlayer[iCoverIndex].wCurrentFrame = 3;

         x = PAL_X(g_Battle.rgPlayer[sTarget].pos) - 24;
         y = PAL_Y(g_Battle.rgPlayer[sTarget].pos) - 12;

         g_Battle.rgPlayer[iCoverIndex].pos = PAL_XY(x, y);
      }
      else if (fAutoDefend)
      {
         g_Battle.rgPlayer[sTarget].wCurrentFrame = 3;
         iSound = gpGlobals->g.PlayerRoles.rgwCoverSound[wPlayerRole];
      }

      if (g_Battle.rgEnemy[wEnemyIndex].e.wAttackFrames == 0)
      {
         g_Battle.rgEnemy[wEnemyIndex].wCurrentFrame =
            g_Battle.rgEnemy[wEnemyIndex].e.wIdleFrames - 1;

         g_Battle.rgEnemy[wEnemyIndex].pos = PAL_XY(ex, ey);

         PAL_BattleDelay(2, 0, FALSE);
      }
      else
      {
         for (i = 0; i <= g_Battle.rgEnemy[wEnemyIndex].e.wAttackFrames; i++)
         {
            g_Battle.rgEnemy[wEnemyIndex].wCurrentFrame =
               g_Battle.rgEnemy[wEnemyIndex].e.wIdleFrames +
               g_Battle.rgEnemy[wEnemyIndex].e.wMagicFrames + i - 1;

            g_Battle.rgEnemy[wEnemyIndex].pos = PAL_XY(ex, ey);

            PAL_BattleDelay(g_Battle.rgEnemy[wEnemyIndex].e.wActWaitFrames, 0, FALSE);
         }
      }

      if (!fAutoDefend)
      {
         g_Battle.rgPlayer[sTarget].wCurrentFrame = 4;

         sDamage = PAL_CalcPhysicalAttackDamage(str + RandomLong(0, 2), def, 2);
         sDamage += RandomLong(0, 1);

         if (gpGlobals->rgPlayerStatus[wPlayerRole][kStatusProtect])
         {
            sDamage /= 2;
         }

         if ((short)gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] < sDamage)
         {
            sDamage = gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole];
         }

         if (sDamage <= 0)
         {
            sDamage = 1;
         }

#ifndef INVINCIBLE
         gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] -= sDamage;
#endif

         PAL_BattleDisplayStatChange();

         g_Battle.rgPlayer[sTarget].iColorShift = 6;
      }
	  if (!gConfig.fIsWIN95 || iSound != 0)
      {
         AUDIO_PlaySound(iSound);
      }
      PAL_BattleDelay(1, 0, FALSE);

      g_Battle.rgPlayer[sTarget].iColorShift = 0;

      if (iCoverIndex != -1)
      {
         g_Battle.rgEnemy[wEnemyIndex].pos =
            PAL_XY(PAL_X(g_Battle.rgEnemy[wEnemyIndex].pos) - 10,
                   PAL_Y(g_Battle.rgEnemy[wEnemyIndex].pos) - 8);
         g_Battle.rgPlayer[iCoverIndex].pos =
            PAL_XY(PAL_X(g_Battle.rgPlayer[iCoverIndex].pos) + 4,
                   PAL_Y(g_Battle.rgPlayer[iCoverIndex].pos) + 2);
      }
      else
      {
         g_Battle.rgPlayer[sTarget].pos =
            PAL_XY(PAL_X(g_Battle.rgPlayer[sTarget].pos) + 8,
                   PAL_Y(g_Battle.rgPlayer[sTarget].pos) + 4);
      }

      PAL_BattleDelay(1, 0, FALSE);

      if (gpGlobals->g.PlayerRoles.rgwHP[wPlayerRole] == 0)
      {
         AUDIO_PlaySound(gpGlobals->g.PlayerRoles.rgwDeathSound[wPlayerRole]);
         wFrameBak = 2;
      }
      else if (PAL_IsPlayerDying(wPlayerRole))
      {
         wFrameBak = 1;
      }

      if (iCoverIndex == -1)
      {
         g_Battle.rgPlayer[sTarget].pos =
            PAL_XY(PAL_X(g_Battle.rgPlayer[sTarget].pos) + 2,
                   PAL_Y(g_Battle.rgPlayer[sTarget].pos) + 1);
      }

      PAL_BattleDelay(3, 0, FALSE);

      g_Battle.rgEnemy[wEnemyIndex].pos = g_Battle.rgEnemy[wEnemyIndex].posOriginal;
      g_Battle.rgEnemy[wEnemyIndex].wCurrentFrame = 0;

      PAL_BattleDelay(1, 0, FALSE);

      g_Battle.rgPlayer[sTarget].wCurrentFrame = wFrameBak;
      PAL_BattleDelay(1, 0, TRUE);

      PAL_BattleDelay(4, 0, TRUE);

      PAL_BattleUpdateFighters();

      if (iCoverIndex == -1 && !fAutoDefend &&
         g_Battle.rgEnemy[wEnemyIndex].e.wAttackEquivItemRate >= RandomLong(1, 10) &&
		 PAL_GetPlayerPoisonResistance(wPlayerRole) < RandomLong(1, 100) )
      {
         i = g_Battle.rgEnemy[wEnemyIndex].e.wAttackEquivItem;
         gpGlobals->g.rgObject[i].item.wScriptOnUse =
            PAL_RunTriggerScript(gpGlobals->g.rgObject[i].item.wScriptOnUse, wPlayerRole);
      }

      PAL_BattlePostActionCheck(TRUE);
   }

end:

   i = 0; // do nothing
}

void
PAL_BattleStealFromEnemy(
   unsigned short           wTarget,
   unsigned short           wStealRate
)
/*++
  Purpose:

    Steal from the enemy.

  Parameters:

    [IN]  wTarget - the target enemy index.

    [IN]  wStealRate - the rate of successful theft.

  Return value:

    None.

--*/
{
   int   iPlayerIndex = g_Battle.wMovingPlayerIndex;
   int   offset, x, y, i;
   unsigned short s[256] = L"";

   g_Battle.rgPlayer[iPlayerIndex].wCurrentFrame = 10;
   offset = ((int)wTarget - iPlayerIndex) * 8;

   x = PAL_X(g_Battle.rgEnemy[wTarget].pos) + 64 - offset;
   y = PAL_Y(g_Battle.rgEnemy[wTarget].pos) + 22 + offset;

   g_Battle.rgPlayer[iPlayerIndex].pos = PAL_XY(x, y);

   PAL_BattleDelay(1, 0, TRUE);

   for (i = 0; i < 5; i++)
   {
      x -= i + 8;
      y -= 4;

      g_Battle.rgPlayer[iPlayerIndex].pos = PAL_XY(x, y);

      if (i == 4)
      {
         g_Battle.rgEnemy[wTarget].iColorShift = 6;
      }

      PAL_BattleDelay(1, 0, TRUE);
   }

   g_Battle.rgEnemy[wTarget].iColorShift = 0;
   x--;
   g_Battle.rgPlayer[iPlayerIndex].pos = PAL_XY(x, y);
   PAL_BattleDelay(3, 0, TRUE);

   g_Battle.rgPlayer[iPlayerIndex].state = kFighterWait;
   g_Battle.rgPlayer[iPlayerIndex].flTimeMeter = 0;
   PAL_BattleUpdateFighters();
   PAL_BattleDelay(1, 0, TRUE);

   if (g_Battle.rgEnemy[wTarget].e.nStealItem > 0 &&
      (RandomLong(0, 10) <= wStealRate || wStealRate == 0))
   {
      if (g_Battle.rgEnemy[wTarget].e.wStealItem == 0)
      {
         //
         // stolen coins
         //
         int c = g_Battle.rgEnemy[wTarget].e.nStealItem / RandomLong(2, 3);
         g_Battle.rgEnemy[wTarget].e.nStealItem -= c;
         gpGlobals->dwCash += c;

         if (c > 0)
         {
            PAL_swprintf(s, sizeof(s) / sizeof(unsigned short), L"@%ls @%d @%ls@", PAL_GetWord(34), c, PAL_GetWord(10));
         }
      }
      else
      {
         //
         // stolen item
         //
         g_Battle.rgEnemy[wTarget].e.nStealItem--;
         PAL_AddItemToInventory(g_Battle.rgEnemy[wTarget].e.wStealItem, 1);
         PAL_swprintf(s, sizeof(s) / sizeof(unsigned short), L"%ls@%ls@", PAL_GetWord(34), PAL_GetWord(g_Battle.rgEnemy[wTarget].e.wStealItem));
	  }

      if (s[0] != '\0')
      {
         PAL_StartDialog(kDialogCenterWindow, 0, 0, FALSE);
         PAL_ShowDialogText(s);
      }
   }
}

void
PAL_BattleSimulateMagic(
   short      sTarget,
   unsigned short       wMagicObjectID,
   unsigned short       wBaseDamage
)
/*++
  Purpose:

    Simulate a magic for players. Mostly used in item throwing script.

  Parameters:

    [IN]  sTarget - the target enemy index. -1 = all enemies.

    [IN]  wMagicObjectID - the object ID of the magic to be simulated.

    [IN]  wBaseDamage - the base damage of the simulation.

  Return value:

    None.

--*/
{
   short   sDamage;
   int     i, def;

   if (gpGlobals->g.rgObject[wMagicObjectID].magic.wFlags & kMagicFlagApplyToAll)
   {
      sTarget = -1;
   }
   else if (sTarget == -1)
   {
      sTarget = PAL_BattleSelectAutoTargetFrom(sTarget);
   }

   //
   // Show the magic animation
   //
   PAL_BattleShowPlayerOffMagicAnim(0xFFFF, wMagicObjectID, sTarget, FALSE);

   if (gpGlobals->g.lprgMagic[gpGlobals->g.rgObject[wMagicObjectID].magic.wMagicNumber].wBaseDamage > 0 ||
      wBaseDamage > 0)
   {
      if (sTarget == -1)
      {
         //
         // Apply to all enemies
         //
         for (i = 0; i <= g_Battle.wMaxEnemyIndex; i++)
         {
            if (g_Battle.rgEnemy[i].wObjectID == 0)
            {
               continue;
            }

            def = (short)g_Battle.rgEnemy[i].e.wDefense;
            def += (g_Battle.rgEnemy[i].e.wLevel + 6) * 4;

            if (def < 0)
            {
               def = 0;
            }

            sDamage = PAL_CalcMagicDamage(wBaseDamage, (unsigned short)def, g_Battle.rgEnemy[i].e.wElemResistance,
               g_Battle.rgEnemy[i].e.wPoisonResistance, 1, wMagicObjectID);

            if (sDamage < 0)
            {
               sDamage = 0;
            }

            g_Battle.rgEnemy[i].e.wHealth -= sDamage;
         }
      }
      else
      {
         //
         // Apply to one enemy
         //
         def = (short)g_Battle.rgEnemy[sTarget].e.wDefense;
         def += (g_Battle.rgEnemy[sTarget].e.wLevel + 6) * 4;

         if (def < 0)
         {
            def = 0;
         }

         sDamage = PAL_CalcMagicDamage(wBaseDamage, (unsigned short)def, g_Battle.rgEnemy[sTarget].e.wElemResistance,
            g_Battle.rgEnemy[sTarget].e.wPoisonResistance, 1, wMagicObjectID);

         if (sDamage < 0)
         {
            sDamage = 0;
         }

         g_Battle.rgEnemy[sTarget].e.wHealth -= sDamage;
      }
   }
}
