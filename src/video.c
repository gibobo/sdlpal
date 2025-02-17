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

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

// The global palette
PAL_Surface *gpScreen = NULL;                  // Screen buffer
static PAL_Surface *gpBackup[] = {NULL, NULL}; // Backup screen buffer
volatile unsigned char g_bRenderPaused = false;
static unsigned short g_wShakeTime = 0;
static unsigned short g_wShakeLevel = 0;
static unsigned char *bufScreenReal = NULL;
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
   bufScreenReal = (unsigned char *)UTIL_malloc(SCREEN_SIZE * 3);

   // Failed?
   if (gpScreen == NULL || gpBackup[0] == NULL || gpBackup[1] == NULL || bufScreenReal == NULL || bufPalette == NULL)
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
   // since gConfig is cleared already we'd to detect on side effects
   PAL_FreeSurface(gpScreen);
   PAL_FreeSurface(gpBackup[0]);
   PAL_FreeSurface(gpBackup[1]);
   UTIL_free(bufScreenReal);
   UTIL_free(bufPalette);

   gpScreen = NULL;
   gpBackup[0] = NULL;
   gpBackup[1] = NULL;
   bufScreenReal = NULL;
   bufPalette = NULL;
}

void DRIVER_PreFrameShow(const unsigned char *frame_src, const PAL_Rect *roi_src, const PAL_Rect *roi_dst) {
  if (frame_src) {
      unsigned short x = 0;
      unsigned short y = 0;
      unsigned char *src = (unsigned char *)frame_src;
      unsigned char *dst = bufScreenReal;
      if (roi_src) {
         for (y = roi_src->y; y < (roi_src->y + roi_src->h); y++) {
            for (x = roi_src->x; x < (roi_src->x + roi_src->w); x++) {
               unsigned int offset = x + y * SCREEN_W;
               dst[offset * 3 + 0] = bufPalette[src[offset] * 3 + 0];
               dst[offset * 3 + 1] = bufPalette[src[offset] * 3 + 1];
               dst[offset * 3 + 2] = bufPalette[src[offset] * 3 + 2];
            }
         }
      } else {
         unsigned short roi_x1 = (roi_dst) ? roi_dst->x : 0;
         unsigned short roi_y1 = (roi_dst) ? roi_dst->y : 0;
         unsigned short roi_x2 = (roi_dst) ? (roi_dst->x + roi_dst->w) : SCREEN_W;
         unsigned short roi_y2 = (roi_dst) ? (roi_dst->y + roi_dst->h) : SCREEN_H;
         for (y = 0; y < SCREEN_H; y++) {
            if ((y < roi_y1) || (roi_y2 <= y)) {
               memset(dst, 0, SCREEN_W * 3);
            } else {
               for (x = 0; x < SCREEN_W; x++) {
                  unsigned char flag = (roi_x1 <= x) && (x < roi_x2);
                  dst[x * 3 + 0] = (flag) ? bufPalette[src[x] * 3 + 0] : 0;
                  dst[x * 3 + 1] = (flag) ? bufPalette[src[x] * 3 + 1] : 0;
                  dst[x * 3 + 2] = (flag) ? bufPalette[src[x] * 3 + 2] : 0;
               }
               src += SCREEN_W;
            }
            dst += SCREEN_W * 3;
         }
      }
      DRIVER_FrameShow(bufScreenReal);
  }
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
   int i = 0;
   int j = 0;
   PAL_Rect ROI;
   ROI.x = (lpRect) ? lpRect->x : 0;
   ROI.y = (lpRect) ? lpRect->y : 0;
   ROI.w = (lpRect) ? lpRect->w : SCREEN_W;
   ROI.h = (lpRect) ? lpRect->h : SCREEN_H;
   if (g_bRenderPaused)
      return;

   if (lpRect != NULL) {
     DRIVER_PreFrameShow(gpScreen->pixels, lpRect, NULL);
   } else {
     if (g_wShakeTime != 0) {
       ROI.y = (g_wShakeTime & 1) ? g_wShakeLevel : 0;
       ROI.h -= g_wShakeLevel;
       g_wShakeTime--;
     }
     DRIVER_PreFrameShow(gpScreen->pixels, NULL, &ROI);
   }
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
   VIDEO_UpdateScreen(NULL);
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

      DRIVER_PreFrameShow(gpBackup[0]->pixels, NULL, NULL);
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
   const int gain = 16;
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
         DRIVER_PreFrameShow(gpBackup[0]->pixels, NULL, &ROI);
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

void VIDEO_Resize(int w, int h) {
   DRIVER_FrameResize(w, h);
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
