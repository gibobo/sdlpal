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
#include <SDL_hints.h>
#include <SDL_render.h>

// The global palette
PAL_Surface         *gpScreen           = NULL; // Screen buffer
PAL_Surface         *gpScreenBak        = NULL; // Backup screen buffer
SDL_Window          *gpWindow           = NULL;
SDL_Renderer        *gpRenderer         = NULL;

volatile unsigned char g_bRenderPaused = FALSE;
static unsigned short g_wShakeTime = 0;
static unsigned short g_wShakeLevel = 0;
static unsigned char *bufScreenReal = NULL;
static unsigned char *bufPalette = NULL;

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
    SDL_RendererInfo rendererInfo;
#ifdef GLES
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengles2");
#if SDL_VIDEO_OPENGL_EGL && (SDL_VIDEO_DRIVER_EMSCRIPTEN || SDL_VIDEO_DRIVER_WINRT)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif
#else
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
#endif

    // Before we can render anything, we need a window and a renderer.
    gpWindow = SDL_CreateWindow(NULL,
                                SDL_WINDOWPOS_UNDEFINED,
                                SDL_WINDOWPOS_UNDEFINED,
                                gConfig.dwTextureWidth, gConfig.dwTextureHeight,
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

    SDL_GetRendererInfo(gpRenderer, &rendererInfo);
    VIDEO_GLSL_Setup(rendererInfo.name);

    //
    // Create the screen buffer and the backup screen buffer.
    //
    gpScreen = VIDEO_CreateCompatibleSizedSurface(NULL);
    gpScreenBak = VIDEO_CreateCompatibleSizedSurface(NULL);
    bufScreenReal = malloc(SCREEN_W * 3 * SCREEN_H);

    // Create palette object
    bufPalette = malloc(256 * 3);

    // Failed?
    if (gpScreen == NULL || gpScreenBak == NULL || bufScreenReal == NULL || bufPalette == NULL)
    {
        VIDEO_Shutdown();
        return -2;
    }

    int w, h;
    SDL_GetRendererOutputSize(gpRenderer, &w, &h);
    VIDEO_Resize(w, h);
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

    PAL_FreeSurface(gpScreen);
    gpScreen = NULL;

    PAL_FreeSurface(gpScreenBak);
    gpScreenBak = NULL;

    if (bufScreenReal != NULL)
    {
        free(bufScreenReal);
    }
    bufScreenReal = NULL;

    if (bufPalette != NULL)
    {
        free(bufPalette);
    }
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
   SDL_Rect        srcrect, dstrect;

   if (g_bRenderPaused)
   {
       return;
   }

   if (lpRect != NULL)
   {
       for (int j = lpRect->y; j < lpRect->y + lpRect->h; j++)
       {
           unsigned char *src = gpScreen->pixels + j * SCREEN_W;
           unsigned char *dst = bufScreenReal + j * (SCREEN_W * 3);
           for (int i = lpRect->x; i < lpRect->x + lpRect->w; i++)
           {
             dst[i * 3 + 0] = bufPalette[src[i] * 3 + 0];
             dst[i * 3 + 1] = bufPalette[src[i] * 3 + 1];
             dst[i * 3 + 2] = bufPalette[src[i] * 3 + 2];
           }
       }
   }
   else if (g_wShakeTime != 0)
   {
      // Shake the screen
      srcrect.x = 0;
      srcrect.y = 0;
      srcrect.w = SCREEN_W;
      srcrect.h = SCREEN_H - g_wShakeLevel;

      dstrect.x = 0;
      dstrect.y = 0;
      dstrect.w = SCREEN_W;
      dstrect.h = SCREEN_H - g_wShakeLevel;

      if (g_wShakeTime & 1)
      {
         srcrect.y = g_wShakeLevel;
      }
      else
      {
         dstrect.y = g_wShakeLevel;
      }

      unsigned char *src = gpScreen->pixels;
      unsigned char *dst = bufScreenReal;
      int sx, sy, sw = SCREEN_W;
      int dx, dy, dw = SCREEN_W * 3;
      for (dy = dstrect.y; dy < dstrect.y + dstrect.h; dy++)
      {
          sy = (dy * srcrect.h) / dstrect.h;
          for (dx = dstrect.x; dx < dstrect.x + dstrect.w; dx++)
          {
              sx = (dx * srcrect.w) / dstrect.w;
              unsigned char val = src[sy * sw + sx];
              unsigned int i = dy * dw + dx * 3;
              dst[i + 0] = bufPalette[val * 3 + 0];
              dst[i + 1] = bufPalette[val * 3 + 1];
              dst[i + 2] = bufPalette[val * 3 + 2];
          }
      }

      if (g_wShakeTime & 1)
      {
         dstrect.y = 200 - g_wShakeLevel;
      }
      else
      {
         dstrect.y = 0;
      }

      dstrect.h = g_wShakeLevel;

      memset(bufScreenReal + dstrect.y * (SCREEN_W * 3), 0, dstrect.h * (SCREEN_W * 3));

      g_wShakeTime--;
   }
   else
   {
      unsigned char *src = gpScreen->pixels;
      unsigned char *dst = bufScreenReal;
      for (int i = 0; i < SCREEN_W * SCREEN_H; i++, src++, dst += 3)
      {
         dst[0] = bufPalette[(*src) * 3 + 0];
         dst[1] = bufPalette[(*src) * 3 + 1];
         dst[2] = bufPalette[(*src) * 3 + 2];
      }
   }

   VIDEO_GLSL_RenderCopy(bufScreenReal);
   SDL_GL_SwapWindowWithResult(gpWindow);
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

   wSpeed++;
   wSpeed *= 10;

   unsigned char *src = gpScreen->pixels;
   unsigned char *srcBak = gpScreenBak->pixels;
   unsigned char *dst = bufScreenReal;
   for (i = 0; i < 6; i++)
   {
       // Draw the backup buffer to the screen
       for (j = 0; j < SCREEN_W * SCREEN_H; j++)
       {
           if (j % 6 == rgIndex[i])
               srcBak[j] = src[j];
           dst[j * 3 + 0] = bufPalette[srcBak[j]*3+0];
           dst[j * 3 + 1] = bufPalette[srcBak[j]*3+1];
           dst[j * 3 + 2] = bufPalette[srcBak[j]*3+2];
       }

       VIDEO_GLSL_RenderCopy(bufScreenReal);
       SDL_GL_SwapWindowWithResult(gpWindow);

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
   unsigned int      i, j, k;
   unsigned int      time;
   unsigned char     a, b;
   const unsigned int rgIndex[6] = {0, 3, 1, 5, 2, 4};

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
         for (k = rgIndex[j]; k < SCREEN_W * SCREEN_H; k += 6)
         {
            a = gpScreen->pixels[k];
            b = gpScreenBak->pixels[k];

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
            gpScreenBak->pixels[k] = ((a & 0xF0) | (b & 0x0F));
         }

         // Draw the backup buffer to the screen
         if (g_wShakeTime != 0)
         {
            // Shake the screen
            PAL_Rect srcrect = {0, 0, SCREEN_W, SCREEN_H - g_wShakeLevel};
            PAL_Rect dstrect = {0, 0, SCREEN_W, SCREEN_H - g_wShakeLevel};

            if (g_wShakeTime & 1)
            {
               srcrect.y = g_wShakeLevel;
            }
            else
            {
               dstrect.y = g_wShakeLevel;
            }

            unsigned char *src = gpScreenBak->pixels;
            unsigned char *dst = bufScreenReal;
            int sx, sy, sw = SCREEN_W;
            int dx, dy, dw = (SCREEN_W * 3);
            for (dy = dstrect.y; dy < dstrect.y + dstrect.h; dy++)
            {
                sy = (dy * srcrect.h) / dstrect.h;
                for (dx = dstrect.x; dx < dstrect.x + dstrect.w; dx++)
                {
                    sx = (dx * srcrect.w) / dstrect.w;
                    unsigned char val = src[sy * sw + sx];
                    unsigned int i = dy * dw + dx * 3;
                    dst[i + 0] = bufPalette[val*3+0];
                    dst[i + 1] = bufPalette[val*3+1];
                    dst[i + 2] = bufPalette[val*3+2];
                }
            }

            if (g_wShakeTime & 1)
            {
               dstrect.y = 200 - g_wShakeLevel;
            }
            else
            {
               dstrect.y = 0;
            }

            dstrect.h = g_wShakeLevel;

            memset(bufScreenReal + dstrect.y * (SCREEN_W * 3), 0, dstrect.h * (SCREEN_W * 3));
            VIDEO_GLSL_RenderCopy(bufScreenReal);
            g_wShakeTime--;
         }
         else
         {
             unsigned char *src = gpScreenBak->pixels;
             unsigned char *dst = bufScreenReal;
             for (int j = 0; j < SCREEN_W * SCREEN_H; j++, src++, dst += 3)
             {
                 dst[0] = bufPalette[(*src)*3+0];
                 dst[1] = bufPalette[(*src)*3+1];
                 dst[2] = bufPalette[(*src)*3+2];
             }
             VIDEO_GLSL_RenderCopy(dst);
         }
         SDL_GL_SwapWindowWithResult(gpWindow);
      }
   }

   // Draw the result buffer to the screen as the final step
   VIDEO_UpdateScreen(NULL);
}

