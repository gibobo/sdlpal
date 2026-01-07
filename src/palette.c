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
#include "driver.h"
#include "global.h"
#include "input.h"
#include "palcommon.h"
#include "play.h"
#include "resource.h"
#include "scene.h"
#include "util.h"
#include <stdbool.h>
#include <string.h>

static uint8_t gPalette[PALETTE_SIZE];

uint8_t *PAL_GetPalette(int32_t iPaletteNum, uint8_t fNight)
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
    static int iPaletteNum_cached = 0xFFFF;
    static uint8_t fNight_cached = 0xFF;
    if (iPaletteNum == iPaletteNum_cached && fNight == fNight_cached)
        return gPalette;

    iPaletteNum_cached = iPaletteNum;
    fNight_cached = fNight;

    uint8_t buf[PALETTE_SIZE * 2];
    // Read the palette data from the pat.mkf file
    uint32_t i = RES_MKFReadChunk(buf, PALETTE_SIZE * 2, iPaletteNum, Res_PAT);
    if (i == 0)
        return NULL; // Read failed
    else if (i <= PALETTE_SIZE)
        fNight = 0; // There is no night colors in the palette

    uint8_t *ptr = buf + PALETTE_SIZE * ((fNight) ? 1 : 0);

    for (i = 0; i < PALETTE_SIZE; i++)
        gPalette[i] = ptr[i] << 2;

    return gPalette;
}

void PAL_SetPalette(int32_t iPaletteNum, uint8_t fNight)
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
    uint8_t *p = PAL_GetPalette(iPaletteNum, fNight);

    if (p != NULL)
    {
        DRIVER_UpdatePalette(p);
        // VIDEO_UpdateScreen(NULL);
    }
}

void PAL_FadeOut(uint32_t iDelay)
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
    uint32_t i, j;
    uint8_t org_palette[PALETTE_SIZE];
    uint8_t new_palette[PALETTE_SIZE];

    // Get the original palette...
    memcpy(org_palette, gPalette, PALETTE_SIZE);
    memset(new_palette, 0, PALETTE_SIZE);

    //
    // Start fading out...
    //
    for (j = 60; j > 0; j -= (iDelay > 10 ? iDelay / 10 : 1))
    {
        for (i = 0; i < PALETTE_SIZE; i++)
        {
            new_palette[i] = (org_palette[i] * j) >> 6;
        }

        DRIVER_UpdatePalette(new_palette);
        VIDEO_UpdateScreen(NULL);
        UTIL_Delay(iDelay * 10);
    }

    memset(new_palette, 0, sizeof(new_palette));
    DRIVER_UpdatePalette(new_palette);
    VIDEO_UpdateScreen(NULL);
}

void PAL_FadeIn(int32_t iPaletteNum, uint8_t fNight, uint16_t iDelay)
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
    uint16_t i, j;
    uint8_t *palette = PAL_GetPalette(iPaletteNum, fNight);
    uint8_t newpalette[PALETTE_SIZE] = {0};

    // Start fading in...
    for (j = 0; j < 60U; j += (iDelay > 10U ? iDelay / 10U : 1U))
    {
        for (i = 0; i < PALETTE_SIZE; i++)
        {
            newpalette[i] = (uint8_t)(((uint16_t)palette[i] * j) >> 6U);
        }

        DRIVER_UpdatePalette(newpalette);
        VIDEO_UpdateScreen(NULL);
        UTIL_Delay(iDelay * 10U);
    }

    DRIVER_UpdatePalette(palette);
    // VIDEO_UpdateScreen(NULL);
}

void PAL_SceneFade(int32_t iPaletteNum, uint8_t fNight, int iStep)
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
    uint8_t *palette;
    uint8_t newpalette[PALETTE_SIZE];
    int32_t i;
    uint32_t j;

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
            // VIDEO_UpdateScreen(NULL);

            // Calculate the current palette...
            for (j = 0; j < PALETTE_SIZE; j++)
            {
                newpalette[j] = (uint8_t)(((uint32_t)palette[j] * (uint32_t)i) >> 6U);
            }
            DRIVER_UpdatePalette(newpalette);
            VIDEO_UpdateScreen(NULL);

            UTIL_Delay(FRAME_TIME);
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
            // VIDEO_UpdateScreen(NULL);

            // Calculate the current palette...
            for (j = 0; j < PALETTE_SIZE; j++)
            {
                newpalette[j] = (uint8_t)(((uint32_t)palette[j] * (uint32_t)i) >> 6U);
            }
            DRIVER_UpdatePalette(newpalette);
            VIDEO_UpdateScreen(NULL);

            UTIL_Delay(FRAME_TIME);
        }
    }
}

