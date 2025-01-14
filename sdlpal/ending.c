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

#include "ending.h"
#include "audio/audio.h"
#include "common.h"
#include "global.h"
#include "palcfg.h"
#include "palcommon.h"
#include "palette.h"
#include "play.h"
#include "rngplay.h"
#include "scene.h"
#include "util.h"
#include "video/video.h"

static unsigned short g_wCurEffectSprite = 0;

static void PAL_ShowFBP(
    unsigned short wChunkNum,
    unsigned short wFade)
/*++
  Purpose:

    Draw an FBP picture to the screen.

  Parameters:

    [IN]  wChunkNum - number of chunk in fbp.mkf file.

    [IN]  wFade - fading speed of showing the picture.

  Return value:

    None.

--*/
{
   PAL_LARGE unsigned char buf[SCREEN_W * SCREEN_H];
   PAL_LARGE unsigned char bufSprite[SCREEN_W * SCREEN_H];
   const int rgIndex[6] = {0, 3, 1, 5, 2, 4};
   int i, j, k;
   unsigned char a, b;

   if (PAL_MKFDecompressChunk(buf, SCREEN_W * SCREEN_H, wChunkNum, gpGlobals->f.fpFBP) <= 0)
   {
      memset(buf, 0, sizeof(buf));
   }

   if (g_wCurEffectSprite != 0)
   {
      PAL_MKFDecompressChunk(bufSprite, SCREEN_W * SCREEN_H, g_wCurEffectSprite, gpGlobals->f.fpMGO);
   }

   if (wFade)
   {
      PAL_Surface *p = VIDEO_CreateCompatibleSizedSurface(NULL);

      wFade++;
      wFade *= 10;

      PAL_FBPBlitToSurface(buf, p);
      VIDEO_BackupScreen(gpScreen);

      for (i = 0; i < 16; i++)
      {
         for (j = 0; j < 6; j++)
         {
            //
            // Blend the pixels in the 2 buffers, and put the result into the
            // backup buffer
            //
            for (k = rgIndex[j]; k < SCREEN_W * SCREEN_H; k += 6)
            {
               a = ((unsigned char *)p->pixels)[k];
               b = ((unsigned char *)gpScreenBak->pixels)[k];

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

               ((unsigned char *)gpScreenBak->pixels)[k] = ((a & 0xF0) | (b & 0x0F));
            }

            VIDEO_RestoreScreen(gpScreen);

            if (g_wCurEffectSprite != 0)
            {
               int f = UTIL_GetTicks() / 150;
               PAL_RLEBlitToSurface(PAL_SpriteGetFrame(bufSprite, f % PAL_SpriteGetNumFrames(bufSprite)),
                                    gpScreen, PAL_XY(0, 0));
            }

            VIDEO_UpdateScreen(NULL);
            UTIL_Delay(wFade);
         }
      }

      PAL_FreeSurface(p);
   }

   //
   // HACKHACK: to make the ending show correctly
   //
   if (wChunkNum != 68)
   {
      PAL_FBPBlitToSurface(buf, gpScreen);
   }

   VIDEO_UpdateScreen(NULL);
}

static void PAL_ScrollFBP(
    unsigned short wChunkNum,
    unsigned short wScrollSpeed,
    int fScrollDown)
