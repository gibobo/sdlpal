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
#include <SDL.h>

#include "audio/audio.h"
#include "common.h"
#include "font.h"
#include "game.h"
#include "global.h"
#include "input/input.h"
#include "main.h"
#include "palcfg.h"
#include "palcommon.h"
#include "palette.h"
#include "res.h"
#include "rngplay.h"
#include "text.h"
#include "util.h"
#include "video/video.h"
#include <setjmp.h>

static jmp_buf g_exit_jmp_buf;
static int g_exit_code = 0;

static void
PAL_Init(
    void)
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

   //
   // Initialize subsystems.
   //
   e = PAL_InitGlobals();
   if (e != 0)
   {
		PAL_Shutdown(255);
      TerminateOnError("Could not initialize global data: %d.\n", e);
   }

   e = VIDEO_Startup();
   if (e != 0)
   {
		PAL_Shutdown(255);
      TerminateOnError("Could not initialize Video: %d.\n", e);
   }

   VIDEO_SetWindowTitle("Loading...");

   e = PAL_InitUI();
   if (e != 0)
   {
		PAL_Shutdown(255);
      TerminateOnError("Could not initialize UI subsystem: %d.\n", e);
   }

   e = PAL_InitText();
   if (e != 0)
   {
		PAL_Shutdown(255);
      TerminateOnError("Could not initialize text subsystem: %d.\n", e);
   }

   PAL_InitFont();

   PAL_InitInput();
   PAL_InitResources();
   AUDIO_OpenDevice();

   VIDEO_SetWindowTitle(UTIL_va(UTIL_GlobalBuffer(0), 32, "Pal"));
}

void PAL_Shutdown(
    int exit_code)
/*++
  Purpose:

    Free everything needed by the game.

  Parameters:

    exit_code -  The exit code return to OS.

  Return value:

    None.

--*/
{
   AUDIO_CloseDevice();
   PAL_FreeResources();
   PAL_FreeUI();
   PAL_FreeText();
   PAL_ShutdownInput();
   VIDEO_Shutdown();

   //
   // global needs be free in last
   // since subsystems may needs config content during destroy
   // which also cleared here
   //
   PAL_FreeGlobals();

   g_exit_code = exit_code;
   longjmp(g_exit_jmp_buf, 1);
}

void PAL_TrademarkScreen(
    void)
