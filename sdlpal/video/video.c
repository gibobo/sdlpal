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

#include "video.h"
#include "common.h"
#include "input/input.h"
#include "mini_glloader.h"
#include "palcfg.h"
#include "util.h"
#include "video_glsl.h"
#include <SDL_render.h>

// The global palette
static SDL_Palette  *gpPalette          = NULL;
PAL_Surface         *gpScreen           = NULL; // Screen buffer
PAL_Surface         *gpScreenBak        = NULL; // Backup screen buffer
PAL_Surface         *gpScreenReal       = NULL; // The real screen surface
SDL_Window          *gpWindow           = NULL;
SDL_Renderer        *gpRenderer         = NULL;

volatile unsigned char g_bRenderPaused = FALSE;
static unsigned short g_wShakeTime = 0;
static unsigned short g_wShakeLevel = 0;

int
VIDEO_Startup(
   void
)
/*++
  Purpose:

    Initialze the video subsystem.

  Parameters:

    None.

  Return value:

    0 = success, -1 = fail to create the screen surface,
    -2 = fail to create screen buffer.

--*/
{
    SDL_Texture *gpTexture = NULL;
    VIDEO_GLSL_Init();

    // Before we can render anything, we need a window and a renderer.
    gpWindow = SDL_CreateWindow(NULL,
                                SDL_WINDOWPOS_UNDEFINED,
                                SDL_WINDOWPOS_UNDEFINED,
                                640,
                                400,
                                SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
    if (gpWindow == NULL)
    {
        return -1;
    }

    gpRenderer = SDL_CreateRenderer(gpWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (gpRenderer == NULL)
    {
        return -1;
    }

    VIDEO_GLSL_Setup();

    //
    // Create the screen buffer and the backup screen buffer.
    //
    gpScreen = (PAL_Surface *)SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 200, 8, 0, 0, 0, 0);
    gpScreenBak = (PAL_Surface *)SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 200, 8, 0, 0, 0, 0);
    gpScreenReal = (PAL_Surface *)SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 200, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

    // Create texture for screen.
    gpTexture = (SDL_Texture *)VIDEO_GLSL_CreateTexture();

    // Create palette object
    gpPalette = SDL_AllocPalette(256);

    // Failed?
    if (gpScreen == NULL || gpScreenBak == NULL || gpScreenReal == NULL || gpTexture == NULL || gpPalette == NULL)
    {
        VIDEO_Shutdown();
        return -2;
    }

    return 0;
}

void
VIDEO_Shutdown(
   void
)
/*++
  Purpose:

    Shutdown the video subsystem.

  Parameters:

    None.

  Return value:

    None.

--*/
{
    // since gConfig is cleared already we'd to detect on side effects
   VIDEO_GLSL_Destroy();

   if (gpScreen != NULL)
   {
      SDL_FreeSurface((SDL_Surface *)gpScreen);
   }
   gpScreen = NULL;

   if (gpScreenBak != NULL)
   {
      SDL_FreeSurface((SDL_Surface *)gpScreenBak);
   }
   gpScreenBak = NULL;

   if (gpRenderer)
   {
      SDL_DestroyRenderer(gpRenderer);
   }
   gpRenderer = NULL;

   if (gpWindow)
   {
      SDL_DestroyWindow(gpWindow);
   }
   gpWindow = NULL;

   if (gpPalette)
   {
      SDL_FreePalette(gpPalette);
   }
   gpPalette = NULL;

   if (gpScreenReal != NULL)
   {
      SDL_FreeSurface((SDL_Surface *)gpScreenReal);
   }
   gpScreenReal = NULL;
}