void PAL_PaletteFade(int32_t iPaletteNum, uint8_t fNight, int fUpdateScene)
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
    int32_t i, j;
    uint8_t *new_palette = NULL;
    uint8_t org_palette[PALETTE_SIZE];
    uint8_t tmp_palette[PALETTE_SIZE];

    memcpy(org_palette, gPalette, PALETTE_SIZE);
    memset(tmp_palette, 0xff, PALETTE_SIZE);
    new_palette = PAL_GetPalette(iPaletteNum, fNight);

    // Start fading...
    for (i = 0; i < 32; i++)
    {
        for (j = 0; j < PALETTE_SIZE; j++)
        {
            tmp_palette[j] = (uint8_t)(((int)org_palette[j] * (31 - i) + (int)(new_palette[j]) * i) / 31);
        }
        DRIVER_UpdatePalette(tmp_palette);
        // VIDEO_UpdateScreen(NULL);

        if (fUpdateScene)
        {
            PAL_ClearKeyState();
            PAL_SetDirInput(kDirUnknown);
            PAL_GameUpdate(false);
            PAL_MakeScene();
        }

        VIDEO_UpdateScreen(NULL);
        UTIL_Delay(fUpdateScene ? FRAME_TIME : FRAME_TIME / 4);
    }
}

void PAL_ColorFade(int32_t iDelay, uint8_t bColor, int fFrom)
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
    int32_t i, j;
    uint8_t *org_palette = NULL;
    uint8_t new_palette[PALETTE_SIZE];

    org_palette = PAL_GetPalette(gpGlobals->wNumPalette, gpGlobals->fNightPalette);
    memset(new_palette, 0, sizeof(new_palette));

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
            new_palette[i * 3 + 0] = org_palette[bColor * 3 + 0];
            new_palette[i * 3 + 1] = org_palette[bColor * 3 + 1];
            new_palette[i * 3 + 2] = org_palette[bColor * 3 + 2];
        }

        for (i = 0; i < 64; i++)
        {
            for (j = 0; j < 256; j++)
            {
                Converge(new_palette[j * 3 + 0], org_palette[j * 3 + 0]);
                Converge(new_palette[j * 3 + 1], org_palette[j * 3 + 1]);
                Converge(new_palette[j * 3 + 2], org_palette[j * 3 + 2]);
            }

            DRIVER_UpdatePalette(new_palette);
            VIDEO_UpdateScreen(NULL);
            UTIL_Delay(iDelay);
        }

        DRIVER_UpdatePalette(org_palette);
        // VIDEO_UpdateScreen(NULL);
    }
    else
    {
        memcpy(new_palette, org_palette, sizeof(new_palette));

        for (i = 0; i < 64; i++)
        {
            for (j = 0; j < 256; j++)
            {
                Converge(new_palette[j * 3 + 0], org_palette[bColor * 3 + 0]);
                Converge(new_palette[j * 3 + 1], org_palette[bColor * 3 + 1]);
                Converge(new_palette[j * 3 + 2], org_palette[bColor * 3 + 2]);
            }

            DRIVER_UpdatePalette(new_palette);
            VIDEO_UpdateScreen(NULL);
            UTIL_Delay(iDelay);
        }

        for (i = 0; i < 256; i++)
        {
            new_palette[i] = org_palette[bColor];
        }

        DRIVER_UpdatePalette(new_palette);
        // VIDEO_UpdateScreen(NULL);
    }
}

void PAL_FadeToRed(void)
/*++
  Purpose:

    Fade the whole screen to red color.

  Parameters:

    None.

  Return value:

    None.

--*/
{
    int32_t i, j;
    uint8_t color;
    uint8_t *org_palette = NULL;
    uint8_t new_palette[PALETTE_SIZE];

    org_palette = PAL_GetPalette(gpGlobals->wNumPalette, gpGlobals->fNightPalette);
    memcpy(new_palette, org_palette, PALETTE_SIZE);

    for (i = 0; i < SCREEN_SIZE; i++)
    {
        if (gpScreen->pixels[i] == 0x4F)
        {
            gpScreen->pixels[i] = 0x4E; // HACKHACK
        }
    }

    // VIDEO_UpdateScreen(NULL);

    for (i = 0; i < 32; i++)
    {
        for (j = 0; j < 256; j++)
        {
            if (j == 0x4F)
            {
                continue; // so that texts will not be affected
            }

            color = ((int)org_palette[j * 3 + 0] + (int)org_palette[j * 3 + 1] + (int)org_palette[j * 3 + 2]) / 4 + 64;

            if (new_palette[j * 3 + 0] > color)
                new_palette[j * 3 + 0] -= (new_palette[j * 3 + 0] - color > 8 ? 8 : new_palette[j * 3 + 0] - color);
            else if (new_palette[j * 3 + 0] < color)
                new_palette[j * 3 + 0] += (color - new_palette[j * 3 + 0] > 8 ? 8 : color - new_palette[j * 3 + 0]);

            if (new_palette[j * 3 + 1] > 0)
                new_palette[j * 3 + 1] -= (new_palette[j * 3 + 1] > 8 ? 8 : new_palette[j * 3 + 1]);

            if (new_palette[j * 3 + 2] > 0)
                new_palette[j * 3 + 2] -= (new_palette[j * 3 + 2] > 8 ? 8 : new_palette[j * 3 + 2]);
        }

        DRIVER_UpdatePalette(new_palette);
        VIDEO_UpdateScreen(NULL);
        UTIL_Delay(75);
    }
}
