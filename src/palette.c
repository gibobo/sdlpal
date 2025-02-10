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

#include "palette.h"
#include "common.h"
#include "driver.h"
#include "global.h"
#include "input.h"
#include "palcommon.h"
#include "play.h"
#include "scene.h"
#include "util.h"

unsigned char *
PAL_GetPalette(
    int iPaletteNum,
    int fNight)
/*++
  Purpose:

    Get the specified palette in pat.mkf file.

  Parameters:

    [IN]  iPaletteNum - number of the palette.

    [IN]  fNight - whether use the night palette or not.

  Return value:

    Pointer to the palette. NULL if failed.

--*/
{
   static unsigned char palette[256 * 3];
   PAL_LARGE unsigned char buf[256 * 3 * 2];
   unsigned char *ptr;
   int i;
   void *fp;

   fp = UTIL_fopen(RESOURCE_PATH "/pat.mkf", "rb");

   memset(palette, 0, sizeof(palette));
   memset(buf, 0, sizeof(buf));

   // Read the palette data from the pat.mkf file
   i = PAL_MKFReadChunk(buf, sizeof(buf), iPaletteNum, fp);
   UTIL_fclose(fp);

   if (i < 0)
   {
      // Read failed
      return NULL;
   }
   else if (i <= 256 * 3)
   {
      // There is no night colors in the palette
      fNight = false;
   }
   ptr = buf + 256 * 3 * ((fNight) ? 1 : 0);

   for (i = 0; i < 256 * 3; i++)
   {
      palette[i] = ptr[i] << 2;
   }

   return palette;
}

void PAL_SetPalette(
    int iPaletteNum,
    int fNight)
/*++
  Purpose:

    Set the screen palette to the specified one.

  Parameters:

    [IN]  iPaletteNum - number of the palette.

    [IN]  fNight - whether use the night palette or not.

  Return value:

    None.

--*/
{
   unsigned char *p = PAL_GetPalette(iPaletteNum, fNight);

   if (p != NULL)
   {
      VIDEO_SetPalette(p);
   }
}

void PAL_FadeOut(
    int iDelay)
/*++
  Purpose:

    Fadeout screen to black from the specified palette.

  Parameters:

    [IN]  iPaletteNum - number of the palette.

    [IN]  fNight - whether use the night palette or not.

    [IN]  iDelay - delay time for each step.

  Return value:

    None.

--*/
{
   int i, j;
   unsigned int time;
   PAL_LARGE unsigned char palette[256 * 3];
   PAL_LARGE unsigned char newpalette[256 * 3];

   // Get the original palette...
   memcpy(palette, VIDEO_GetPalette(), sizeof(palette));
   memset(newpalette, 0, sizeof(newpalette));

   //
   // Start fading out...
   //
   time = UTIL_GetTicks() + iDelay * 10 * 60;

   while (time > UTIL_GetTicks())
   {
      //
      // Set the current palette...
      //
      j = (time - UTIL_GetTicks()) / (iDelay * 10);

      for (i = 0; i < 256 * 3; i++)
      {
         newpalette[i] = (palette[i] * j) >> 6;
      }

      VIDEO_SetPalette(newpalette);

      UTIL_Delay(10);
   }

   memset(newpalette, 0, sizeof(newpalette));
   VIDEO_SetPalette(newpalette);
}

void PAL_FadeIn(
    int iPaletteNum,
    int fNight,
    int iDelay)
/*++
  Purpose:

    Fade in the screen to the specified palette.

  Parameters:

    [IN]  iPaletteNum - number of the palette.

    [IN]  fNight - whether use the night palette or not.

    [IN]  iDelay - delay time for each step.

  Return value:

    None.

--*/
{
   int i, j;
   unsigned int time;
   unsigned char *palette;
   PAL_LARGE unsigned char newpalette[256 * 3];

   // Get the new palette...
   palette = PAL_GetPalette(iPaletteNum, fNight);
   memset(newpalette, 0, sizeof(newpalette));

   // Start fading in...
   time = UTIL_GetTicks() + iDelay * 10 * 60;
   while (true)
   {
      // Set the current palette...
      j = (int)(time - UTIL_GetTicks()) / iDelay / 10;
      if (j < 0)
      {
         break;
      }

      j = 60 - j;

      for (i = 0; i < 256 * 3; i++)
      {
         newpalette[i] = (palette[i] * j) >> 6;
      }

      VIDEO_SetPalette(newpalette);

      UTIL_Delay(10);
   }

   VIDEO_SetPalette(palette);
}

