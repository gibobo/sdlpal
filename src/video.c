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
VIDEO_Surface *gpScreen = NULL;                  // Screen buffer
static VIDEO_Surface *gpBackup[] = {NULL, NULL}; // Backup screen buffer
volatile uint8_t g_bRenderPaused = false;
static uint16_t g_wShakeTime = 0;
static uint16_t g_wShakeLevel = 0;

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
    if (gpScreen == NULL)
    {
        return -1; // Fail to create screen buffer
    }

    // Create backup buffers
    gpBackup[0] = VIDEO_CreateCompatibleSizedSurface(NULL);
    gpBackup[1] = VIDEO_CreateCompatibleSizedSurface(NULL);
    if (gpBackup[0] == NULL || gpBackup[1] == NULL)
    {
        return -2; // Fail to create backup buffers
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
    VIDEO_FreeSurface(gpScreen);
    VIDEO_FreeSurface(gpBackup[0]);
    VIDEO_FreeSurface(gpBackup[1]);

    gpScreen = NULL;
    gpBackup[0] = NULL;
    gpBackup[1] = NULL;
}

void VIDEO_UpdateScreen(const VIDEO_Rect *lpRect)
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

    if (lpRect != NULL)
    {
        DRIVER_FrameShow(gpScreen->pixels, lpRect->x, lpRect->y, lpRect->w, lpRect->h, false);
    }
    else if (g_wShakeTime != 0)
    {
        g_wShakeTime--;
        DRIVER_FrameShow(gpScreen->pixels, 0, (uint16_t)((g_wShakeTime & 0x1) * g_wShakeLevel), SCREEN_W, (uint16_t)(SCREEN_H - g_wShakeLevel), true);
    }
    else
        DRIVER_FrameShow(gpScreen->pixels, 0, 0, SCREEN_W, SCREEN_H, false);
}

void VIDEO_ShakeScreen(uint16_t wShakeTime, uint16_t wShakeLevel)
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

void VIDEO_SwitchScreen(void)
/*++
  Purpose:

    Switch the screen from the backup screen buffer to the current screen buffer.
    NOTE: This will destroy the backup buffer.

  Parameters:

    None.

  Return value:

    None.

--*/
{
    int32_t i, j;
    const int rgIndex[6] = {0, 3, 1, 5, 2, 4};

    for (i = 0; i < 6; i++)
    {
        // Draw the backup buffer to the screen
        for (j = rgIndex[i]; j < SCREEN_SIZE; j += 6)
            gpBackup[0]->pixels[j] = gpScreen->pixels[j];
        DRIVER_FrameShow(gpBackup[0]->pixels, 0, 0, SCREEN_W, SCREEN_H, false);
        UTIL_Delay(60);
    }
}

void VIDEO_FadeScreen(uint16_t wSpeed)
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
    uint32_t i, j, k;
    const uint32_t rgIndex[6] = {0, 3, 1, 5, 2, 4};
    uint8_t a, b;
    VIDEO_Rect ROI = {0, 0, SCREEN_W, SCREEN_H};

    wSpeed = (wSpeed + 1) * 10;

    for (i = 0; i < 12; i++)
    {
        for (j = 0; j < 6; j++)
        {
            // Draw the backup buffer to the screen
            if (g_wShakeTime != 0)
            {
                ROI.y = (g_wShakeTime & 1) ? g_wShakeLevel : 0;
                ROI.h = SCREEN_H - g_wShakeLevel;
                g_wShakeTime--;
            }

            for (k = rgIndex[j]; k < SCREEN_SIZE; k += 6)
            {
                // Blend the pixels in the 2 buffers, and put the result into the backup buffer
                a = gpScreen->pixels[k];
                b = gpBackup[0]->pixels[k];
                if (i > 0)
                {
                    if ((a & 0x0F) > (b & 0x0F))
                        b++;
                    else if ((a & 0x0F) < (b & 0x0F))
                        b--;
                }
                gpBackup[0]->pixels[k] = (a & 0xF0) | (b & 0x0F);
            }
            DRIVER_FrameShow(gpBackup[0]->pixels, ROI.x, ROI.y, ROI.w, ROI.h, true);
            UTIL_Delay(wSpeed);
        }
    }

    // Draw the result buffer to the screen as the final step
    VIDEO_UpdateScreen(NULL);
}

