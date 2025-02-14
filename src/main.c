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

#include "main.h"
#include "audio.h"
#include "driver.h"
#include "font.h"
#include "global.h"
#include "input.h"
#include "palcommon.h"
#include "palette.h"
#include "play.h"
#include "res.h"
#include "rngplay.h"
#include "text.h"
#include "ui.h"
#include "uigame.h"
#include "util.h"
#include "video.h"
#include <assert.h>
#include <setjmp.h>
#include <stdbool.h>

static jmp_buf g_exit_jmp_buf;
static int g_exit_code = 0;

void PAL_Init(void)
/*++
  Purpose:

    Initialize everything needed by the game.

  Parameters:

    None.

  Return value:

    None.

--*/
{
    int e;

    // Initialize subsystems.
    e = PAL_InitGlobals();
    if (e != 0) {
        PAL_Shutdown(255);
        TerminateOnError("Could not initialize global data: %d.\n", e);
    }
  
    e = DRIVER_Init();
    if (e != 0) {
        PAL_Shutdown(255);
        TerminateOnError("Could not initialize driver work: %d.\n", e);
    }

    e = VIDEO_Startup();
    if (e != 0) {
        PAL_Shutdown(255);
        TerminateOnError("Could not initialize Video: %d.\n", e);
    }

    e = PAL_InitUI();
    if (e != 0) {
        PAL_Shutdown(255);
        TerminateOnError("Could not initialize UI subsystem: %d.\n", e);
    }

    e = PAL_InitText();
    if (e != 0) {
        PAL_Shutdown(255);
        TerminateOnError("Could not initialize text subsystem: %d.\n", e);
    }

    PAL_InitFont();
    PAL_InitInput();
    PAL_InitResources();
    AUDIO_OpenDevice();
}

void PAL_Shutdown(int exit_code)
/*++
  Purpose:

    Free everything needed by the game.

  Parameters:

    exit_code -  The exit code return to OS.

  Return value:

    None.

--*/
{
   PAL_FreeResources();
   PAL_FreeUI();
   PAL_FreeText();
   PAL_ShutdownInput();
   AUDIO_CloseDevice();
   VIDEO_Shutdown();
   DRIVER_DeInit();

   // global needs be free in last
   // since subsystems may needs config content during destroy
   // which also cleared here
   PAL_DeInitFont();
   PAL_FreeGlobals();

   g_exit_code = exit_code;
   longjmp(g_exit_jmp_buf, 1);
}

