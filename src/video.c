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
#include "driver.h"
#include "global.h"
#include "input.h"
#include "util.h"
#include <stdbool.h>
#include <string.h>

// The global palette
PAL_Surface *gpScreen = NULL;                  // Screen buffer
static PAL_Surface *gpBackup[] = {NULL, NULL}; // Backup screen buffer
volatile unsigned char g_bRenderPaused = false;
static unsigned short g_wShakeTime = 0;
static unsigned short g_wShakeLevel = 0;
static unsigned char *bufPalette = NULL;

int VIDEO_Startup(void)
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
   // Create palette object
   bufPalette = (unsigned char *)UTIL_malloc(256 * 3);

   // Create the screen buffer and the backup screen buffer.
   gpScreen = VIDEO_CreateCompatibleSizedSurface(NULL);
   gpBackup[0] = VIDEO_CreateCompatibleSizedSurface(NULL);
   gpBackup[1] = VIDEO_CreateCompatibleSizedSurface(NULL);

   // Failed?
   if (gpScreen == NULL || gpBackup[0] == NULL || gpBackup[1] == NULL || bufPalette == NULL)
   {
      VIDEO_Shutdown();
      return -2;
   }

   return 0;
}

void VIDEO_Shutdown(void)
/*++
  Purpose:

    Shutdown the video subsystem.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   PAL_FreeSurface(gpScreen);
   PAL_FreeSurface(gpBackup[0]);
   PAL_FreeSurface(gpBackup[1]);
   UTIL_free(bufPalette);

   gpScreen = NULL;
   gpBackup[0] = NULL;
   gpBackup[1] = NULL;
   bufPalette = NULL;
}

void VIDEO_UpdateScreen(const PAL_Rect *lpRect)
/*++
  Purpose:

    Update the screen area specified by lpRect.

  Parameters:

    [IN]  lpRect - Screen area to update.

  Return value:

    None.

--*/
{
   if (g_bRenderPaused)
      return;

   if (lpRect != NULL) {
      DRIVER_FrameShow(gpScreen->pixels, lpRect->x, lpRect->y, lpRect->w, lpRect->h, 0);
   } else if (g_wShakeTime != 0) {
      g_wShakeTime--;
      DRIVER_FrameShow(gpScreen->pixels, 0, (g_wShakeTime & 0x1) * g_wShakeLevel, SCREEN_W, SCREEN_H - g_wShakeLevel, 1);
   } else
      DRIVER_FrameShow(gpScreen->pixels, 0, 0, SCREEN_W, SCREEN_H, 0);
}

void VIDEO_SetPalette(const unsigned char *rgPalette)
/*++
  Purpose:

    Set the palette of the screen.

  Parameters:

    [IN]  rgPalette - array of 256 colors.

  Return value:

    None.

--*/
{
   memcpy(bufPalette, rgPalette, 256 * 3);
   DRIVER_UpdatePalette(bufPalette);
}

const unsigned char *VIDEO_GetPalette(void)
/*++
  Purpose:

    Get the current palette of the screen.

  Parameters:

    None.

  Return value:

    Pointer to the current palette.

--*/
{
   return (const unsigned char *)bufPalette;
}

void VIDEO_ShakeScreen(unsigned short wShakeTime, unsigned short wShakeLevel)
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

void VIDEO_SwitchScreen(unsigned short wSpeed)
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
   int i, j;
   const int rgIndex[6] = {0, 3, 1, 5, 2, 4};

   wSpeed = (wSpeed + 1) * 10;

   for (i = 0; i < 6; i++) {
      // Draw the backup buffer to the screen
      for (j = rgIndex[i]; j < SCREEN_SIZE; j += 6)
         gpBackup[0]->pixels[j] = gpScreen->pixels[j];
      DRIVER_FrameShow(gpBackup[0]->pixels, 0, 0, SCREEN_W, SCREEN_H, 0);
      UTIL_Delay(wSpeed);
   }
}