void VIDEO_SetWindowTitle(const char *pszTitle)
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
    PAL_Surface *dest;
    dest = malloc(sizeof(PAL_Surface));
    dest->w = pSize ? pSize->w : SCREEN_W;
    dest->h = pSize ? pSize->h : SCREEN_H;
    dest->pixels = (unsigned char *)malloc(dest->w * dest->h);
    return dest;
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
VIDEO_RenderPaused(
    unsigned char flag
)
{
   g_bRenderPaused = flag;
}

void VIDEO_CopySurface(
    PAL_Surface *src,
    const PAL_Rect *srcrect,
    PAL_Surface *dst,
    PAL_Rect *dstrect) {
    unsigned int sr_x = (srcrect) ? (srcrect->x) : 0;
    unsigned int sr_y = (srcrect) ? (srcrect->y) : 0;
    unsigned int sr_w = (srcrect) ? (srcrect->w) : src->w;
    unsigned int sr_h = (srcrect) ? (srcrect->h) : src->h;
    unsigned int dr_x = (dstrect) ? (dstrect->x) : 0;
    unsigned int dr_y = (dstrect) ? (dstrect->y) : 0;
    unsigned int dr_w = (dstrect) ? (dstrect->w) : dst->w;
    unsigned int dr_h = (dstrect) ? (dstrect->h) : dst->h;

    unsigned char *p_src = src->pixels + sr_y * src->w + sr_x;
    unsigned char *p_dst = dst->pixels + dr_y * dst->w + dr_x;
    unsigned int dx, dy;
    unsigned int sx, sy;
    for (dy = 0; dy < dr_h; dy++)
    {
        sy = (dy * sr_h) / dr_h;
        for (dx = 0; dx < dr_w; dx++)
        {
            sx = (dx * sr_w) / dr_w;
            p_dst[dx + dy * dst->w] = p_src[sx + sy * src->w];
        }
    }
}

void VIDEO_CopyEntireSurface(
    PAL_Surface *src,
    PAL_Surface *dst) {
    memcpy(dst->pixels, src->pixels, dst->w * dst->h);
}

void VIDEO_BackupScreen(PAL_Surface *src) {
    memcpy(gpScreenBak->pixels, src->pixels, SCREEN_W * SCREEN_H);
}

void VIDEO_RestoreScreen(PAL_Surface *dst) {
    memcpy(dst->pixels, gpScreenBak->pixels, SCREEN_W * SCREEN_H);
}

void PAL_FreeSurface(PAL_Surface *surface) {
  if (surface) {
    if (surface->pixels)
      free(surface->pixels);
    free(surface);
  }
}

void PAL_CleanScreen(void) {
    memset(gpScreen->pixels, 0, SCREEN_W * SCREEN_H);
}