void PAL_TrademarkScreen(void)
/*++
  Purpose:

    Show the trademark screen.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   PAL_SetPalette(3, false);
   PAL_RNGPlay(6, 0, -1, 25);
   UTIL_Delay(1000);
   PAL_FadeOut(1);
}

void PAL_SplashScreen(void)
/*++
  Purpose:

    Show the splash screen.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   unsigned char *palette = PAL_GetPalette(1, false);
   unsigned char rgCurrentPalette[256 * 3];
   PAL_Surface *lpBitmapUp= VIDEO_GetBackupSurface(0);
   PAL_Surface *lpBitmapDown= VIDEO_GetBackupSurface(1);
   PAL_Rect srcrect;
   PAL_Rect dstrect;
   unsigned char *lpTitleBuf = NULL;
   unsigned char *lpSpriteCrane = NULL;
   unsigned char *lpBitmapTitle = NULL;
   int cranepos[9][3];
   int i;
   int iImgPos = SCREEN_H;
   int iCraneFrame = 0;
   int iTitleHeight;
   unsigned int dwTime = 0;
   unsigned int dwBeginTime = 0;

   if (palette == NULL) {
      TerminateOnError("ERROR: PAL_SplashScreen(): palette == NULL\n");
      return;
   }

   // Read the bitmaps
   PAL_MKFDecompressChunk(&lpBitmapUp->pixels, SCREEN_SIZE, 0x03, gpGlobals->f.fpFBP);
   PAL_MKFDecompressChunk(&lpBitmapDown->pixels, SCREEN_SIZE, 0x04, gpGlobals->f.fpFBP);
   PAL_MKFDecompressChunk(&lpTitleBuf, 0, 0x47, gpGlobals->f.fpMGO);
   PAL_MKFDecompressChunk(&lpSpriteCrane, 0, 0x49, gpGlobals->f.fpMGO);
   lpBitmapTitle = (unsigned char *)PAL_SpriteGetFrame(lpTitleBuf, 0);
   iTitleHeight = PAL_RLEGetHeight(lpBitmapTitle);
   lpBitmapTitle[2] = 0;
   lpBitmapTitle[3] = 0; // HACKHACK

   // Generate the positions of the cranes
   for (i = 0; i < 9; i++)
   {
      cranepos[i][0] = RandomLong(300, 600);
      cranepos[i][1] = RandomLong(0, 80);
      cranepos[i][2] = RandomLong(0, 8);
   }

   // Play the title music
   AUDIO_PlayMusic(-1, false, 0);
   AUDIO_PlayMusic(0x05, true, 2);

   // Clear all of the events and key states
   PAL_ProcessEvent();
   PAL_ClearKeyState();

   dwBeginTime = UTIL_GetTicks();

   srcrect.x = 0;
   srcrect.w = SCREEN_W;
   dstrect.x = 0;
   dstrect.w = SCREEN_W;

   while (true)
   {
      PAL_ProcessEvent();
      dwTime = UTIL_GetTicks() - dwBeginTime;

      // Set the palette
      if (dwTime < 15000) {
        for (i = 0; i < 256 * 3; i++) {
          rgCurrentPalette[i] = (unsigned char)(((unsigned int)palette[i] * dwTime) / 15000U);
        }
        VIDEO_SetPalette(rgCurrentPalette);
      } else
        VIDEO_SetPalette(palette);

      // Draw the screen
      if (iImgPos)
         iImgPos--;

      // The lower part...
      srcrect.y = 0;
      dstrect.y = SCREEN_H - iImgPos;
      srcrect.h = iImgPos;
      dstrect.h = srcrect.h;
      if (srcrect.h && dstrect.h)
        VIDEO_CopySurface(lpBitmapDown, &srcrect, gpScreen, &dstrect);
      // The upper part...
      srcrect.y = iImgPos;
      dstrect.y = 0;
      srcrect.h = SCREEN_H - iImgPos;
      dstrect.h = srcrect.h;
      if (srcrect.h && dstrect.h)
        VIDEO_CopySurface(lpBitmapUp, &srcrect, gpScreen, &dstrect);

      // Draw the cranes...
      for (i = 0; i < 9; i++) {
         const unsigned char *lpFrame = PAL_SpriteGetFrame(lpSpriteCrane, cranepos[i][2]);
         PAL_RLEBlitToSurface(lpFrame, gpScreen, PAL_XY(cranepos[i][0], cranepos[i][1]));
         cranepos[i][0]--;
         cranepos[i][1] +=(iImgPos & 1) ? 1 : 0;
         if (iCraneFrame & 1)
            cranepos[i][2] = (cranepos[i][2] + 1) % 8;
      }
      iCraneFrame++;

      // Draw the title...
      if (PAL_RLEGetHeight(lpBitmapTitle) < iTitleHeight)
      {
         // HACKHACK
         unsigned short w = lpBitmapTitle[2] | (lpBitmapTitle[3] << 8);
         w++;
         lpBitmapTitle[2] = (w & 0xFF);
         lpBitmapTitle[3] = (w >> 8);
      }

      PAL_RLEBlitToSurface(lpBitmapTitle, gpScreen, PAL_XY(255, 10));
      VIDEO_UpdateScreen(NULL);

      // Check for keypress...
      if (PAL_GetKeyInput() & (kKeyMenu | kKeySearch))
      {
         // User has pressed a key...
         lpBitmapTitle[2] = iTitleHeight & 0xFF;
         lpBitmapTitle[3] = iTitleHeight >> 8; // HACKHACK

         PAL_RLEBlitToSurface(lpBitmapTitle, gpScreen, PAL_XY(255, 10));

         VIDEO_UpdateScreen(NULL);

         // If the picture has not completed fading in, complete the rest
         while (dwTime < 15000)
         {
            for (i = 0; i < 256 * 3; i++)
            {
               rgCurrentPalette[i] = (unsigned char)(palette[i] * ((float)dwTime / 15000));
            }
            VIDEO_SetPalette(rgCurrentPalette);
            UTIL_Delay(8);
            dwTime += 250;
         }
         if (dwTime < 15250)
            UTIL_Delay(500);

         // Quit the splash screen
         break;
      }

      // Delay a while...
      do {
        PAL_ProcessEvent();
        UTIL_Sleep(1);
      } while (UTIL_GetTicks() < dwTime + dwBeginTime + 85);
   }

   UTIL_free(lpTitleBuf);
   UTIL_free(lpSpriteCrane);

   AUDIO_PlayMusic(0x00, false, 1);

   PAL_FadeOut(1);
}

int main(int argc, char *argv[])
/*++
  Purpose:

    Program entry.

  Parameters:

    argc - Number of arguments.

    argv - Array of arguments.

  Return value:

    Integer value.

--*/
{
  if (setjmp(g_exit_jmp_buf) != 0) {
    // A longjmp is made, should exit here
    return g_exit_code;
  }

   // Initialize everything
   PAL_Init();

   // Show the trademark screen and splash screen
   // PAL_TrademarkScreen();
   PAL_SplashScreen();

   // Show the opening menu.
   gpGlobals->bCurrentSaveSlot = (unsigned char)PAL_OpeningMenu();

   // Initialize game data and set the flags to load the game resources.
   PAL_ReloadInNextTick(gpGlobals->bCurrentSaveSlot);

   // Run the main game routine
   while (1)
   {
      // Load the game resources if needed.
      PAL_LoadResources();

      // Clear the input state of previous frame.
      PAL_ClearKeyState();

      // Wait for the time of one frame. Accept input here.
      UTIL_Delay(FRAME_TIME);

      // Run the main frame routine.
      PAL_StartFrame();
   }

   // Should not really reach here...
   assert(false);
   return 255;
}
