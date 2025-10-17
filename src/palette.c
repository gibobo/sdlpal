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
#include "scene.h"
#include "util.h"
#include <stdbool.h>
#include <string.h>

static unsigned char gPalette[PALETTE_SIZE];

unsigned char *PAL_GetPalette(int iPaletteNum, unsigned char fNight)
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
    static unsigned char fNight_cached = 0xFF;
    if (iPaletteNum == iPaletteNum_cached && fNight == fNight_cached)
        return gPalette;

    iPaletteNum_cached = iPaletteNum;
    fNight_cached = fNight;

    unsigned char buf[PALETTE_SIZE * 2];
    // Read the palette data from the pat.mkf file
    void *fpPAT = UTIL_Open(Res_PAT, "rb");
    int i = PAL_MKFReadChunk(buf, PALETTE_SIZE * 2, iPaletteNum, fpPAT);
    UTIL_Close(Res_PAT);

    if (i < 0)
        return NULL; // Read failed
    else if (i <= PALETTE_SIZE)
        fNight = 0; // There is no night colors in the palette

    unsigned char *ptr = buf + PALETTE_SIZE * ((fNight) ? 1 : 0);

    for (i = 0; i < PALETTE_SIZE; i++)
        gPalette[i] = ptr[i] << 2;

    return gPalette;
}

void PAL_SetPalette(int iPaletteNum, unsigned char fNight)
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
        VIDEO_UpdateScreen(NULL);
    }
}

void PAL_FadeOut(int iDelay)
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
    unsigned int i, j;
    unsigned long time, now;
    unsigned char org_palette[PALETTE_SIZE];
    unsigned char new_palette[PALETTE_SIZE];

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

        VIDEO_SetPalette(new_palette);
        VIDEO_UpdateScreen(NULL);
        UTIL_Delay(iDelay * 10);
    }

    memset(new_palette, 0, sizeof(new_palette));
    VIDEO_SetPalette(new_palette);
    VIDEO_UpdateScreen(NULL);
}

void PAL_FadeIn(int iPaletteNum, unsigned char fNight, unsigned short iDelay)
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
    unsigned short i, j;
    unsigned char *palette = PAL_GetPalette(iPaletteNum, fNight);
    unsigned char newpalette[PALETTE_SIZE] = {0};

    // Start fading in...
    for (j = 0; j < 60U; j += (iDelay > 10U ? iDelay / 10U : 1U))
    {
        for (i = 0; i < PALETTE_SIZE; i++)
        {
            newpalette[i] = (unsigned char)(((unsigned short)palette[i] * j) >> 6U);
        }

        VIDEO_SetPalette(newpalette);
        VIDEO_UpdateScreen(NULL);
        UTIL_Delay(iDelay * 10U);
    }

    VIDEO_SetPalette(palette);
    VIDEO_UpdateScreen(NULL);
}

void PAL_SceneFade(int iPaletteNum, unsigned char fNight, int iStep)
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
    unsigned char newpalette[PALETTE_SIZE];
    int i;
    unsigned int j;

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
            for (j = 0; j < PALETTE_SIZE; j++)
            {
                newpalette[j] = (unsigned char)(((unsigned int)palette[j] * (unsigned int)i) >> 6U);
            }
            VIDEO_SetPalette(newpalette);
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
            VIDEO_UpdateScreen(NULL);

            // Calculate the current palette...
            for (j = 0; j < PALETTE_SIZE; j++)
            {
                newpalette[j] = (unsigned char)(((unsigned int)palette[j] * (unsigned int)i) >> 6U);
            }
            VIDEO_SetPalette(newpalette);
            VIDEO_UpdateScreen(NULL);

            UTIL_Delay(FRAME_TIME);
        }
    }
}

void PAL_PaletteFade(int iPaletteNum, unsigned char fNight, int fUpdateScene)
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
    unsigned char *new_palette = NULL;
    unsigned char org_palette[PALETTE_SIZE];
    unsigned char tmp_palette[PALETTE_SIZE];

    memcpy(org_palette, gPalette, PALETTE_SIZE);
    memset(tmp_palette, 0xff, PALETTE_SIZE);
    new_palette = PAL_GetPalette(iPaletteNum, fNight);

    // Start fading...
    for (i = 0; i < 32; i++)
    {
        for (j = 0; j < PALETTE_SIZE; j++)
        {
            tmp_palette[j] = (unsigned char)(((int)org_palette[j] * (31 - i) + (int)(new_palette[j]) * i) / 31);
        }
        VIDEO_SetPalette(tmp_palette);
        VIDEO_UpdateScreen(NULL);

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

void PAL_ColorFade(int iDelay, unsigned char bColor, int fFrom)
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
    int i, j;
    unsigned char *org_palette = NULL;
    unsigned char new_palette[PALETTE_SIZE];

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

            VIDEO_SetPalette(new_palette);
            VIDEO_UpdateScreen(NULL);
            UTIL_Delay(iDelay);
        }

        VIDEO_SetPalette(org_palette);
        VIDEO_UpdateScreen(NULL);
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

            VIDEO_SetPalette(new_palette);
            VIDEO_UpdateScreen(NULL);
            UTIL_Delay(iDelay);
        }

        for (i = 0; i < 256; i++)
        {
            new_palette[i] = org_palette[bColor];
        }

        VIDEO_SetPalette(new_palette);
        VIDEO_UpdateScreen(NULL);
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
    int i, j;
    unsigned char color;
    unsigned char *org_palette = NULL;
    unsigned char new_palette[PALETTE_SIZE];

    org_palette = PAL_GetPalette(gpGlobals->wNumPalette, gpGlobals->fNightPalette);
    memcpy(new_palette, org_palette, PALETTE_SIZE);

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

        VIDEO_SetPalette(new_palette);
        VIDEO_UpdateScreen(NULL);
        UTIL_Delay(75);
    }
}