void
VIDEO_UpdateScreen(
   const PAL_Rect  *lpRect
)
/*++
  Purpose:

    Update the screen area specified by lpRect.

  Parameters:

    [IN]  lpRect - Screen area to update.

  Return value:

    None.

--*/
{
   SDL_Rect        srcrect, dstrect;
   short           offset = 40;
   short           screenRealHeight = gpScreenReal->h;
   short           screenRealY = 0;

   if (g_bRenderPaused)
   {
	   return;
   }

   //
   // Lock surface if needed
   //
   if (SDL_MUSTLOCK(gpScreenReal))
   {
      if (SDL_LockSurface((SDL_Surface *)gpScreenReal) < 0)
         return;
   }

   if (lpRect != NULL)
   {
      dstrect.x = (short)((int)(lpRect->x) * gpScreenReal->w / gpScreen->w);
      dstrect.y = (short)((int)(screenRealY + lpRect->y) * screenRealHeight / gpScreen->h);
      dstrect.w = (unsigned short)((unsigned int)(lpRect->w) * gpScreenReal->w / gpScreen->w);
      dstrect.h = (unsigned short)((unsigned int)(lpRect->h) * screenRealHeight / gpScreen->h);

      SDL_UpperBlit((SDL_Surface *)gpScreen, (SDL_Rect *)lpRect, (SDL_Surface *)gpScreenReal, &dstrect);
   }
   else if (g_wShakeTime != 0)
   {
      //
      // Shake the screen
      //
      srcrect.x = 0;
      srcrect.y = 0;
      srcrect.w = 320;
      srcrect.h = 200 - g_wShakeLevel;

      dstrect.x = 0;
      dstrect.y = screenRealY;
      dstrect.w = 320 * gpScreenReal->w / gpScreen->w;
      dstrect.h = (200 - g_wShakeLevel) * screenRealHeight / gpScreen->h;

      if (g_wShakeTime & 1)
      {
         srcrect.y = g_wShakeLevel;
      }
      else
      {
         dstrect.y = (screenRealY + g_wShakeLevel) * screenRealHeight / gpScreen->h;
      }

      SDL_UpperBlit((SDL_Surface *)gpScreen, &srcrect, (SDL_Surface *)gpScreenReal, &dstrect);

      if (g_wShakeTime & 1)
      {
         dstrect.y = (screenRealY + screenRealHeight - g_wShakeLevel) * screenRealHeight / gpScreen->h;
      }
      else
      {
         dstrect.y = screenRealY;
      }

      dstrect.h = g_wShakeLevel * screenRealHeight / gpScreen->h;

      SDL_FillRect((SDL_Surface *)gpScreenReal, &dstrect, 0);

      g_wShakeTime--;
   }
   else
   {
      dstrect.x = 0;
      dstrect.y = screenRealY;
      dstrect.w = gpScreenReal->w;
      dstrect.h = screenRealHeight;

      SDL_UpperBlit((SDL_Surface *)gpScreen, NULL, (SDL_Surface *)gpScreenReal, &dstrect);
   }

   VIDEO_GLSL_RenderCopy();

   if (SDL_MUSTLOCK(gpScreenReal))
   {
	   SDL_UnlockSurface((SDL_Surface *)gpScreenReal);
   }
}

void
VIDEO_SetPalette(
   PAL_Color       *rgPalette
)
/*++
  Purpose:

    Set the palette of the screen.

  Parameters:

    [IN]  rgPalette - array of 256 colors.

  Return value:

    None.

--*/
{
   PAL_Rect rect;

   SDL_SetPaletteColors(gpPalette, (const SDL_Color *)rgPalette, 0, 256);

   SDL_SetSurfacePalette((SDL_Surface *)gpScreen, gpPalette);
   SDL_SetSurfacePalette((SDL_Surface *)gpScreenBak, gpPalette);

   //
   // HACKHACK: need to invalidate gpScreen->map otherwise the palette
   // would not be effective during blit
   //
   SDL_SetSurfaceColorMod((SDL_Surface *)gpScreen, 0, 0, 0);
   SDL_SetSurfaceColorMod((SDL_Surface *)gpScreen, 0xFF, 0xFF, 0xFF);
   SDL_SetSurfaceColorMod((SDL_Surface *)gpScreenBak, 0, 0, 0);
   SDL_SetSurfaceColorMod((SDL_Surface *)gpScreenBak, 0xFF, 0xFF, 0xFF);

   rect.x = 0;
   rect.y = 0;
   rect.w = 320;
   rect.h = 200;

   VIDEO_UpdateScreen(&rect);
}

PAL_Color *
VIDEO_GetPalette(
   void
)
/*++
  Purpose:

    Get the current palette of the screen.

  Parameters:

    None.

  Return value:

    Pointer to the current palette.

--*/
{
   return (PAL_Color *)gpPalette->colors;
}

void
VIDEO_ShakeScreen(
   unsigned short           wShakeTime,
   unsigned short           wShakeLevel
)
/*++
  Purpose:

    Set the screen shake time and level.

  Parameters:

    [IN]  wShakeTime - how many times should we shake the screen.

    [IN]  wShakeLevel - level of shaking.

  Return value:

    None.

--*/
{
   g_wShakeTime = wShakeTime;
   g_wShakeLevel = wShakeLevel;
}

