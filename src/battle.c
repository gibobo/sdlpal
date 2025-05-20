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

#include "battle.h"
#include "audio.h"
#include "driver.h"
#include "fight.h"
#include "global.h"
#include "input.h"
#include "palcommon.h"
#include "palette.h"
#include "play.h"
#include "scene.h"
#include "script.h"
#include "text.h"
#include "ui.h"
#include "util.h"
#include "video.h"
#include <assert.h>
#include <stdbool.h>
#include <string.h>

BATTLE *g_Battle = NULL;

unsigned short g_rgPlayerPos[MAX_PLAYERS_IN_PARTY][3][2] = {
    {{240, 170}},                        // one player
    {{200, 176}, {256, 152}},            // two players
    {{180, 180}, {234, 170}, {270, 146}} // three players
};

void PAL_GetPlayerPos(unsigned char PlayerIndex, int *posX, int *posY) {
   (*posX) = g_rgPlayerPos[gpGlobals->wMaxPartyMemberIndex][PlayerIndex][0];
   (*posY) = g_rgPlayerPos[gpGlobals->wMaxPartyMemberIndex][PlayerIndex][1];
}

void PAL_BattleDrawBackground(
    void)
/*++
  Purpose:

    Generate the combat background to the specified screen buffer.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   int i;
   unsigned char *pSrc;
   unsigned char *pDst;
   unsigned char b;

   // Draw the background
   pSrc = g_Battle->lpBackground->pixels;
   pDst = g_Battle->lpSceneBuf->pixels;

   for (i = 0; i < g_Battle->lpSceneBuf->w * g_Battle->lpSceneBuf->h; i++)
   {
      b = (*pSrc & 0x0F);
      b += g_Battle->sBackgroundColorShift;

      if (b & 0x80)
      {
         b = 0;
      }
      else if (b & 0x70)
      {
         b = 0x0F;
      }

      *pDst = (b | (*pSrc & 0xF0));

      ++pSrc;
      ++pDst;
   }

   PAL_ApplyWave(g_Battle->lpSceneBuf->pixels);
}

void PAL_BattleDrawEnemySprites(
    unsigned short wEnemyIndex,
    PAL_Surface *lpDstSurface)
/*++
  Purpose:

    Generate enemies in battle into the specified buffer.

  Parameters:

    [IN]  wEnemyIndex - The index of the enemy.

    [OUT] lpDstSurface - pointer to the destination SDL surface.

  Return value:

    None.

--*/
{
   unsigned int pos;

   // Draw the enemies
   pos = g_Battle->rgEnemy[wEnemyIndex].pos;

   if (g_Battle->rgEnemy[wEnemyIndex].rgwStatus[kStatusConfused] > 0 &&
       g_Battle->rgEnemy[wEnemyIndex].rgwStatus[kStatusSleep] == 0 &&
       g_Battle->rgEnemy[wEnemyIndex].rgwStatus[kStatusParalyzed] == 0)
   {
      // Enemy is confused
      pos = PAL_XY_OFFSET(pos, RandomLong(-1, 1), 0);
   }

   pos = PAL_XY_OFFSET(pos,
                       -PAL_RLEGetWidth(PAL_SpriteGetFrame(g_Battle->rgEnemy[wEnemyIndex].lpSprite, g_Battle->rgEnemy[wEnemyIndex].wCurrentFrame)) / 2,
                       -PAL_RLEGetHeight(PAL_SpriteGetFrame(g_Battle->rgEnemy[wEnemyIndex].lpSprite, g_Battle->rgEnemy[wEnemyIndex].wCurrentFrame)));

   if (g_Battle->rgEnemy[wEnemyIndex].wObjectID != 0)
   {
      if (g_Battle->rgEnemy[wEnemyIndex].iColorShift)
      {
         PAL_RLEBlitWithColorShift(PAL_SpriteGetFrame(g_Battle->rgEnemy[wEnemyIndex].lpSprite, g_Battle->rgEnemy[wEnemyIndex].wCurrentFrame),
                                   lpDstSurface, pos, g_Battle->rgEnemy[wEnemyIndex].iColorShift);
      }
      else
      {
         PAL_RLEBlitToSurface(PAL_SpriteGetFrame(g_Battle->rgEnemy[wEnemyIndex].lpSprite, g_Battle->rgEnemy[wEnemyIndex].wCurrentFrame),
                              lpDstSurface, pos);
      }
   }
}

void PAL_BattleDrawPlayerSprites(
    unsigned short wPlayerIndex,
    PAL_Surface *lpDstSurface)
/*++
  Purpose:

    Generate players in battle into the specified buffer.

  Parameters:

    [IN]  wPlayerIndex - The index of the player (-1 = summon god).

    [OUT] lpDstSurface - pointer to the destination SDL surface.

  Return value:

    None.

--*/
{
   unsigned int pos;

   if (wPlayerIndex == 0xFFFF)
   {
      //
      // Draw the summoned god
      //
      if (g_Battle->lpSummonSprite != NULL)
      {
         pos = PAL_XY_OFFSET(g_Battle->posSummon,
                              -PAL_RLEGetWidth(PAL_SpriteGetFrame(g_Battle->lpSummonSprite, g_Battle->iSummonFrame)) / 2,
                              -PAL_RLEGetHeight(PAL_SpriteGetFrame(g_Battle->lpSummonSprite, g_Battle->iSummonFrame)));

         PAL_RLEBlitToSurface(PAL_SpriteGetFrame(g_Battle->lpSummonSprite, g_Battle->iSummonFrame),
                              lpDstSurface, pos);
      }
   }
   else
   {
      //
      // Draw the players
      //
      pos = g_Battle->rgPlayer[wPlayerIndex].pos;

      if (gpGlobals->rgPlayerStatus[gpGlobals->rgParty[wPlayerIndex].wPlayerRole][kStatusConfused] != 0 &&
          gpGlobals->rgPlayerStatus[gpGlobals->rgParty[wPlayerIndex].wPlayerRole][kStatusSleep] == 0 &&
          gpGlobals->rgPlayerStatus[gpGlobals->rgParty[wPlayerIndex].wPlayerRole][kStatusParalyzed] == 0 &&
          gpGlobals->g.PlayerRoles->rgwHP[gpGlobals->rgParty[wPlayerIndex].wPlayerRole] > 0 &&
          !PAL_IsPlayerDying(gpGlobals->rgParty[wPlayerIndex].wPlayerRole))
      {
         //
         // Player is confused and not dead
         //
         pos = PAL_XY_OFFSET(pos, 0, RandomLong(-1, 1));
      }

      pos = PAL_XY_OFFSET(pos,
                          -PAL_RLEGetWidth(PAL_SpriteGetFrame(g_Battle->rgPlayer[wPlayerIndex].lpSprite, g_Battle->rgPlayer[wPlayerIndex].wCurrentFrame)) / 2,
                          -PAL_RLEGetHeight(PAL_SpriteGetFrame(g_Battle->rgPlayer[wPlayerIndex].lpSprite, g_Battle->rgPlayer[wPlayerIndex].wCurrentFrame)));

      if (g_Battle->rgPlayer[wPlayerIndex].iColorShift != 0)
      {
         PAL_RLEBlitWithColorShift(PAL_SpriteGetFrame(g_Battle->rgPlayer[wPlayerIndex].lpSprite, g_Battle->rgPlayer[wPlayerIndex].wCurrentFrame),
                                   lpDstSurface, pos, g_Battle->rgPlayer[wPlayerIndex].iColorShift);
      }
      else if (g_Battle->iHidingTime == 0)
      {
         PAL_RLEBlitToSurface(PAL_SpriteGetFrame(g_Battle->rgPlayer[wPlayerIndex].lpSprite, g_Battle->rgPlayer[wPlayerIndex].wCurrentFrame),
                              lpDstSurface, pos);
      }
   }
}