void VIDEO_FadeScreen(unsigned short wSpeed)
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
   unsigned short i, j, k;
   const unsigned int rgIndex[6] = {0, 3, 1, 5, 2, 4};
   unsigned char a, b;
   PAL_Rect ROI;

   wSpeed++;
   wSpeed *= 10;

   ROI.x = 0;
   ROI.y = 0;
   ROI.w = SCREEN_W;
   ROI.h = SCREEN_H;

   for (i = 0; i < 12; i++) {
      for (j = 0; j < 6; j++) {
         UTIL_Delay(wSpeed);
         // Draw the backup buffer to the screen
         if (g_wShakeTime != 0) {
            ROI.y = (g_wShakeTime & 1) ? g_wShakeLevel : 0;
            ROI.h -= g_wShakeLevel;
            g_wShakeTime--;
         }

         for (k = rgIndex[j]; k < SCREEN_SIZE; k += 6) {
            // Blend the pixels in the 2 buffers, and put the result into the backup buffer
            a = gpScreen->pixels[k];
            b = gpBackup[0]->pixels[k];
            if (i > 0) {
               if ((a & 0x0F) > (b & 0x0F))
                  b++;
               else if ((a & 0x0F) < (b & 0x0F))
                  b--;
            }
            gpBackup[0]->pixels[k] = (a & 0xF0) | (b & 0x0F);
         }
         DRIVER_FrameShow(gpBackup[0]->pixels, ROI.x, ROI.y, ROI.w, ROI.h, 1);
      }
   }

  // Draw the result buffer to the screen as the final step
  VIDEO_UpdateScreen(NULL);
}

PAL_Surface *VIDEO_CreateCompatibleSizedSurface(const PAL_Rect *pSize)
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
   PAL_Surface *dest = NULL;
   dest = (PAL_Surface *)UTIL_malloc(sizeof(PAL_Surface));
   dest->w = pSize ? max(pSize->w, 0) : SCREEN_W;
   dest->h = pSize ? max(pSize->h, 0) : SCREEN_H;
   if (dest->w && dest->h)
     dest->pixels = (unsigned char *)UTIL_calloc(dest->w * dest->h, sizeof(unsigned char));
   else {
     UTIL_free(dest);
     dest = NULL;
   }

   return dest;
}

PAL_Surface *VIDEO_DuplicateSurface(const PAL_Rect *pRect)
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
   PAL_Surface *dest = VIDEO_CreateCompatibleSizedSurface(pRect);
   VIDEO_CopySurface(gpScreen, pRect, dest, NULL);
   return dest;
}

void VIDEO_RenderPaused(unsigned char flag) {
   g_bRenderPaused = flag;
}

void VIDEO_CopySurface(
   PAL_Surface *src,
   const PAL_Rect *srcrect,
   PAL_Surface *dst,
   PAL_Rect *dstrect) {
   unsigned int sr_x = (srcrect) ? srcrect->x : 0;
   unsigned int sr_y = (srcrect) ? srcrect->y : 0;
   unsigned int sr_w = (srcrect) ? min(src->w, srcrect->x + srcrect->w) - sr_x : src->w;
   unsigned int sr_h = (srcrect) ? min(src->h, srcrect->y + srcrect->h) - sr_y : src->h;
   unsigned int dr_x = (dstrect) ? dstrect->x : 0;
   unsigned int dr_y = (dstrect) ? dstrect->y : 0;
   unsigned char *p_src = src->pixels + sr_y * src->w + sr_x;
   unsigned char *p_dst = dst->pixels + dr_y * dst->w + dr_x;
   for (unsigned int dy = 0; dy < sr_h; dy++) {
      memcpy(p_dst + dy * dst->w, p_src + dy * src->w, sr_w);
   }
}

void VIDEO_CopyEntireSurface(PAL_Surface *src, PAL_Surface *dst) {
   if (src && dst && dst != src)
      memcpy(dst->pixels, src->pixels, dst->w * dst->h);
}

void VIDEO_BackupScreen(PAL_Surface *src) {
   if (src)
      memcpy(gpBackup[0]->pixels, src->pixels, SCREEN_SIZE);
}

void VIDEO_RestoreScreen(PAL_Surface *dst) {
   if (dst)
      memcpy(dst->pixels, gpBackup[0]->pixels, SCREEN_SIZE);
}

void PAL_FreeSurface(PAL_Surface *surface) {
   if (surface) {
      UTIL_free(surface->pixels);
      UTIL_free(surface);
   }
}

void PAL_CleanScreen(void) {
   memset(gpScreen->pixels, 0, SCREEN_SIZE);
}

PAL_Surface *VIDEO_GetBackupSurface(unsigned char idx) {
  return (idx < 2) ? gpBackup[idx] : NULL;
}