VIDEO_Surface *VIDEO_CreateCompatibleSizedSurface(const VIDEO_Rect *pSize)
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
    VIDEO_Surface *dest = NULL;
    dest = (VIDEO_Surface *)UTIL_malloc(sizeof(VIDEO_Surface));
    dest->w = pSize ? max((uint16_t)pSize->w, 0) : SCREEN_W;
    dest->h = pSize ? max((uint16_t)pSize->h, 0) : SCREEN_H;
    if (dest->w && dest->h)
        dest->pixels = (uint8_t *)UTIL_calloc(dest->w * dest->h, sizeof(uint8_t));
    else
    {
        UTIL_free(dest);
        dest = NULL;
    }

    return dest;
}

VIDEO_Surface *VIDEO_DuplicateSurface(const VIDEO_Rect *pRect)
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
    VIDEO_Surface *dest = VIDEO_CreateCompatibleSizedSurface(pRect);
    VIDEO_CopySurface(gpScreen, pRect, dest, NULL);
    return dest;
}

void VIDEO_RenderPaused(uint8_t flag)
{
    g_bRenderPaused = flag;
}

void VIDEO_CopySurface(
    VIDEO_Surface *src,
    const VIDEO_Rect *srcrect,
    VIDEO_Surface *dst,
    VIDEO_Rect *dstrect)
{
    if (src == NULL || dst == NULL || src->pixels == NULL || dst->pixels == NULL)
        return;

    uint32_t sr_x = (srcrect) ? srcrect->x : 0;
    uint32_t sr_y = (srcrect) ? srcrect->y : 0;
    uint32_t sr_w = (srcrect) ? min(src->w, srcrect->x + srcrect->w) - sr_x : src->w;
    uint32_t sr_h = (srcrect) ? min(src->h, srcrect->y + srcrect->h) - sr_y : src->h;
    uint32_t dr_x = (dstrect) ? dstrect->x : 0;
    uint32_t dr_y = (dstrect) ? dstrect->y : 0;
    uint8_t *p_src = src->pixels + sr_y * src->w + sr_x;
    uint8_t *p_dst = dst->pixels + dr_y * dst->w + dr_x;
    for (uint32_t dy = 0; dy < sr_h; dy++)
    {
        memcpy(p_dst + dy * dst->w, p_src + dy * src->w, sr_w);
    }
}

void VIDEO_CopyEntireSurface(VIDEO_Surface *src, VIDEO_Surface *dst)
{
    if (src && dst && dst != src)
        memcpy(dst->pixels, src->pixels, dst->w * dst->h);
}

void VIDEO_BackupScreen(VIDEO_Surface *src)
{
    if (src)
        memcpy(gpBackup[0]->pixels, src->pixels, SCREEN_SIZE);
}

void VIDEO_RestoreScreen(VIDEO_Surface *dst)
{
    if (dst)
        memcpy(dst->pixels, gpBackup[0]->pixels, SCREEN_SIZE);
}

void VIDEO_FreeSurface(VIDEO_Surface *surface)
{
    if (surface)
    {
        UTIL_free(surface->pixels);
        UTIL_free(surface);
    }
}

void VIDEO_CleanScreen(void)
{
    memset(gpScreen->pixels, 0, SCREEN_SIZE);
}

VIDEO_Surface *VIDEO_GetBackupSurface(uint8_t idx)
{
    return (idx < 2) ? gpBackup[idx] : NULL;
}