void PAL_BattleDrawMagicSprites(
    int iMagicNum,
    PAL_Surface *lpDstSurface,
    unsigned int pos)
/*++
  Purpose:

    Generate magic in battle into the specified buffer.

  Parameters:

    [IN]  wObjectID - The index of the magic.

    [OUT] lpDstSurface - pointer to the destination SDL surface.

  Return value:

    None.

--*/
{
   short x, y;
   const unsigned char *lpBitmap = g_Battle->lpMagicBitmap;

   x = PAL_X(pos);
   y = PAL_Y(pos);

   PAL_RLEBlitToSurface(lpBitmap, lpDstSurface, PAL_XY(x - PAL_RLEGetWidth(lpBitmap) / 2, y - PAL_RLEGetHeight(lpBitmap)));
}

void PAL_BattleClearSpriteObject(
    void)
/*++
  Purpose:

    Empty the Sprite object drawing sequence.

  Parameters:

    [IN]  wSpriteObjectIndex - Specifies the index of the sprite object to be removed.

  Return value:

    None.

--*/
{
   memset(&g_Battle->SpriteDrawSeq, 0, sizeof(g_Battle->SpriteDrawSeq));

   g_Battle->wMaxSpriteDrawSeqIndex = 0;
}

void PAL_BattleSpriteAddUnlock(
    void)
{
   g_Battle->fSpriteAddLock = false;

   PAL_BattleClearSpriteObject();
}

void PAL_BattleAddSpriteObject(
    unsigned short wType,
    unsigned short wObjectIndex,
    unsigned int pos,
    short sLayerOffset,
    int fHaveColorShift)
/*++
  Purpose:

    Add sprite objects to the drawing sequence.

  Parameters:

    [IN]  bType - What type is the sprite object?

    [IN]  bObjectIndex - The index of this sprite object in its type.

    [IN]  pos - The pos of sprite object.

    [IN]  sLayerOffset - The layer offset of sprite object.

    [IN]  fHaveColorShift - true is the highest layer in battle.

  Return value:

    None.

--*/
{
   unsigned short *wMaxIndex = &g_Battle->wMaxSpriteDrawSeqIndex;
   BATTLESPRITESEQ *SpriteObject;

   if (*wMaxIndex + 1 < MAX_BATTLESPRITESEQ_ITEMS)
   {
      SpriteObject = &g_Battle->SpriteDrawSeq[*wMaxIndex];

      SpriteObject->wType = wType;
      SpriteObject->wObjectIndex = wObjectIndex;
      SpriteObject->pos = pos;
      SpriteObject->sLayerOffset = sLayerOffset;
      SpriteObject->fHaveColorShift = fHaveColorShift;

      (*wMaxIndex)++;
   }
}

void PAL_BattleRemoveSpriteObject(
    unsigned short wSpriteObjectIndex)
/*++
  Purpose:

    Removes the specified sprite object from the drawn sequence by index.

  Parameters:

    [IN]  wSpriteObjectIndex - Specifies the index of the sprite object to be removed.

  Return value:

    None.

--*/
{
   BATTLESPRITESEQ *SpriteObject;

   if (wSpriteObjectIndex < MAX_BATTLESPRITESEQ_ITEMS)
   {
      SpriteObject = &g_Battle->SpriteDrawSeq[wSpriteObjectIndex];

      memset(SpriteObject, 0, sizeof(*SpriteObject));

      g_Battle->wMaxSpriteDrawSeqIndex--;
   }
}

void PAL_BattleAddFighterSpriteObject(
    void)
/*++
  Purpose:

    Place enemies and players in the drawing sequence.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   int i;

   //
   // Place enemies in the drawing sequence.
   //
   for (i = 0; i <= g_Battle->wMaxEnemyIndex; i++)
   {
      PAL_BattleAddSpriteObject(kBattleSpriteTypeEnemy, i, g_Battle->rgEnemy[i].pos, 0, g_Battle->rgEnemy[i].iColorShift);
   }

   if (g_Battle->lpSummonSprite != NULL)
   {
      //
      // Place summon god in the drawing sequence.
      //
      PAL_BattleAddSpriteObject(kBattleSpriteTypePlayer, 0xFFFF, g_Battle->posSummon, 0, g_Battle->fSummonColorShift);
   }
   else
   {
      //
      // Place players in the drawing sequence.
      //
      for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
      {
         PAL_BattleAddSpriteObject(kBattleSpriteTypePlayer, i, g_Battle->rgPlayer[i].pos, 0, g_Battle->rgPlayer[i].iColorShift);
      }
   }
}

void PAL_BattleSortSpriteObjecByPos(
    void)
/*++
  Purpose:

    Sort all sprite according to pos.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   int i, j;
   BATTLESPRITESEQ *this, *next, tmp;
   short thisPosX, thisPosY, nextPosX, nextPosY;

   //
   // Sort the players drawing order by Y coordinate
   //
   for (i = 0; i < g_Battle->wMaxSpriteDrawSeqIndex; i++)
   {
      for (j = i + 1; j <= g_Battle->wMaxSpriteDrawSeqIndex - 1; j++)
      {
         this = &g_Battle->SpriteDrawSeq[i];
         next = &g_Battle->SpriteDrawSeq[j];

         thisPosY = PAL_Y(this->pos) + this->sLayerOffset;
         nextPosY = PAL_Y(next->pos) + next->sLayerOffset;

         if (thisPosY > nextPosY)
         {
            //
            // Pos Y compare successfully, directly swap variable values
            //
            tmp = *this;
            *this = *next;
            *next = tmp;
         }
         else if (thisPosY == nextPosY)
         {
            //
            // The pos Y of the two are equal. Compare their X coordinates
            //
            thisPosX = PAL_X(this->pos);
            nextPosX = PAL_X(next->pos);

            if (thisPosX < nextPosX)
            {
               tmp = *this;
               *this = *next;
               *next = tmp;
            }
         }
      }
   }
}

void PAL_BattleDrawAllSprites(
    void)
/*++
  Purpose:

    Draw all sprites to the battle screen buffer.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   //
   // Draw all sprites to the battle screen buffer.
   //
   PAL_BattleDrawAllSpritesWithColorShift(false);

   //
   // Only draw sprites with color shift into the battle screen buffer.
   // Because in the original game,
   // sprites with color shift are directly overlaid on the original sprites.
   //
   PAL_BattleDrawAllSpritesWithColorShift(true);
}

void PAL_BattleDrawAllSpritesWithColorShift(
    int fColorShift)
/*++
  Purpose:

    Draws all sprites with color shift to the battle screen buffer..

  Parameters:

    None.

  Return value:

    None.

--*/
{
   int i;
   BATTLESPRITESEQ *SpriteObject;

   //
   // Sort the sprite that need to be drawn
   //
   PAL_BattleSortSpriteObjecByPos();

   for (i = 0; i <= g_Battle->wMaxSpriteDrawSeqIndex; i++)
   {
      SpriteObject = &g_Battle->SpriteDrawSeq[i];

      if (fColorShift)
      {
         //
         // Draw only sprites with color shift
         //
         if (!SpriteObject->fHaveColorShift)
            continue;
      }

      switch (SpriteObject->wType)
      {
      case kBattleSpriteTypeNone:
         break;

      case kBattleSpriteTypeEnemy:
         PAL_BattleDrawEnemySprites(SpriteObject->wObjectIndex, g_Battle->lpSceneBuf);
         break;

      case kBattleSpriteTypePlayer:
         PAL_BattleDrawPlayerSprites(SpriteObject->wObjectIndex, g_Battle->lpSceneBuf);
         break;

      case kBattleSpriteTypeMagic:
         PAL_BattleDrawMagicSprites(SpriteObject->wObjectIndex, g_Battle->lpSceneBuf, SpriteObject->pos);
         break;
      }
   }
}