void PAL_SceneFade(
    int iPaletteNum,
    int fNight,
    int iStep)
/*++
  Purpose:

    Fade in or fade out the screen. Update the scene during the process.

  Parameters:

    [IN]  iPaletteNum - number of the palette.

    [IN]  fNight - whether use the night palette or not.

    [IN]  iStep - positive to fade in, nagative to fade out.

  Return value:

    None.

--*/
{
   unsigned char *palette;
   unsigned char newpalette[256 * 3];
   int i, j;

   palette = PAL_GetPalette(iPaletteNum, fNight);
   memset(newpalette, 0, sizeof(newpalette));

   if (palette == NULL)
   {
      return;
   }

   if (iStep == 0)
   {
      iStep = 1;
   }

   gpGlobals->fNeedToFadeIn = false;

   if (iStep > 0)
   {
      for (i = 0; i < 64; i += iStep)
      {
         // Generate the scene
         PAL_ClearKeyState();
         PAL_SetDirInput(kDirUnknown);
         PAL_GameUpdate(false);
         PAL_MakeScene();
         VIDEO_UpdateScreen(NULL);

         // Calculate the current palette...
         for (j = 0; j < 256 * 3; j++)
         {
            newpalette[j] = (palette[j] * i) >> 6;
         }
         VIDEO_SetPalette(newpalette);

         UTIL_Delay(100);
      }
   }
   else
   {
      for (i = 63; i >= 0; i += iStep)
      {
         // Generate the scene
         PAL_ClearKeyState();
         PAL_SetDirInput(kDirUnknown);
         PAL_GameUpdate(false);
         PAL_MakeScene();
         VIDEO_UpdateScreen(NULL);

         // Calculate the current palette...
         for (j = 0; j < 256 * 3; j++)
         {
            newpalette[j] = (palette[j] * i) >> 6;
         }
         VIDEO_SetPalette(newpalette);

         UTIL_Delay(100);
      }
   }
}

void PAL_PaletteFade(
    int iPaletteNum,
    int fNight,
    int fUpdateScene)
/*++
  Purpose:

    Fade from the current palette to the specified one.

  Parameters:

    [IN]  iPaletteNum - number of the palette.

    [IN]  fNight - whether use the night palette or not.

    [IN]  fUpdateScene - true if update the scene in the progress.

  Return value:

    None.

--*/
{
   int i, j;
   unsigned char *newpalette = PAL_GetPalette(iPaletteNum, fNight);
   PAL_LARGE unsigned char palette[256 * 3];
   PAL_LARGE unsigned char t[256 * 3];

   if (newpalette == NULL)
   {
      return;
   }

   memcpy(palette, VIDEO_GetPalette(), sizeof(palette));
   memset(t, 0, sizeof(t));

   // Start fading...
   for (i = 0; i < 32; i++)
   {
      for (j = 0; j < 256 * 3; j++)
      {
         t[j] = (unsigned char)(((int)palette[j] * (31 - i) + (int)(newpalette[j]) * i) / 31);
      }
      VIDEO_SetPalette(t);

      if (fUpdateScene)
      {
         PAL_ClearKeyState();
         PAL_SetDirInput(kDirUnknown);
         PAL_GameUpdate(false);
         PAL_MakeScene();
         VIDEO_UpdateScreen(NULL);
      }

      UTIL_Delay(fUpdateScene ? FRAME_TIME : FRAME_TIME / 4);
   }
}

void PAL_ColorFade(
    int iDelay,
    unsigned char bColor,
    int fFrom)