void
VIDEO_SwitchScreen(
   unsigned short           wSpeed
)
/*++
  Purpose:

    Switch the screen from the backup screen buffer to the current screen buffer.
    NOTE: This will destroy the backup buffer.

  Parameters:

    [IN]  wSpeed - speed of fading (the larger value, the slower).

  Return value:

    None.

--*/
{
   int               i, j;
   const int         rgIndex[6] = {0, 3, 1, 5, 2, 4};
   PAL_Rect          dstrect;
   short             offset = 40;
   short             screenRealHeight = gpScreenReal->h;
   short             screenRealY = 0;

   wSpeed++;
   wSpeed *= 10;

   for (i = 0; i < 6; i++)
   {
      for (j = rgIndex[i]; j < gpScreen->pitch * gpScreen->h; j += 6)
      {
         ((unsigned char *)(gpScreenBak->pixels))[j] = ((unsigned char *)(gpScreen->pixels))[j];
      }

      //
      // Draw the backup buffer to the screen
      //
      dstrect.x = 0;
      dstrect.y = screenRealY;
      dstrect.w = gpScreenReal->w;
      dstrect.h = screenRealHeight;

	  if (SDL_MUSTLOCK(gpScreenReal))
	  {
		  if (SDL_LockSurface((SDL_Surface *)gpScreenReal) < 0)
			  return;
	  }

      VIDEO_CopySurface(gpScreenBak, NULL, gpScreenReal, &dstrect);

      VIDEO_GLSL_RenderCopy();

	  if (SDL_MUSTLOCK(gpScreenReal))
	  {
		  SDL_UnlockSurface((SDL_Surface *)gpScreenReal);
	  }

      UTIL_Delay(wSpeed);
   }
}

void
VIDEO_FadeScreen(
   unsigned short           wSpeed
)
/*++
  Purpose:

    Fade from the backup screen buffer to the current screen buffer.
    NOTE: This will destroy the backup buffer.

  Parameters:

    [IN]  wSpeed - speed of fading (the larger value, the slower).

  Return value:

    None.

--*/
{
   int               i, j, k;
   unsigned int             time;
   unsigned char              a, b;
   const int         rgIndex[6] = {0, 3, 1, 5, 2, 4};
   PAL_Rect          dstrect;
   short             offset = 240 - 200;
   short             screenRealHeight = gpScreenReal->h;
   short             screenRealY = 0;

   //
   // Lock surface if needed
   //
   if (SDL_MUSTLOCK(gpScreenReal))
   {
      if (SDL_LockSurface((SDL_Surface *)gpScreenReal) < 0)
         return;
   }

   time = UTIL_GetTicks();

   wSpeed++;
   wSpeed *= 10;

   for (i = 0; i < 12; i++)
   {
      for (j = 0; j < 6; j++)
      {
         PAL_DelayUntil(time);
         time = UTIL_GetTicks() + wSpeed;

         //
         // Blend the pixels in the 2 buffers, and put the result into the
         // backup buffer
         //
         for (k = rgIndex[j]; k < gpScreen->pitch * gpScreen->h; k += 6)
         {
            a = ((unsigned char *)(gpScreen->pixels))[k];
            b = ((unsigned char *)(gpScreenBak->pixels))[k];

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

            ((unsigned char *)(gpScreenBak->pixels))[k] = ((a & 0xF0) | (b & 0x0F));
         }

         //
         // Draw the backup buffer to the screen
         //
         if (g_wShakeTime != 0)
         {
            //
            // Shake the screen
            //
            PAL_Rect srcrect, dstrect;

            srcrect.x = 0;
            srcrect.y = 0;
            srcrect.w = 320;
            srcrect.h = 200 - g_wShakeLevel;

            dstrect.x = 0;
            dstrect.y = screenRealY;
            dstrect.w = 320 * gpScreenReal->w / gpScreen->w;
            dstrect.h = (200 - g_wShakeLevel) * screenRealHeight / gpScreen->h;

            if (g_wShakeTime & 1)
            {
               srcrect.y = g_wShakeLevel;
            }
            else
            {
               dstrect.y = (screenRealY + g_wShakeLevel) * screenRealHeight / gpScreen->h;
            }

            SDL_UpperBlit((SDL_Surface *)gpScreenBak, (const SDL_Rect *)&srcrect, (SDL_Surface *)gpScreenReal, (SDL_Rect *)&dstrect);

            if (g_wShakeTime & 1)
            {
               dstrect.y = (screenRealY + screenRealHeight - g_wShakeLevel) * screenRealHeight / gpScreen->h;
            }
            else
            {
               dstrect.y = screenRealY;
            }

            dstrect.h = g_wShakeLevel * screenRealHeight / gpScreen->h;

            SDL_FillRect((SDL_Surface *)gpScreenReal, (const SDL_Rect *)&dstrect, 0);
            VIDEO_GLSL_RenderCopy();
            g_wShakeTime--;
         }
         else
         {
            dstrect.x = 0;
            dstrect.y = screenRealY;
            dstrect.w = gpScreenReal->w;
            dstrect.h = screenRealHeight;

            VIDEO_CopySurface(gpScreenBak, NULL, gpScreenReal, &dstrect);
            VIDEO_GLSL_RenderCopy();
         }
      }
   }

   if (SDL_MUSTLOCK((SDL_Surface *)gpScreenReal))
   {
      SDL_UnlockSurface((SDL_Surface *)gpScreenReal);
   }

   //
   // Draw the result buffer to the screen as the final step
   //
   VIDEO_UpdateScreen(NULL);
}