void PAL_BattleMakeScene(
    void)
/*++
  Purpose:

    Generate the battle scene into the scene buffer.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   PAL_BattleDrawBackground();

   if (g_Battle->fSpriteAddLock)
   {
      //
      // Initialize the drawing sequence
      //
      PAL_BattleClearSpriteObject();
   }
   else
   {
      g_Battle->fSpriteAddLock = true;
   }

   //
   // Place enemies and players in the drawing sequence
   //
   PAL_BattleAddFighterSpriteObject();

   //
   // Draw all sprite
   //
   PAL_BattleDrawAllSprites();
}

void PAL_BattleFadeScene(
    void)
/*++
  Purpose:

    Fade in the scene of battle.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   int i, j, k;
   unsigned char a, b;
   const int rgIndex[6] = {0, 3, 1, 5, 2, 4};

   for (i = 0; i < 12; i++)
   {
      for (j = 0; j < 6; j++)
      {
         UTIL_Delay(16);
         // Blend the pixels in the 2 buffers, and put the result into the
         // backup buffer
         for (k = rgIndex[j]; k < SCREEN_SIZE; k += 6)
         {
            a = g_Battle->lpSceneBuf->pixels[k];
            b = gpScreen->pixels[k];

            if (i > 0)
            {
               if ((a & 0x0F) > (b & 0x0F))
               {
                  b++;
               }
               else if ((a & 0x0F) < (b & 0x0F))
               {
                  b--;
               }
            }

            gpScreen->pixels[k] = (a & 0xF0) | (b & 0x0F);
         }

         // Draw the backup buffer to the screen
         PAL_BattleUIUpdate();
         VIDEO_UpdateScreen(NULL);
      }
   }

   //
   // Draw the result buffer to the screen as the final step
   //
   VIDEO_CopyEntireSurface(g_Battle->lpSceneBuf, gpScreen);
   PAL_BattleUIUpdate();

   VIDEO_UpdateScreen(NULL);
}

static BATTLERESULT
PAL_BattleMain(
    void)
/*++
  Purpose:

    The main battle routine.

  Parameters:

    None.

  Return value:

    The result of the battle.

--*/
{
   int i;

   VIDEO_BackupScreen(gpScreen);

   //
   // Generate the scene and draw the scene to the screen buffer
   //
   PAL_BattleMakeScene();
   VIDEO_CopyEntireSurface(g_Battle->lpSceneBuf, gpScreen);

   //
   // Fade out the music and delay for a while
   //
   AUDIO_PlayMusic(0x00, false, 1);
   UTIL_Delay(200);

   //
   // Switch the screen
   //
   VIDEO_SwitchScreen(5);

   //
   // Play the battle music
   //
   AUDIO_PlayMusic(gpGlobals->wNumBattleMusic, true, 0);

   //
   // Fade in the screen when needed
   //
   if (gpGlobals->fNeedToFadeIn)
   {
      PAL_FadeIn(gpGlobals->wNumPalette, gpGlobals->fNightPalette, 1);
      gpGlobals->fNeedToFadeIn = false;
   }

   //
   // Run the pre-battle scripts for each enemies
   //
   for (i = 0; i <= g_Battle->wMaxEnemyIndex; i++)
   {
      g_Battle->rgEnemy[i].wScriptOnTurnStart =
          PAL_RunTriggerScript(g_Battle->rgEnemy[i].wScriptOnTurnStart, i);

      if (g_Battle->BattleResult != kBattleResultPreBattle)
      {
         break;
      }
   }

   if (g_Battle->BattleResult == kBattleResultPreBattle)
   {
      g_Battle->BattleResult = kBattleResultOnGoing;
   }

   PAL_ClearKeyState();

   //
   // Run the main battle loop.
   //
   while (true)
   {
      // Break out if the battle ended.
      if (g_Battle->BattleResult != kBattleResultOnGoing)
      {
         break;
      }

      // Wait for the time of one frame. Accept input here.
      UTIL_Delay(BATTLE_FRAME_TIME);

      // Run the main frame routine.
      PAL_BattleStartFrame();

      // Update the screen.
      VIDEO_UpdateScreen(NULL);
   }

   //
   // Return the battle result
   //
   return g_Battle->BattleResult;
}

static void
PAL_FreeBattleSprites(
    void)