/*++
  Purpose:

    Fade the palette from/to the specified color.

  Parameters:

    [IN]  iDelay - the delay time of each step.

    [IN]  bColor - the color to fade from/to.

    [IN]  fFrom - if true then fade from bColor, else fade to bColor.

  Return value:

    None.

--*/
{
   unsigned char *palette;
   PAL_LARGE unsigned char newpalette[256 * 3];
   int i, j;

   palette = PAL_GetPalette(gpGlobals->wNumPalette, gpGlobals->fNightPalette);
   memset(newpalette, 0, sizeof(newpalette));

   iDelay *= 10;
   if (iDelay == 0)
   {
      iDelay = 10;
   }

#define Converge(A, B) A = (A > B) ? (A - 4) : ((A < B) ? (A + 4) : B)
   if (fFrom)
   {
      for (i = 0; i < 256; i++)
      {
        newpalette[i * 3 + 0] = palette[bColor * 3 + 0];
        newpalette[i * 3 + 1] = palette[bColor * 3 + 1];
        newpalette[i * 3 + 2] = palette[bColor * 3 + 2];
      }

      for (i = 0; i < 64; i++)
      {
         for (j = 0; j < 256; j++)
         {
           Converge(newpalette[j * 3 + 0], palette[j * 3 + 0]);
           Converge(newpalette[j * 3 + 1], palette[j * 3 + 1]);
           Converge(newpalette[j * 3 + 2], palette[j * 3 + 2]);
         }

         VIDEO_SetPalette(newpalette);
         UTIL_Delay(iDelay);
      }

      VIDEO_SetPalette(palette);
   }
   else
   {
      memcpy(newpalette, palette, sizeof(newpalette));

      for (i = 0; i < 64; i++)
      {
         for (j = 0; j < 256; j++)
         {
           Converge(newpalette[j * 3 + 0], palette[bColor * 3 + 0]);
           Converge(newpalette[j * 3 + 1], palette[bColor * 3 + 1]);
           Converge(newpalette[j * 3 + 2], palette[bColor * 3 + 2]);
         }

         VIDEO_SetPalette(newpalette);
         UTIL_Delay(iDelay);
      }

      for (i = 0; i < 256; i++)
      {
         newpalette[i] = palette[bColor];
      }

      VIDEO_SetPalette(newpalette);
   }
}

void PAL_FadeToRed(
    void)
/*++
  Purpose:

    Fade the whole screen to red color.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   unsigned char *palette;
   PAL_LARGE unsigned char newpalette[256 * 3];
   int i, j;
   unsigned char color;

   palette = PAL_GetPalette(gpGlobals->wNumPalette, gpGlobals->fNightPalette);
   memcpy(newpalette, palette, sizeof(newpalette));

   for (i = 0; i < SCREEN_SIZE; i++)
   {
      if (gpScreen->pixels[i] == 0x4F)
      {
         gpScreen->pixels[i] = 0x4E; // HACKHACK
      }
   }

   VIDEO_UpdateScreen(NULL);

   for (i = 0; i < 32; i++)
   {
      for (j = 0; j < 256; j++)
      {
         if (j == 0x4F)
         {
            continue; // so that texts will not be affected
         }

         color = ((int)palette[j*3+0] + (int)palette[j*3+1] + (int)palette[j*3+2]) / 4 + 64;

         if (newpalette[j*3+0] > color)
         {
            newpalette[j*3+0] -= (newpalette[j*3+0] - color > 8 ? 8 : newpalette[j*3+0] - color);
         }
         else if (newpalette[j*3+0] < color)
         {
            newpalette[j*3+0] += (color - newpalette[j*3+0] > 8 ? 8 : color - newpalette[j*3+0]);
         }

         if (newpalette[j*3+1] > 0)
         {
            newpalette[j*3+1] -= (newpalette[j*3+1] > 8 ? 8 : newpalette[j*3+1]);
         }

         if (newpalette[j*3+2] > 0)
         {
            newpalette[j*3+2] -= (newpalette[j*3+2] > 8 ? 8 : newpalette[j*3+2]);
         }
      }

      VIDEO_SetPalette(newpalette);
      UTIL_Delay(75);
   }
}
