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
#include "global.h"
#include "input.h"
#include "util.h"
#include "driver.h"

// The global palette
PAL_Surface *gpScreen = NULL;    // Screen buffer
PAL_Surface *gpScreenBak = NULL; // Backup screen buffer
volatile unsigned char g_bRenderPaused = FALSE;
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

    // Create the screen buffer and the backup screen buffer.
    gpScreen = VIDEO_CreateCompatibleSizedSurface(NULL);
    gpScreenBak = VIDEO_CreateCompatibleSizedSurface(NULL);
    bufScreenReal = (unsigned char *)UTIL_malloc(SCREEN_SIZE * 3);

    // Create palette object
    bufPalette = (unsigned char *)UTIL_malloc(256 * 3);

    // Failed?
    if (gpScreen == NULL || gpScreenBak == NULL || bufScreenReal == NULL || bufPalette == NULL)
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
    gpScreen = NULL;

    PAL_FreeSurface(gpScreenBak);
    gpScreenBak = NULL;

    UTIL_free(bufScreenReal);
    bufScreenReal = NULL;

    UTIL_free(bufPalette);
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
    int i = 0;
    int j = 0;
    int roi_h = SCREEN_H;
    unsigned char *src = gpScreen->pixels;
    unsigned char *dst = bufScreenReal;
    if (g_bRenderPaused)
    {
        return;
    }

    if (lpRect != NULL)
    {
        int offset = (lpRect->y * SCREEN_W + lpRect->x);
        src += offset;
        dst += (offset * 3);
        for (j = 0; j < lpRect->h; j++)
        {
            for (i = 0; i < lpRect->w; i++)
            {
                dst[i * 3 + 0] = bufPalette[src[i] * 3 + 0];
                dst[i * 3 + 1] = bufPalette[src[i] * 3 + 1];
                dst[i * 3 + 2] = bufPalette[src[i] * 3 + 2];
            }
            src += SCREEN_W;
            dst += SCREEN_W * 3;
        }
    }
    else
    {
        if (g_wShakeTime != 0)
        {
            // Shake the screen
            roi_h -= g_wShakeLevel;
            if (g_wShakeTime & 1)
            {
                memset(dst + roi_h * SCREEN_W * 3, 0, g_wShakeLevel * SCREEN_W * 3);
                src += (SCREEN_W * g_wShakeLevel);
            }
            else
            {
                memset(dst, 0, g_wShakeLevel * SCREEN_W * 3);
                dst += (g_wShakeLevel * SCREEN_W * 3);
            }
            g_wShakeTime--;
        }

        for (i = 0; i < SCREEN_W * roi_h; i++, src++, dst += 3)
        {
            dst[0] = bufPalette[(*src) * 3 + 0];
            dst[1] = bufPalette[(*src) * 3 + 1];
            dst[2] = bufPalette[(*src) * 3 + 2];
        }
    }

    DRIVER_FrameShow(bufScreenReal);
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
   int               i, j;
   const int         rgIndex[6] = {0, 3, 1, 5, 2, 4};

   wSpeed++;
   wSpeed *= 10;

   unsigned char *src = gpScreen->pixels;
   unsigned char *dst = bufScreenReal;
   for (i = 0; i < 6; i++)
   {
       // Draw the backup buffer to the screen
       for (j = rgIndex[i]; j < SCREEN_SIZE; j += 6)
       {
           dst[j * 3 + 0] = bufPalette[src[j] * 3 + 0];
           dst[j * 3 + 1] = bufPalette[src[j] * 3 + 1];
           dst[j * 3 + 2] = bufPalette[src[j] * 3 + 2];
       }

       DRIVER_FrameShow(bufScreenReal);

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
   unsigned char idx;
   short a, b;

   wSpeed++;
   wSpeed *= 10;

   for (i = 0; i < 12; i++)
   {
      for (j = 0; j < 6; j++)
      {
         UTIL_Delay(wSpeed);

         unsigned int roi_h = SCREEN_H;
         unsigned char *src = gpScreen->pixels;
         unsigned char *dst = bufScreenReal;

         // Draw the backup buffer to the screen
         if (g_wShakeTime != 0)
         {
            roi_h -= g_wShakeLevel;
            if (g_wShakeTime & 1)
            {
               memset(dst + roi_h * SCREEN_W * 3, 0, g_wShakeLevel * SCREEN_W * 3);
               src += (SCREEN_W * g_wShakeLevel);
            }
            else
            {
               memset(dst, 0, g_wShakeLevel * SCREEN_W * 3);
               dst += (g_wShakeLevel * SCREEN_W * 3);
            }
            g_wShakeTime--;
         }

         for (k = rgIndex[j]; k < SCREEN_W * roi_h; k += 6)
         {
            // Blend the pixels in the 2 buffers, and put the result into the backup buffer
            for (idx = 0; idx < 3; idx++)
            {
               a = dst[k * 3 + idx];
               b = bufPalette[src[k] * 3 + idx];
               if (a + gain < b)
                  a += gain;
               else if (a - gain > b)
                  a -= gain;
               else
                  a = b;
               dst[k * 3 + idx] = (unsigned char)a;
            }
         }
         DRIVER_FrameShow(bufScreenReal);
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
    if (dest) {
      dest->w = pSize ? pSize->w : SCREEN_W;
      dest->h = pSize ? pSize->h : SCREEN_H;
      dest->pixels = (unsigned char *)UTIL_calloc(dest->w * dest->h, sizeof(unsigned char));
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
    PAL_Surface* dest = VIDEO_CreateCompatibleSizedSurface(pRect);

    if (dest)
    {
        VIDEO_CopySurface(gpScreen, pRect, dest, NULL);
    }

    return dest;
}

void VIDEO_RenderPaused(unsigned char flag)
{
    g_bRenderPaused = flag;
}

void VIDEO_Resize(int w, int h)
{
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

void VIDEO_CopyEntireSurface(
    PAL_Surface *src,
    PAL_Surface *dst) {
    memcpy(dst->pixels, src->pixels, dst->w * dst->h);
}

void VIDEO_BackupScreen(PAL_Surface *src) {
    memcpy(gpScreenBak->pixels, src->pixels, SCREEN_SIZE);
}

void VIDEO_RestoreScreen(PAL_Surface *dst) {
    memcpy(dst->pixels, gpScreenBak->pixels, SCREEN_SIZE);
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