/*++
  Purpose:

    Free all the loaded sprites.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   int i;

   //
   // Free all the loaded sprites
   //
   for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
   {
      UTIL_free(g_Battle->rgPlayer[i].lpSprite);
      g_Battle->rgPlayer[i].lpSprite = NULL;
   }

   for (i = 0; i <= g_Battle->wMaxEnemyIndex; i++)
   {
      UTIL_free(g_Battle->rgEnemy[i].lpSprite);
      g_Battle->rgEnemy[i].lpSprite = NULL;
   }

   UTIL_free(g_Battle->lpSummonSprite);
   g_Battle->lpSummonSprite = NULL;
}

void PAL_LoadBattleSprites(
    void)
/*++
  Purpose:

    Load all the loaded sprites.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   int i, x, y, s;
   void *fp = NULL;

   PAL_FreeBattleSprites();

   fp = UTIL_fopen(RESOURCE_PATH "/abc.mkf", "rb");

   // Load battle sprites for players
   for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
   {
      s = PAL_GetPlayerBattleSprite(gpGlobals->rgParty[i].wPlayerRole);

      if (PAL_MKFDecompressChunk(&g_Battle->rgPlayer[i].lpSprite, 0, s, gFiles[Res_F].fp) <= 0)
        continue;

      //
      // Set the default position for this player
      //
      x = g_rgPlayerPos[gpGlobals->wMaxPartyMemberIndex][i][0];
      y = g_rgPlayerPos[gpGlobals->wMaxPartyMemberIndex][i][1];

      g_Battle->rgPlayer[i].posOriginal = PAL_XY(x, y);
      g_Battle->rgPlayer[i].pos = PAL_XY(x, y);
   }

   // Load battle sprites for enemies
   for (i = 0; i < MAX_ENEMIES_IN_TEAM; i++)
   {
      if (g_Battle->rgEnemy[i].wObjectID == 0)
         continue;

      if (PAL_MKFDecompressChunk(&g_Battle->rgEnemy[i].lpSprite, 0, gpGlobals->g.rgObject[g_Battle->rgEnemy[i].wObjectID].enemy.wEnemyID, fp) <= 0)
        continue;

      //
      // Set the default position for this enemy
      //
      x = gpGlobals->g.EnemyPos[i * MAX_ENEMIES_IN_TEAM + g_Battle->wMaxEnemyIndex].x;
      y = gpGlobals->g.EnemyPos[i * MAX_ENEMIES_IN_TEAM + g_Battle->wMaxEnemyIndex].y;

      y += g_Battle->rgEnemy[i].e.wYPosOffset;

      g_Battle->rgEnemy[i].posOriginal = PAL_XY(x, y);
      g_Battle->rgEnemy[i].pos = PAL_XY(x, y);
   }

   UTIL_fclose(fp);
}

static void
PAL_LoadBattleBackground(
    void)
/*++
  Purpose:

    Load the screen background picture of the battle.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   // Create the surface
   g_Battle->lpBackground = VIDEO_CreateCompatibleSizedSurface(NULL);

   // Load the picture
   PAL_MKFDecompressChunk(&g_Battle->lpBackground->pixels,
                          SCREEN_SIZE,
                          gpGlobals->wNumBattleField,
                          gFiles[Res_FBP].fp);
}

static void
PAL_BattleWon(
    void)
/*++
  Purpose:

    Show the "you win" message and add the experience points for players.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   const PAL_Rect rect = {0, 60, SCREEN_W, 100};
   PAL_Rect rect1 = {80, 0, 180, SCREEN_H};

   int i, j, iTotalCount;
   unsigned int dwExp;
   unsigned short w;
   int fLevelUp;
   PLAYERROLES OrigPlayerRoles;

   //
   // Backup the initial player stats
   //
   OrigPlayerRoles = *gpGlobals->g.PlayerRoles;

   VIDEO_BackupScreen(gpScreen);

   if (g_Battle->iExpGained > 0)
   {
      unsigned int w1 = PAL_WordWidth(BATTLEWIN_GETEXP_LABEL) + 3;
      unsigned int ww1 = (unsigned int)(w1 - 8) << 3;
      //
      // Play the "battle win" music
      //
      AUDIO_PlayMusic(g_Battle->fIsBoss ? 2 : 3, false, 0);

      //
      // Show the message about the total number of exp. and cash gained
      //
      PAL_CreateSingleLineBox(PAL_XY(83 - ww1, 60), w1, NULL);
      PAL_CreateSingleLineBox(PAL_XY(65, 105), 10, NULL);

      PAL_DrawText(PAL_GetWord(BATTLEWIN_GETEXP_LABEL), PAL_XY(95 - ww1, 70), 0, false, false, false);
      PAL_DrawText(PAL_GetWord(BATTLEWIN_BEATENEMY_LABEL), PAL_XY(77, 115), 0, false, false, false);
      PAL_DrawText(PAL_GetWord(BATTLEWIN_DOLLAR_LABEL), PAL_XY(197, 115), 0, false, false, false);

      PAL_DrawNumber(g_Battle->iExpGained, 5, PAL_XY(182 + ww1, 74), kNumColorYellow, kNumAlignRight);
      PAL_DrawNumber(g_Battle->iCashGained, 5, PAL_XY(162, 119), kNumColorYellow, kNumAlignMid);

      VIDEO_UpdateScreen(&rect);
      PAL_WaitForAnyKey(g_Battle->fIsBoss ? 5500 : 3000);
   }

   //
   // Add the cash value
   //
   gpGlobals->dwCash += g_Battle->iCashGained;

   const MENUITEM rgFakeMenuItem[] =
       {
           // value  label                        enabled   pos
           {1, gpGlobals->g.PlayerRoles->rgwName[0], true, PAL_XY(0, 0)},
           {2, gpGlobals->g.PlayerRoles->rgwName[1], true, PAL_XY(0, 0)},
           {3, gpGlobals->g.PlayerRoles->rgwName[2], true, PAL_XY(0, 0)},
           {4, gpGlobals->g.PlayerRoles->rgwName[3], true, PAL_XY(0, 0)},
           {5, gpGlobals->g.PlayerRoles->rgwName[4], true, PAL_XY(0, 0)},
           {6, gpGlobals->g.PlayerRoles->rgwName[5], true, PAL_XY(0, 0)},
       };
   unsigned int maxNameWidth = PAL_MenuTextMaxWidth(rgFakeMenuItem, sizeof(rgFakeMenuItem) / sizeof(MENUITEM));
   const MENUITEM rgFakeMenuItem2[] =
       {
           // value  label                        enabled   pos
           {1, STATUS_LABEL_LEVEL, true, PAL_XY(0, 0)},
           {2, STATUS_LABEL_HP, true, PAL_XY(0, 0)},
           {3, STATUS_LABEL_MP, true, PAL_XY(0, 0)},
           {4, STATUS_LABEL_ATTACKPOWER, true, PAL_XY(0, 0)},
           {5, STATUS_LABEL_MAGICPOWER, true, PAL_XY(0, 0)},
           {6, STATUS_LABEL_RESISTANCE, true, PAL_XY(0, 0)},
           {7, STATUS_LABEL_DEXTERITY, true, PAL_XY(0, 0)},
           {8, STATUS_LABEL_FLEERATE, true, PAL_XY(0, 0)},
       };
   unsigned int maxPropertyWidth = PAL_MenuTextMaxWidth(rgFakeMenuItem2, sizeof(rgFakeMenuItem2) / sizeof(MENUITEM)) - 1;
   int propertyLength = maxPropertyWidth - 1;
   int offsetX = -8 * propertyLength;
   rect1.x += offsetX;
   rect1.w -= 2 * offsetX;
   //
   // Add the experience points for each players
   //
   for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
   {
      fLevelUp = false;

      w = gpGlobals->rgParty[i].wPlayerRole;
      if (gpGlobals->g.PlayerRoles->rgwHP[w] == 0)
      {
         continue; // don't care about dead players
      }

      dwExp = gpGlobals->Exp.rgPrimaryExp[w].wExp;
      dwExp += g_Battle->iExpGained;

      if (gpGlobals->g.PlayerRoles->rgwLevel[w] > MAX_LEVELS)
      {
         gpGlobals->g.PlayerRoles->rgwLevel[w] = MAX_LEVELS;
      }

      while (dwExp >= gpGlobals->g.rgLevelUpExp[gpGlobals->g.PlayerRoles->rgwLevel[w]])
      {
         dwExp -= gpGlobals->g.rgLevelUpExp[gpGlobals->g.PlayerRoles->rgwLevel[w]];

         if (gpGlobals->g.PlayerRoles->rgwLevel[w] < MAX_LEVELS)
         {
            fLevelUp = true;
            PAL_PlayerLevelUp(w, 1);

            gpGlobals->g.PlayerRoles->rgwHP[w] = gpGlobals->g.PlayerRoles->rgwMaxHP[w];
            gpGlobals->g.PlayerRoles->rgwMP[w] = gpGlobals->g.PlayerRoles->rgwMaxMP[w];
         }
      }

      gpGlobals->Exp.rgPrimaryExp[w].wExp = (unsigned short)dwExp;

      if (fLevelUp)
      {
         VIDEO_RestoreScreen(gpScreen);
         //
         // Player has gained a level. Show the message
         //
         PAL_CreateSingleLineBox(PAL_XY(offsetX + 80, 0), propertyLength + 10, NULL);
         PAL_CreateBox(PAL_XY(offsetX + 82, 32), 7, propertyLength + 8, 1, NULL);

         wchar_t buffer[32] = L"";
         PAL_swprintf(buffer, sizeof(buffer) / sizeof(wchar_t), L"%ls%ls%ls", PAL_GetWord(gpGlobals->g.PlayerRoles->rgwName[w]), PAL_GetWord(STATUS_LABEL_LEVEL), PAL_GetWord(BATTLEWIN_LEVELUP_LABEL));
         PAL_DrawText(buffer, PAL_XY(110, 10), 0, false, false, false);

         for (j = 0; j < 8; j++)
         {
            PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_ARROW), gpScreen, PAL_XY(-offsetX + 180, 48 + 18 * j));
         }

         PAL_DrawText(PAL_GetWord(STATUS_LABEL_LEVEL), PAL_XY(offsetX + 100, 44), BATTLEWIN_LEVELUP_LABEL_COLOR, true, false, false);
         PAL_DrawText(PAL_GetWord(STATUS_LABEL_HP), PAL_XY(offsetX + 100, 62), BATTLEWIN_LEVELUP_LABEL_COLOR, true, false, false);
         PAL_DrawText(PAL_GetWord(STATUS_LABEL_MP), PAL_XY(offsetX + 100, 80), BATTLEWIN_LEVELUP_LABEL_COLOR, true, false, false);
         PAL_DrawText(PAL_GetWord(STATUS_LABEL_ATTACKPOWER), PAL_XY(offsetX + 100, 98), BATTLEWIN_LEVELUP_LABEL_COLOR, true, false, false);
         PAL_DrawText(PAL_GetWord(STATUS_LABEL_MAGICPOWER), PAL_XY(offsetX + 100, 116), BATTLEWIN_LEVELUP_LABEL_COLOR, true, false, false);
         PAL_DrawText(PAL_GetWord(STATUS_LABEL_RESISTANCE), PAL_XY(offsetX + 100, 134), BATTLEWIN_LEVELUP_LABEL_COLOR, true, false, false);
         PAL_DrawText(PAL_GetWord(STATUS_LABEL_DEXTERITY), PAL_XY(offsetX + 100, 152), BATTLEWIN_LEVELUP_LABEL_COLOR, true, false, false);
         PAL_DrawText(PAL_GetWord(STATUS_LABEL_FLEERATE), PAL_XY(offsetX + 100, 170), BATTLEWIN_LEVELUP_LABEL_COLOR, true, false, false);

         //
         // Draw the original stats and stats after level up
         //
         PAL_DrawNumber(OrigPlayerRoles.rgwLevel[w], 4, PAL_XY(-offsetX + 133, 47), kNumColorYellow, kNumAlignRight);
         PAL_DrawNumber(gpGlobals->g.PlayerRoles->rgwLevel[w], 4, PAL_XY(-offsetX + 195, 47), kNumColorYellow, kNumAlignRight);

         PAL_DrawNumber(OrigPlayerRoles.rgwHP[w], 4, PAL_XY(-offsetX + 133, 64), kNumColorYellow, kNumAlignRight);
         PAL_DrawNumber(OrigPlayerRoles.rgwMaxHP[w], 4, PAL_XY(-offsetX + 154, 68), kNumColorBlue, kNumAlignRight);
         PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen, PAL_XY(-offsetX + 156, 66));
         PAL_DrawNumber(gpGlobals->g.PlayerRoles->rgwHP[w], 4, PAL_XY(-offsetX + 195, 64), kNumColorYellow, kNumAlignRight);
         PAL_DrawNumber(gpGlobals->g.PlayerRoles->rgwMaxHP[w], 4, PAL_XY(-offsetX + 216, 68), kNumColorBlue, kNumAlignRight);
         PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen, PAL_XY(-offsetX + 218, 66));

         PAL_DrawNumber(OrigPlayerRoles.rgwMP[w], 4, PAL_XY(-offsetX + 133, 82), kNumColorYellow, kNumAlignRight);
         PAL_DrawNumber(OrigPlayerRoles.rgwMaxMP[w], 4, PAL_XY(-offsetX + 154, 86), kNumColorBlue, kNumAlignRight);
         PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen, PAL_XY(-offsetX + 156, 84));
         PAL_DrawNumber(gpGlobals->g.PlayerRoles->rgwMP[w], 4, PAL_XY(-offsetX + 195, 82), kNumColorYellow, kNumAlignRight);
         PAL_DrawNumber(gpGlobals->g.PlayerRoles->rgwMaxMP[w], 4, PAL_XY(-offsetX + 216, 86), kNumColorBlue, kNumAlignRight);
         PAL_RLEBlitToSurface(PAL_SpriteGetFrame(gpSpriteUI, SPRITENUM_SLASH), gpScreen, PAL_XY(-offsetX + 218, 84));

         PAL_DrawNumber(OrigPlayerRoles.rgwAttackStrength[w] + PAL_GetPlayerAttackStrength(w) - gpGlobals->g.PlayerRoles->rgwAttackStrength[w], 4, PAL_XY(-offsetX + 133, 101), kNumColorYellow, kNumAlignRight);
         PAL_DrawNumber(PAL_GetPlayerAttackStrength(w), 4, PAL_XY(-offsetX + 195, 101), kNumColorYellow, kNumAlignRight);

         PAL_DrawNumber(OrigPlayerRoles.rgwMagicStrength[w] + PAL_GetPlayerMagicStrength(w) - gpGlobals->g.PlayerRoles->rgwMagicStrength[w], 4, PAL_XY(-offsetX + 133, 119), kNumColorYellow, kNumAlignRight);
         PAL_DrawNumber(PAL_GetPlayerMagicStrength(w), 4, PAL_XY(-offsetX + 195, 119), kNumColorYellow, kNumAlignRight);

         PAL_DrawNumber(OrigPlayerRoles.rgwDefense[w] + PAL_GetPlayerDefense(w) - gpGlobals->g.PlayerRoles->rgwDefense[w], 4, PAL_XY(-offsetX + 133, 137), kNumColorYellow, kNumAlignRight);
         PAL_DrawNumber(PAL_GetPlayerDefense(w), 4, PAL_XY(-offsetX + 195, 137), kNumColorYellow, kNumAlignRight);

         PAL_DrawNumber(OrigPlayerRoles.rgwDexterity[w] + PAL_GetPlayerDexterity(w) - gpGlobals->g.PlayerRoles->rgwDexterity[w], 4, PAL_XY(-offsetX + 133, 155), kNumColorYellow, kNumAlignRight);
         PAL_DrawNumber(PAL_GetPlayerDexterity(w), 4, PAL_XY(-offsetX + 195, 155), kNumColorYellow, kNumAlignRight);

         PAL_DrawNumber(OrigPlayerRoles.rgwFleeRate[w] + PAL_GetPlayerFleeRate(w) - gpGlobals->g.PlayerRoles->rgwFleeRate[w], 4, PAL_XY(-offsetX + 133, 173), kNumColorYellow, kNumAlignRight);
         PAL_DrawNumber(PAL_GetPlayerFleeRate(w), 4, PAL_XY(-offsetX + 195, 173), kNumColorYellow, kNumAlignRight);

         //
         // Update the screen and wait for key
         //
         VIDEO_UpdateScreen(&rect1);
         PAL_WaitForAnyKey(3000);

         OrigPlayerRoles = *gpGlobals->g.PlayerRoles;
      }

      //
      // Increasing of other hidden levels
      //
      iTotalCount = 0;

      iTotalCount += gpGlobals->Exp.rgAttackExp[w].wCount;
      iTotalCount += gpGlobals->Exp.rgDefenseExp[w].wCount;
      iTotalCount += gpGlobals->Exp.rgDexterityExp[w].wCount;
      iTotalCount += gpGlobals->Exp.rgFleeExp[w].wCount;
      iTotalCount += gpGlobals->Exp.rgHealthExp[w].wCount;
      iTotalCount += gpGlobals->Exp.rgMagicExp[w].wCount;
      iTotalCount += gpGlobals->Exp.rgMagicPowerExp[w].wCount;

      if (iTotalCount > 0)
      {
#define CHECK_HIDDEN_EXP(expname, statname, label)                                                                                                                                                \
   {                                                                                                                                                                                              \
      dwExp = g_Battle->iExpGained;                                                                                                                                                                \
      dwExp *= gpGlobals->Exp.expname[w].wCount;                                                                                                                                                  \
      dwExp /= iTotalCount;                                                                                                                                                                       \
      dwExp *= 2;                                                                                                                                                                                 \
                                                                                                                                                                                                  \
      dwExp += gpGlobals->Exp.expname[w].wExp;                                                                                                                                                    \
                                                                                                                                                                                                  \
      if (gpGlobals->Exp.expname[w].wLevel > MAX_LEVELS)                                                                                                                                          \
      {                                                                                                                                                                                           \
         gpGlobals->Exp.expname[w].wLevel = MAX_LEVELS;                                                                                                                                           \
      }                                                                                                                                                                                           \
                                                                                                                                                                                                  \
      while (dwExp >= gpGlobals->g.rgLevelUpExp[gpGlobals->Exp.expname[w].wLevel])                                                                                                                \
      {                                                                                                                                                                                           \
         dwExp -= gpGlobals->g.rgLevelUpExp[gpGlobals->Exp.expname[w].wLevel];                                                                                                                    \
         gpGlobals->g.PlayerRoles->statname[w] += RandomLong(1, 2);                                                                                                                                \
         if (gpGlobals->Exp.expname[w].wLevel < MAX_LEVELS)                                                                                                                                       \
         {                                                                                                                                                                                        \
            gpGlobals->Exp.expname[w].wLevel++;                                                                                                                                                   \
         }                                                                                                                                                                                        \
      }                                                                                                                                                                                           \
                                                                                                                                                                                                  \
      gpGlobals->Exp.expname[w].wExp = (unsigned short)dwExp;                                                                                                                                     \
                                                                                                                                                                                                  \
      if (gpGlobals->g.PlayerRoles->statname[w] != OrigPlayerRoles.statname[w])                                                                                                                    \
      {                                                                                                                                                                                           \
         wchar_t buffer[32] = L"";                                                                                                                                                        \
         PAL_swprintf(buffer, sizeof(buffer) / sizeof(wchar_t), L"%ls%ls%ls", PAL_GetWord(gpGlobals->g.PlayerRoles->rgwName[w]), PAL_GetWord(label), PAL_GetWord(BATTLEWIN_LEVELUP_LABEL)); \
         PAL_CreateSingleLineBox(PAL_XY(offsetX + 78, 60), maxNameWidth + maxPropertyWidth + PAL_TextWidth(PAL_GetWord(BATTLEWIN_LEVELUP_LABEL)) / 32 + 4, NULL);                                \
         PAL_DrawText(buffer, PAL_XY(offsetX + 90, 70), 0, false, false, false);                                                                                                                  \
         PAL_DrawNumber(gpGlobals->g.PlayerRoles->statname[w] - OrigPlayerRoles.statname[w], 5, PAL_XY(183 + (maxNameWidth + maxPropertyWidth - 3) * 8, 74), kNumColorYellow, kNumAlignRight);     \
         VIDEO_UpdateScreen(&rect);                                                                                                                                                               \
         PAL_WaitForAnyKey(3000);                                                                                                                                                                 \
      }                                                                                                                                                                                           \
   }

         CHECK_HIDDEN_EXP(rgHealthExp, rgwMaxHP, STATUS_LABEL_HP);
         CHECK_HIDDEN_EXP(rgMagicExp, rgwMaxMP, STATUS_LABEL_MP);
         CHECK_HIDDEN_EXP(rgAttackExp, rgwAttackStrength, STATUS_LABEL_ATTACKPOWER);
         CHECK_HIDDEN_EXP(rgMagicPowerExp, rgwMagicStrength, STATUS_LABEL_MAGICPOWER);
         CHECK_HIDDEN_EXP(rgDefenseExp, rgwDefense, STATUS_LABEL_RESISTANCE);
         CHECK_HIDDEN_EXP(rgDexterityExp, rgwDexterity, STATUS_LABEL_DEXTERITY);
         CHECK_HIDDEN_EXP(rgFleeExp, rgwFleeRate, STATUS_LABEL_FLEERATE);

#undef CHECK_HIDDEN_EXP

         //
         // Avoid HP/MP out of sync with upgraded maxHP/MP
         //
         if (fLevelUp)
         {
            gpGlobals->g.PlayerRoles->rgwHP[w] = gpGlobals->g.PlayerRoles->rgwMaxHP[w];
            gpGlobals->g.PlayerRoles->rgwMP[w] = gpGlobals->g.PlayerRoles->rgwMaxMP[w];
         }
      }

      //
      // Learn all magics at the current level
      //
      j = 0;

      while (j < BATTLEWIN_LEVELUP_MAGIC)
      {
         if (gpGlobals->g.lprgLevelUpMagic[j].m[w].wMagic == 0 ||
             gpGlobals->g.lprgLevelUpMagic[j].m[w].wLevel > gpGlobals->g.PlayerRoles->rgwLevel[w])
         {
            j++;
            continue;
         }

         if (PAL_AddMagic(w, gpGlobals->g.lprgLevelUpMagic[j].m[w].wMagic))
         {
            unsigned int ww;
            unsigned int w1 = (ww = PAL_WordWidth(gpGlobals->g.PlayerRoles->rgwName[w])) > 3 ? ww : 3;
            unsigned int w2 = (ww = PAL_WordWidth(BATTLEWIN_ADDMAGIC_LABEL)) > 2 ? ww : 2;
            unsigned int w3 = (ww = PAL_WordWidth(gpGlobals->g.lprgLevelUpMagic[j].m[w].wMagic)) > 5 ? ww : 5;
            ww = (unsigned int)(w1 + w2 + w3 - 10) << 3;
            PAL_CreateSingleLineBox(PAL_XY(65 - ww, 105), w1 + w2 + w3, NULL);

            PAL_DrawText(PAL_GetWord(gpGlobals->g.PlayerRoles->rgwName[w]), PAL_XY(75 - ww, 115), 0, false, false, false);
            PAL_DrawText(PAL_GetWord(BATTLEWIN_ADDMAGIC_LABEL), PAL_XY(75 + 16 * w1 - ww, 115), 0, false, false, false);
            PAL_DrawText(PAL_GetWord(gpGlobals->g.lprgLevelUpMagic[j].m[w].wMagic), PAL_XY(75 + 16 * (w1 + w2) - ww, 115), 0x1B, false, false, false);

            VIDEO_UpdateScreen(&rect);
            PAL_WaitForAnyKey(3000);
         }

         j++;
      }
   }

   //
   // Run the post-battle scripts
   //
   for (i = 0; i <= g_Battle->wMaxEnemyIndex; i++)
   {
      PAL_RunTriggerScript(g_Battle->rgEnemy[i].wScriptOnBattleEnd, i);
   }

   //
   // Recover automatically after each battle
   //
   for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
   {
      w = gpGlobals->rgParty[i].wPlayerRole;
      gpGlobals->g.PlayerRoles->rgwHP[w] +=
          (gpGlobals->g.PlayerRoles->rgwMaxHP[w] - gpGlobals->g.PlayerRoles->rgwHP[w]) / 2;
      gpGlobals->g.PlayerRoles->rgwMP[w] +=
          (gpGlobals->g.PlayerRoles->rgwMaxMP[w] - gpGlobals->g.PlayerRoles->rgwMP[w]) / 2;
   }
}

void PAL_BattleEnemyEscape(
    void)
/*++
  Purpose:

    Enemy flee the battle.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   int j, x, y, w;
   int f = true;

   AUDIO_PlaySound(45);

   //
   // Show the animation
   //
   while (f)
   {
      f = false;

      for (j = 0; j <= g_Battle->wMaxEnemyIndex; j++)
      {
         if (g_Battle->rgEnemy[j].wObjectID == 0)
         {
            continue;
         }

         x = PAL_X(g_Battle->rgEnemy[j].pos) - 5;
         y = PAL_Y(g_Battle->rgEnemy[j].pos);

         g_Battle->rgEnemy[j].pos = PAL_XY(x, y);

         w = PAL_RLEGetWidth(PAL_SpriteGetFrame(g_Battle->rgEnemy[j].lpSprite, 0));

         if (x + w > 0)
         {
            f = true;
         }
      }

      PAL_BattleMakeScene();
      VIDEO_CopyEntireSurface(g_Battle->lpSceneBuf, gpScreen);
      VIDEO_UpdateScreen(NULL);

      UTIL_Delay(10);
   }

   UTIL_Delay(500);
   g_Battle->BattleResult = kBattleResultTerminated;
}

void PAL_BattlePlayerEscape(
    void)
/*++
  Purpose:

    Player flee the battle.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   int i, j;
   unsigned short wPlayerRole;

   AUDIO_PlaySound(45);

   PAL_BattleUpdateFighters();

   for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
   {
      wPlayerRole = gpGlobals->rgParty[i].wPlayerRole;

      if (gpGlobals->g.PlayerRoles->rgwHP[wPlayerRole] > 0)
      {
         g_Battle->rgPlayer[i].wCurrentFrame = 0;
      }
   }

   for (i = 0; i < 16; i++)
   {
      for (j = 0; j <= gpGlobals->wMaxPartyMemberIndex; j++)
      {
         wPlayerRole = gpGlobals->rgParty[j].wPlayerRole;

         if (gpGlobals->g.PlayerRoles->rgwHP[wPlayerRole] > 0)
         {
            //
            // TODO: This is still not the same as the original game
            //
            switch (j)
            {
            case 0:
               if (gpGlobals->wMaxPartyMemberIndex > 0)
               {
                  g_Battle->rgPlayer[j].pos =
                      PAL_XY(PAL_X(g_Battle->rgPlayer[j].pos) + 4,
                             PAL_Y(g_Battle->rgPlayer[j].pos) + 6);
                  break;
               }

            case 1:
               g_Battle->rgPlayer[j].pos =
                   PAL_XY(PAL_X(g_Battle->rgPlayer[j].pos) + 4,
                          PAL_Y(g_Battle->rgPlayer[j].pos) + 4);
               break;

            case 2:
               g_Battle->rgPlayer[j].pos =
                   PAL_XY(PAL_X(g_Battle->rgPlayer[j].pos) + 6,
                          PAL_Y(g_Battle->rgPlayer[j].pos) + 3);
               break;

            default:
               assert(false); // Not possible
               break;
            }
         }
      }

      PAL_BattleDelay(1, 0, false);
   }

   //
   // Remove all players from the screen
   //
   for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
   {
      g_Battle->rgPlayer[i].pos = 0xFFFFFFFF;
   }

   PAL_BattleDelay(1, 0, false);

   g_Battle->BattleResult = kBattleResultFleed;
}

BATTLERESULT
PAL_StartBattle(
    unsigned short wEnemyTeam,
    int fIsBoss)
/*++
  Purpose:

    Start a battle.

  Parameters:

    [IN]  wEnemyTeam - the number of the enemy team.

    [IN]  fIsBoss - true for boss fight (not allowed to flee).

  Return value:

    The result of the battle.

--*/
{
   int i, j;
   unsigned short w, wPrevWaveLevel;
   short sPrevWaveProgression;

   g_Battle = (BATTLE *)UTIL_malloc(sizeof(BATTLE));

   // Set the screen waving effects
   wPrevWaveLevel = gpGlobals->wScreenWave;
   sPrevWaveProgression = gpGlobals->sWaveProgression;

   gpGlobals->sWaveProgression = 0;
   gpGlobals->wScreenWave = gpGlobals->g.lprgBattleField[gpGlobals->wNumBattleField].wScreenWave;

   // Make sure everyone in the party is alive, also clear all hidden
   // EXP count records
   for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
   {
      w = gpGlobals->rgParty[i].wPlayerRole;

      if (gpGlobals->g.PlayerRoles->rgwHP[w] == 0)
      {
         gpGlobals->g.PlayerRoles->rgwHP[w] = 1;
         gpGlobals->rgPlayerStatus[w][kStatusPuppet] = 0;
      }

      gpGlobals->Exp.rgHealthExp[w].wCount = 0;
      gpGlobals->Exp.rgMagicExp[w].wCount = 0;
      gpGlobals->Exp.rgAttackExp[w].wCount = 0;
      gpGlobals->Exp.rgMagicPowerExp[w].wCount = 0;
      gpGlobals->Exp.rgDefenseExp[w].wCount = 0;
      gpGlobals->Exp.rgDexterityExp[w].wCount = 0;
      gpGlobals->Exp.rgFleeExp[w].wCount = 0;
   }

   // Clear all item-using records
   for (i = 0; i < MAX_INVENTORY; i++)
   {
      gpGlobals->rgInventory[i].nAmountInUse = 0;
   }

   // Store all enemies
   for (i = 0, j = 0; j < MAX_ENEMIES_IN_TEAM; j++)
   {
      memset(&g_Battle->rgEnemy[j], 0, sizeof(BATTLEENEMY));
      w = gpGlobals->g.lprgEnemyTeam[wEnemyTeam].rgwEnemy[j];

      if (w == 0xFFFF)
      {
         continue;
      }

      if (w != 0)
      {
         g_Battle->rgEnemy[i].e = gpGlobals->g.lprgEnemy[gpGlobals->g.rgObject[w].enemy.wEnemyID];
         g_Battle->rgEnemy[i].state = kFighterWait;
         g_Battle->rgEnemy[i].wScriptOnTurnStart = gpGlobals->g.rgObject[w].enemy.wScriptOnTurnStart;
         g_Battle->rgEnemy[i].wScriptOnBattleEnd = gpGlobals->g.rgObject[w].enemy.wScriptOnBattleEnd;
         g_Battle->rgEnemy[i].wScriptOnReady = gpGlobals->g.rgObject[w].enemy.wScriptOnReady;
         g_Battle->rgEnemy[i].iColorShift = 0;
      }

      g_Battle->rgEnemy[i++].wObjectID = w;
   }

   g_Battle->wMaxEnemyIndex = min(max(i - 1, 0), MAX_ENEMIES_IN_TEAM - 1);

   //
   // Store all players
   //
   for (i = 0; i <= gpGlobals->wMaxPartyMemberIndex; i++)
   {
      g_Battle->rgPlayer[i].flTimeMeter = 15.0f;
      g_Battle->rgPlayer[i].wHidingTime = 0;
      g_Battle->rgPlayer[i].state = kFighterWait;
      g_Battle->rgPlayer[i].fDefending = false;
      g_Battle->rgPlayer[i].wCurrentFrame = 0;
      g_Battle->rgPlayer[i].iColorShift = false;
   }

   // Load sprites and background
   PAL_LoadBattleSprites();
   PAL_LoadBattleBackground();

   // Create the surface for scene buffer
   g_Battle->lpSceneBuf = VIDEO_CreateCompatibleSizedSurface(NULL);

   PAL_UpdateEquipments();

   g_Battle->iExpGained = 0;
   g_Battle->iCashGained = 0;

   g_Battle->fIsBoss = fIsBoss;
   g_Battle->fEnemyCleared = false;
   g_Battle->fEnemyMoving = false;
   g_Battle->iHidingTime = 0;
   g_Battle->wMovingPlayerIndex = 0;

   g_Battle->UI.szMsg[0] = '\0';
   g_Battle->UI.szNextMsg[0] = '\0';
   g_Battle->UI.dwMsgShowTime = 0;
   g_Battle->UI.state = kBattleUIWait;
   g_Battle->UI.fAutoAttack = false;
   g_Battle->UI.iSelectedIndex = 0;
   g_Battle->UI.iPrevEnemyTarget = -1;

   memset(g_Battle->UI.rgShowNum, 0, sizeof(g_Battle->UI.rgShowNum));

   g_Battle->lpSummonSprite = NULL;
   g_Battle->sBackgroundColorShift = 0;

   gpGlobals->fInBattle = true;
   g_Battle->BattleResult = kBattleResultPreBattle;
   g_Battle->fSpriteAddLock = true;

   PAL_BattleUpdateFighters();

   //
   // Load the battle effect sprite.
   //
   i = PAL_MKFGetChunkSize(10, gFiles[Res_DATA].fp);
   g_Battle->lpEffectSprite = UTIL_malloc(i);

   PAL_MKFReadChunk(g_Battle->lpEffectSprite, i, 10, gFiles[Res_DATA].fp);

   g_Battle->Phase = kBattlePhaseSelectAction;
   g_Battle->fRepeat = false;
   g_Battle->fForce = false;
   g_Battle->fFlee = false;
   g_Battle->fPrevAutoAtk = false;
   g_Battle->fThisTurnCoop = false;

   // Run the main battle routine.
   i = PAL_BattleMain();

   if (i == kBattleResultWon)
   {
      // Player won the battle. Add the Experience points.
      PAL_BattleWon();
   }

   // Clear all item-using records
   for (w = 0; w < MAX_INVENTORY; w++)
   {
      gpGlobals->rgInventory[w].nAmountInUse = 0;
   }

   // Clear all player status, poisons and temporary effects
   PAL_ClearAllPlayerStatus();
   for (w = 0; w < MAX_PLAYER_ROLES; w++)
   {
      PAL_CurePoisonByLevel(w, 3);
      PAL_RemoveEquipmentEffect(w, kBodyPartExtra);
   }

   // Free all the battle sprites
   PAL_FreeBattleSprites();
   UTIL_free(g_Battle->lpEffectSprite);

   // Free the surfaces for the background picture and scene buffer
   PAL_FreeSurface(g_Battle->lpBackground);
   PAL_FreeSurface(g_Battle->lpSceneBuf);

   g_Battle->lpBackground = NULL;
   g_Battle->lpSceneBuf = NULL;
   UTIL_free(g_Battle);
   g_Battle = NULL;

   gpGlobals->fInBattle = false;

   AUDIO_PlayMusic(gpGlobals->wNumMusic, true, 1);

   // Restore the screen waving effects
   gpGlobals->sWaveProgression = sPrevWaveProgression;
   gpGlobals->wScreenWave = wPrevWaveLevel;

   return i;
}