/*++
  Purpose:

    Scroll up an FBP picture to the screen.

  Parameters:

    [IN]  wChunkNum - number of chunk in fbp.mkf file.

    [IN]  wScrollSpeed - scrolling speed of showing the picture.

    [IN]  fScrollDown - TRUE if scroll down, FALSE if scroll up.

  Return value:

    None.

--*/
{
   PAL_Surface *p;
   PAL_LARGE unsigned char buf[SCREEN_W * SCREEN_H];
   PAL_LARGE unsigned char bufSprite[SCREEN_W * SCREEN_H];
   int i, l;
   PAL_Rect rect, dstrect;

   if (PAL_MKFDecompressChunk(buf, sizeof(buf), wChunkNum, gpGlobals->f.fpFBP) <= 0)
   {
      return;
   }

   if (g_wCurEffectSprite != 0)
   {
      PAL_MKFDecompressChunk(bufSprite, sizeof(bufSprite), g_wCurEffectSprite, gpGlobals->f.fpMGO);
   }

   p = VIDEO_CreateCompatibleSizedSurface(NULL);

   if (p == NULL)
   {
      return;
   }

   VIDEO_BackupScreen(gpScreen);
   PAL_FBPBlitToSurface(buf, p);

   if (wScrollSpeed == 0)
   {
      wScrollSpeed = 1;
   }

   rect.x = 0;
   rect.w = SCREEN_W;
   dstrect.x = 0;
   dstrect.w = SCREEN_W;

   for (l = 0; l < 220; l++)
   {
      i = l;
      if (i > 200)
      {
         i = 200;
      }

      if (fScrollDown)
      {
         rect.y = 0;
         dstrect.y = i;
         rect.h = 200 - i;
         dstrect.h = 200 - i;
      }
      else
      {
         rect.y = i;
         dstrect.y = 0;
         rect.h = 200 - i;
         dstrect.h = 200 - i;
      }

      VIDEO_CopySurface(gpScreenBak, &rect, gpScreen, &dstrect);

      if (fScrollDown)
      {
         rect.y = 200 - i;
         dstrect.y = 0;
         rect.h = i;
         dstrect.h = i;
      }
      else
      {
         rect.y = 0;
         dstrect.y = 200 - i;
         rect.h = i;
         dstrect.h = i;
      }

      VIDEO_CopySurface(p, &rect, gpScreen, &dstrect);

      PAL_ApplyWave(gpScreen);

      if (g_wCurEffectSprite != 0)
      {
         int f = UTIL_GetTicks() / 150;
         PAL_RLEBlitToSurface(PAL_SpriteGetFrame(bufSprite, f % PAL_SpriteGetNumFrames(bufSprite)),
                              gpScreen, PAL_XY(0, 0));
      }

      VIDEO_UpdateScreen(NULL);

      if (gpGlobals->fNeedToFadeIn)
      {
         PAL_FadeIn(gpGlobals->wNumPalette, gpGlobals->fNightPalette, 1);
         gpGlobals->fNeedToFadeIn = FALSE;
      }

      UTIL_Delay(800 / wScrollSpeed);
   }

   VIDEO_CopyEntireSurface(p, gpScreen);
   PAL_FreeSurface(p);
   VIDEO_UpdateScreen(NULL);
}

static void PAL_EndingAnimation(
    void)