/*++
  Purpose:

    Show the trademark screen.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   PAL_SetPalette(3, FALSE);
   PAL_RNGPlay(6, 0, -1, 25);
   UTIL_Delay(1000);
   PAL_FadeOut(1);
}

void PAL_SplashScreen(
    void)
/*++
  Purpose:

    Show the splash screen.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   PAL_Color *palette = PAL_GetPalette(1, FALSE);
   PAL_Color rgCurrentPalette[256];
   PAL_Surface *lpBitmapDown;
   PAL_Surface *lpBitmapUp;
   PAL_Rect srcrect, dstrect;
   unsigned char *lpSpriteCrane;
   unsigned char *lpBitmapTitle;
   unsigned char *buf;
   unsigned char *buf2;
   int cranepos[9][3], i, iImgPos = 200, iCraneFrame = 0, iTitleHeight;
   unsigned int dwTime, dwBeginTime;

   if (palette == NULL)
   {
      fprintf(stderr, "ERROR: PAL_SplashScreen(): palette == NULL\n");
      return;
   }

   // Allocate all the needed memory at once for simplification
   buf = (unsigned char *)UTIL_calloc(1, 320 * 200 * 2);
   buf2 = &buf[320 * 200];
   lpSpriteCrane = (unsigned char *)buf2 + 32000;

   //
   // Create the surfaces
   //
   lpBitmapDown = VIDEO_CreateCompatibleSizedSurface(NULL);
   lpBitmapUp = VIDEO_CreateCompatibleSizedSurface(NULL);

   //
   // Read the bitmaps
   //
   PAL_MKFReadChunk(buf, 320 * 200, 0x03, gpGlobals->f.fpFBP);
   Decompress(buf, buf2, 320 * 200);
   PAL_FBPBlitToSurface(buf2, lpBitmapUp);

   PAL_MKFReadChunk(buf, 320 * 200, 0x04, gpGlobals->f.fpFBP);
   Decompress(buf, buf2, 320 * 200);
   PAL_FBPBlitToSurface(buf2, lpBitmapDown);

   PAL_MKFReadChunk(buf, 32000, 0x47, gpGlobals->f.fpMGO);
   Decompress(buf, buf2, 32000);
   lpBitmapTitle = (unsigned char *)PAL_SpriteGetFrame(buf2, 0);

   PAL_MKFReadChunk(buf, 32000, 0x49, gpGlobals->f.fpMGO);
   Decompress(buf, lpSpriteCrane, 32000);

   iTitleHeight = PAL_RLEGetHeight(lpBitmapTitle);
   lpBitmapTitle[2] = 0;
   lpBitmapTitle[3] = 0; // HACKHACK

   //
   // Generate the positions of the cranes
   //
   for (i = 0; i < 9; i++)
   {
      cranepos[i][0] = RandomLong(300, 600);
      cranepos[i][1] = RandomLong(0, 80);
      cranepos[i][2] = RandomLong(0, 8);
   }

   //
   // Play the title music
   //
   AUDIO_PlayMusic(-1, FALSE, 0);
   AUDIO_PlayMusic(0x05, TRUE, 2);

   //
   // Clear all of the events and key states
   //
   PAL_ProcessEvent();
   PAL_ClearKeyState();

   dwBeginTime = UTIL_GetTicks();

   srcrect.x = 0;
   srcrect.w = 320;
   dstrect.x = 0;
   dstrect.w = 320;

   while (TRUE)
   {
      PAL_ProcessEvent();
      dwTime = UTIL_GetTicks() - dwBeginTime;

      //
      // Set the palette
      //
      if (dwTime < 15000)
      {
         for (i = 0; i < 256; i++)
         {
            rgCurrentPalette[i].r = (unsigned char)(((int)palette[i].r * dwTime) / 15000);
            rgCurrentPalette[i].g = (unsigned char)(((int)palette[i].g * dwTime) / 15000);
            rgCurrentPalette[i].b = (unsigned char)(((int)palette[i].b * dwTime) / 15000);
         }
      }

      VIDEO_SetPalette(rgCurrentPalette);
      VIDEO_UpdateSurfacePalette(lpBitmapDown);
      VIDEO_UpdateSurfacePalette(lpBitmapUp);

      //
      // Draw the screen
      //
      if (iImgPos > 1)
      {
         iImgPos--;
      }

      //
      // The upper part...
      //
      srcrect.y = iImgPos;
      srcrect.h = 200 - iImgPos;

      dstrect.y = 0;
      dstrect.h = srcrect.h;

      VIDEO_CopySurface(lpBitmapUp, &srcrect, gpScreen, &dstrect);

      // The lower part...
      srcrect.y = 0;
      srcrect.h = iImgPos;

      dstrect.y = 200 - iImgPos;
      dstrect.h = srcrect.h;

      VIDEO_CopySurface(lpBitmapDown, &srcrect, gpScreen, &dstrect);

      // Draw the cranes...
      for (i = 0; i < 9; i++)
      {
         const unsigned char *lpFrame = PAL_SpriteGetFrame(lpSpriteCrane,
                                                           cranepos[i][2] = (cranepos[i][2] + (iCraneFrame & 1)) % 8);
         cranepos[i][1] += ((iImgPos > 1) && (iImgPos & 1)) ? 1 : 0;
         PAL_RLEBlitToSurface(lpFrame, gpScreen,
                              PAL_XY(cranepos[i][0], cranepos[i][1]));
         cranepos[i][0]--;
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
            for (i = 0; i < 256; i++)
            {
               rgCurrentPalette[i].r = (unsigned char)(palette[i].r * ((float)dwTime / 15000));
               rgCurrentPalette[i].g = (unsigned char)(palette[i].g * ((float)dwTime / 15000));
               rgCurrentPalette[i].b = (unsigned char)(palette[i].b * ((float)dwTime / 15000));
            }
            VIDEO_SetPalette(rgCurrentPalette);
            VIDEO_UpdateSurfacePalette(lpBitmapDown);
            VIDEO_UpdateSurfacePalette(lpBitmapUp);
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

   PAL_FreeSurface(lpBitmapDown);
   PAL_FreeSurface(lpBitmapUp);
   free(buf);

   AUDIO_PlayMusic(0x00, FALSE, 1);

   PAL_FadeOut(1);
}

int main(
    int argc,
    char *argv[])
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
   if (setjmp(g_exit_jmp_buf) != 0)
   {
      // A longjmp is made, should exit here
      SDL_Quit();
      UTIL_Platform_Quit();
      return g_exit_code;
   }

   //
   // Initialize SDL
   //
#ifdef PAL_HAS_JOYSTICKS
   if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE | SDL_INIT_JOYSTICK) == -1)
#else
   if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE) == -1)
#endif
   {
      TerminateOnError("Could not initialize SDL: %s.\n", SDL_GetError());
   }

   PAL_LoadConfig();

   // Initialize everything
   PAL_Init();

   // Show the trademark screen and splash screen
   PAL_TrademarkScreen();
   PAL_SplashScreen();

   //
   // Run the main game routine
   //
   PAL_GameMain();

   //
   // Should not really reach here...
   //
   assert(FALSE);
   return 255;
}