void
VIDEO_SetWindowTitle(
	const char*     pszTitle
)
/*++
  Purpose:

    Set the caption of the window.

  Parameters:

    [IN]  pszTitle - the new caption of the window.

  Return value:

    None.

--*/
{
	SDL_SetWindowTitle(gpWindow, pszTitle);
}

PAL_Surface *
VIDEO_CreateCompatibleSizedSurface(
	const PAL_Rect *pSize
)
/*++
  Purpose:

    Create a surface that compatible with the source surface.

  Parameters:

    [IN]  pSource   - the source surface from which attributes are taken.
    [IN]  pSize     - the size (width & height) of the created surface.

  Return value:

    None.

--*/
{
    // Create the surface
    SDL_Surface *dest = SDL_CreateRGBSurface(
        gpScreen->flags,
        pSize ? pSize->w : gpScreen->w,
        pSize ? pSize->h : gpScreen->h,
        gpScreen->format->BitsPerPixel,
        gpScreen->format->Rmask,
        gpScreen->format->Gmask,
        gpScreen->format->Bmask,
        gpScreen->format->Amask);

    if (dest) {
        SDL_SetSurfacePalette(dest, gpPalette);
    }

    return (PAL_Surface *)dest;
}

PAL_Surface *
VIDEO_DuplicateSurface(
	const PAL_Rect *pRect
)
/*++
  Purpose:

    Duplicate the selected area from the source surface into new surface.

  Parameters:

    [IN]  pSource - the source surface.
	[IN]  pRect   - the area to be duplicated, NULL for entire surface.

  Return value:

    None.

--*/
{
	PAL_Surface* dest = VIDEO_CreateCompatibleSizedSurface(pRect);

	if (dest)
	{
		VIDEO_CopySurface(gpScreen, pRect, dest, NULL);
	}

	return dest;
}

void
VIDEO_UpdateSurfacePalette(
	PAL_Surface    *pSurface
)
/*++
  Purpose:

    Use the global palette to update the palette of pSurface.

  Parameters:

    [IN]  pSurface - the surface whose palette should be updated.

  Return value:

    None.

--*/
{
	SDL_SetSurfacePalette((SDL_Surface *)pSurface, gpPalette);
}


void
VIDEO_RenderPaused(
	unsigned char flag
)
{
   g_bRenderPaused = flag;
}

int VIDEO_CopySurface(
    PAL_Surface *src,
    const PAL_Rect *srcrect,
    PAL_Surface *dst,
    PAL_Rect *dstrect) {
  return SDL_UpperBlit((SDL_Surface *)src, (const SDL_Rect*)srcrect, (SDL_Surface *)dst, (SDL_Rect*)dstrect);
}

int VIDEO_CopyEntireSurface(
    PAL_Surface *src,
    PAL_Surface *dst) {
  return SDL_UpperBlit((SDL_Surface *)src, NULL, (SDL_Surface *)dst, NULL);
}

int VIDEO_BackupScreen(PAL_Surface *src) {
  return SDL_UpperBlit((SDL_Surface *)src, NULL, (SDL_Surface *)gpScreenBak, NULL);
}

int VIDEO_RestoreScreen(PAL_Surface *dst) {
  return SDL_UpperBlit((SDL_Surface *)gpScreenBak, NULL, (SDL_Surface *)dst, NULL);
}

void PAL_FreeSurface(PAL_Surface *surface) {
  SDL_FreeSurface((SDL_Surface *)surface);
}

void PAL_CleanScreen(void) {
  SDL_FillRect((SDL_Surface *)gpScreen, NULL, 0);
}