/*++
  Purpose:

    Show the ending animation.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   const unsigned int buf_size = SCREEN_W * SCREEN_H;
   unsigned char *buf;
   unsigned char *bufGirl;
   PAL_Surface *pUpper;
   PAL_Surface *pLower;
   PAL_Rect srcrect, dstrect;

   int yPosGirl = 180;
   int i;

   buf = (unsigned char *)UTIL_calloc(1, buf_size);
   bufGirl = (unsigned char *)UTIL_calloc(1, 6000);

   pUpper = VIDEO_CreateCompatibleSizedSurface(NULL);
   pLower = VIDEO_CreateCompatibleSizedSurface(NULL);

   PAL_MKFDecompressChunk(buf, buf_size, 69, gpGlobals->f.fpFBP);
   PAL_FBPBlitToSurface(buf, pUpper);

   PAL_MKFDecompressChunk(buf, buf_size, 70, gpGlobals->f.fpFBP);
   PAL_FBPBlitToSurface(buf, pLower);

   PAL_MKFDecompressChunk(buf, buf_size, 571, gpGlobals->f.fpMGO);
   PAL_MKFDecompressChunk(bufGirl, 6000, 572, gpGlobals->f.fpMGO);

   srcrect.x = 0;
   dstrect.x = 0;
   srcrect.w = SCREEN_W;
   dstrect.w = SCREEN_W;

   gpGlobals->wScreenWave = 2;

   for (i = 0; i < 400; i++)
   {
      //
      // Draw the background
      //
      srcrect.y = 0;
      srcrect.h = 200 - i / 2;

      dstrect.y = i / 2;
      dstrect.h = 200 - i / 2;

      VIDEO_CopySurface(pLower, &srcrect, gpScreen, &dstrect);

      srcrect.y = 200 - i / 2;
      srcrect.h = i / 2;

      dstrect.y = 0;
      dstrect.h = i / 2;

      VIDEO_CopySurface(pUpper, &srcrect, gpScreen, &dstrect);

      PAL_ApplyWave(gpScreen);

      //
      // Draw the beast
      //
      PAL_RLEBlitToSurface(PAL_SpriteGetFrame(buf, 0), gpScreen, PAL_XY(0, -400 + i));
      PAL_RLEBlitToSurface(PAL_SpriteGetFrame(buf, 1), gpScreen, PAL_XY(0, -200 + i));
      //
      // Draw the girl
      //
      yPosGirl -= i & 1;
      if (yPosGirl < 80)
      {
         yPosGirl = 80;
      }

      PAL_RLEBlitToSurface(PAL_SpriteGetFrame(bufGirl, (UTIL_GetTicks() / 50) % 4),
                           gpScreen, PAL_XY(220, yPosGirl));

      //
      // Update the screen
      //
      VIDEO_UpdateScreen(NULL);
      if (gpGlobals->fNeedToFadeIn)
      {
         PAL_FadeIn(gpGlobals->wNumPalette, gpGlobals->fNightPalette, 1);
         gpGlobals->fNeedToFadeIn = FALSE;
      }

      UTIL_Delay(50);
   }

   gpGlobals->wScreenWave = 0;

   PAL_FreeSurface(pUpper);
   PAL_FreeSurface(pLower);

   free(buf);
   free(bufGirl);
}

void PAL_EndingScreen(
    void)
/*++
 Purpose:

   Show the ending screen for Win95 version.

 Parameters:

   None.

 Return value:

   None.

--*/
{
   //
   // Use AVI & WIN95's music if we can
   // Otherwise, simulate the ending of DOS version
   //
#if 1 // 不明音樂播放
   AUDIO_PlayMusic(-1, FALSE, 0);
   AUDIO_PlayMusic(0x1a, TRUE, 0);
   PAL_RNGPlay(gpGlobals->iCurPlayingRNG, 110, 150, 7);
   PAL_RNGPlay(gpGlobals->iCurPlayingRNG, 151, -1, 9);

   PAL_FadeOut(2);
#endif
#if 1 // 水魔獸
   AUDIO_PlayMusic(-1, FALSE, 0);
   AUDIO_PlayMusic(0x19, TRUE, 0);

   PAL_ShowFBP(75, 0);
   PAL_FadeIn(5, FALSE, 1);
   PAL_ScrollFBP(74, 0xf, TRUE);

   PAL_FadeOut(1);

   PAL_CleanScreen();
   gpGlobals->wNumPalette = 4;
   gpGlobals->fNeedToFadeIn = TRUE;
   PAL_EndingAnimation();
#endif
#if 1 // 水災
   AUDIO_PlayMusic(-1, FALSE, 0);
   AUDIO_PlayMusic(0x00, FALSE, 2);
   PAL_ColorFade(7, 15, FALSE);

   AUDIO_PlayMusic(-1, FALSE, 0);
   AUDIO_PlayMusic(0x11, TRUE, 0);

   PAL_CleanScreen();
   PAL_SetPalette(0, FALSE);
   PAL_RNGPlay(11, 0, -1, 7);

   PAL_FadeOut(2);

   PAL_CleanScreen();
   gpGlobals->wNumPalette = 8;
   gpGlobals->fNeedToFadeIn = TRUE;
   PAL_RNGPlay(10, 0, -1, 6);

   g_wCurEffectSprite = 0;
   PAL_ShowFBP(77, 10);

   VIDEO_BackupScreen(gpScreen);

   g_wCurEffectSprite = 0x27b;
   PAL_ShowFBP(76, 7);

   PAL_SetPalette(5, FALSE);
   PAL_ShowFBP(73, 7);
   PAL_ScrollFBP(72, 0xf, TRUE);

   PAL_ShowFBP(71, 7);
   PAL_ShowFBP(68, 7);

   g_wCurEffectSprite = 0;
   PAL_ShowFBP(68, 6);

   PAL_WaitForKey(0);
   AUDIO_PlayMusic(0x00, FALSE, 1);
   UTIL_Delay(500);
#endif
#if 1 // 工作人員名單
   AUDIO_PlayMusic(-1, FALSE, 0);
   AUDIO_PlayMusic(9, TRUE, 0);
   PAL_ScrollFBP(67, 0xf, TRUE);
   PAL_ScrollFBP(66, 0xf, TRUE); // 阿奴
   PAL_ScrollFBP(65, 0xf, TRUE);
   PAL_ScrollFBP(64, 0xf, TRUE); // 林月如
   PAL_ScrollFBP(63, 0xf, TRUE);
   PAL_ScrollFBP(62, 0xf, TRUE); // 趙靈兒
   PAL_ScrollFBP(61, 0xf, TRUE);
   PAL_ScrollFBP(60, 0xf, TRUE); // 李逍遙
   PAL_ScrollFBP(59, 0xf, TRUE);

   AUDIO_PlayMusic(0x00, FALSE, 6);
   PAL_FadeOut(3); // 淡出
#endif
}
