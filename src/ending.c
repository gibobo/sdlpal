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

#include "ending.h"
#include "audio.h"
#include "global.h"
#include "palcommon.h"
#include "palette.h"
#include "play.h"
#include "resource.h"
#include "rngplay.h"
#include "scene.h"
#include "util.h"
#include "video.h"
#include <stdbool.h>
#include <stddef.h>

static void PAL_ShowFBP(
    unsigned short wChunkNum,
    unsigned short wFade,
    unsigned short g_wCurEffectSprite)
/*++
  Purpose:

    Draw an FBP picture to the screen.

  Parameters:

    [IN]  wChunkNum - number of chunk in fbp.mkf file.

    [IN]  wFade - fading speed of showing the picture.

  Return value:

    None.

--*/
{
    unsigned char *buf = NULL;
    unsigned char *bufSprite = NULL;
    const unsigned int rgIndex[6] = {0, 3, 1, 5, 2, 4};
    unsigned char i;
    unsigned int k;
    PAL_Surface *gpScreenBak = VIDEO_GetBackupSurface(0);

    RES_MKFDecompressChunk(&buf, SCREEN_SIZE, wChunkNum, Res_FBP);

    if (g_wCurEffectSprite)
    {
        RES_MKFDecompressChunk(&bufSprite, 0, g_wCurEffectSprite, Res_MGO);
    }

    if (wFade > 0)
    {
        unsigned int fadeDelay = min(max(wFade * 10, 10), 200);
        VIDEO_BackupScreen(gpScreen);

        for (i = 0; i < 96; i++)
        {
            for (k = rgIndex[i % 6]; k < SCREEN_SIZE; k += 6)
            {
                if (i >= 6)
                {
                    unsigned char target = buf[k] & 0x0F;
                    unsigned char current = gpScreenBak->pixels[k] & 0x0F;

                    if (target != current)
                    {
                        current += (target > current) ? 1 : -1;
                    }

                    gpScreenBak->pixels[k] = (buf[k] & 0xF0) | current;
                }
            }

            VIDEO_RestoreScreen(gpScreen);

            if (bufSprite)
            {
                unsigned long f = UTIL_GetMilliseconds() / 150U;
                PAL_RLEBlitToSurface(PAL_SpriteGetFrame(bufSprite, f % PAL_SpriteGetNumFrames(bufSprite)),
                                     gpScreen, PAL_XY(0, 0));
            }

            VIDEO_UpdateScreen(NULL);
            UTIL_Delay(fadeDelay);
        }
    }

    //
    // HACKHACK: to make the ending show correctly
    //
    if (wChunkNum != 68)
    {
        PAL_FBPBlitToSurface(buf, gpScreen);
    }

    VIDEO_UpdateScreen(NULL);
    UTIL_free(buf);
    UTIL_free(bufSprite);
}

static void PAL_ScrollFBP(unsigned short wChunkNum, unsigned short g_wCurEffectSprite)
/*++
  Purpose:

    Scroll up an FBP picture to the screen.

  Parameters:

    [IN]  wChunkNum - number of chunk in fbp.mkf file.

  Return value:

    None.

--*/
{
    unsigned char *bufSprite = NULL;
    int i;
    PAL_Surface *gpScreenBak = VIDEO_GetBackupSurface(0);
    PAL_Surface *p = VIDEO_GetBackupSurface(1);

    RES_MKFDecompressChunk(&p->pixels, SCREEN_SIZE, wChunkNum, Res_FBP);

    if (g_wCurEffectSprite)
    {
        RES_MKFDecompressChunk(&bufSprite, 0, g_wCurEffectSprite, Res_MGO);
    }

    VIDEO_BackupScreen(gpScreen);

    for (int y = 0; y < 220; y++)
    {
        i = min(y, SCREEN_H);

        // Copy bottom part from backup
        VIDEO_CopySurface(gpScreenBak, &(PAL_Rect){0, 0, SCREEN_W, SCREEN_H - i},
                          gpScreen, &(PAL_Rect){0, i, SCREEN_W, SCREEN_H - i});

        // Copy top part from new image
        VIDEO_CopySurface(p, &(PAL_Rect){0, SCREEN_H - i, SCREEN_W, i},
                          gpScreen, &(PAL_Rect){0, 0, SCREEN_W, i});

        PAL_ApplyWave(gpScreen->pixels);

        if (bufSprite)
        {
            unsigned long f = UTIL_GetMilliseconds() / 150U;
            PAL_RLEBlitToSurface(PAL_SpriteGetFrame(bufSprite, f % PAL_SpriteGetNumFrames(bufSprite)),
                                 gpScreen, PAL_XY(0, 0));
        }

        if (gpGlobals->fNeedToFadeIn)
        {
            PAL_FadeIn(gpGlobals->wNumPalette, gpGlobals->fNightPalette, 1);
            gpGlobals->fNeedToFadeIn = false;
        }

        VIDEO_UpdateScreen(NULL);
        UTIL_Delay(53);
    }

    VIDEO_CopyEntireSurface(p, gpScreen);
    VIDEO_UpdateScreen(NULL);
    UTIL_free(bufSprite);
}

static void PAL_EndingAnimation(void)
/*++
  Purpose:

    Show the ending animation.

  Parameters:

    None.

  Return value:

    None.

--*/
{
    PAL_Surface *pUpper = VIDEO_GetBackupSurface(0);
    PAL_Surface *pLower = VIDEO_GetBackupSurface(1);
    unsigned char *bufBeast = NULL;
    unsigned char *bufGirl = NULL;
    int yPosGirl = 180;

    // Load resources
    RES_MKFDecompressChunk(&pUpper->pixels, SCREEN_SIZE, 69, Res_FBP);
    RES_MKFDecompressChunk(&pLower->pixels, SCREEN_SIZE, 70, Res_FBP);
    RES_MKFDecompressChunk(&bufBeast, 0, 571, Res_MGO);
    RES_MKFDecompressChunk(&bufGirl, 0, 572, Res_MGO);

    gpGlobals->wScreenWave = 2;

    for (int i = 0; i < SCREEN_H * 2; i++)
    {
        int halfI = i / 2;

        // Draw background layers
        VIDEO_CopySurface(pLower, &(PAL_Rect){0, 0, SCREEN_W, SCREEN_H - halfI},
                          gpScreen, &(PAL_Rect){0, halfI, SCREEN_W, SCREEN_H - halfI});
        VIDEO_CopySurface(pUpper, &(PAL_Rect){0, SCREEN_H - halfI, SCREEN_W, halfI},
                          gpScreen, &(PAL_Rect){0, 0, SCREEN_W, halfI});

        PAL_ApplyWave(gpScreen->pixels);

        // Draw beast sprites
        PAL_RLEBlitToSurface(PAL_SpriteGetFrame(bufBeast, 0), gpScreen, PAL_XY(0, -SCREEN_H * 2 + i));
        PAL_RLEBlitToSurface(PAL_SpriteGetFrame(bufBeast, 1), gpScreen, PAL_XY(0, -SCREEN_H + i));

        // Draw animated girl
        if (i & 1 && yPosGirl > 80)
            yPosGirl--;
        PAL_RLEBlitToSurface(PAL_SpriteGetFrame(bufGirl, (UTIL_GetMilliseconds() / 50U) % 4U),
                             gpScreen, PAL_XY(220, yPosGirl));

        if (gpGlobals->fNeedToFadeIn)
        {
            PAL_FadeIn(gpGlobals->wNumPalette, gpGlobals->fNightPalette, 1);
            gpGlobals->fNeedToFadeIn = false;
        }

        VIDEO_UpdateScreen(NULL);
        UTIL_Delay(50);
    }

    gpGlobals->wScreenWave = 0;
    UTIL_free(bufBeast);
    UTIL_free(bufGirl);
}

void PAL_EndingScreen(void)
/*++
 Purpose:

   Show the ending screen for Win95 version.

 Parameters:

   None.

 Return value:

   None.

--*/
{
    // Use AVI & WIN95's music if we can
    // Otherwise, simulate the ending of DOS version
#if 1 // unknown music playing
    AUDIO_PlayMusic(-1, false, 0);
    AUDIO_PlayMusic(0x1a, true, 0);
    PAL_RNGPlay(gpGlobals->iCurPlayingRNG, 110, 150, 7);
    PAL_RNGPlay(gpGlobals->iCurPlayingRNG, 151, -1, 9);

    PAL_FadeOut(2);
#endif
#if 1 // beast sence 1
    AUDIO_PlayMusic(-1, false, 0);
    AUDIO_PlayMusic(0x19, true, 0);

    PAL_ShowFBP(75, 0, 0);
    PAL_FadeIn(5, false, 1);
    PAL_ScrollFBP(74, 0);

    PAL_FadeOut(1);
#endif
#if 1 // beast sence 2
    PAL_CleanScreen();
    gpGlobals->wNumPalette = 4;
    gpGlobals->fNeedToFadeIn = true;
    PAL_EndingAnimation();
#endif
#if 1 // disaster
    AUDIO_PlayMusic(-1, false, 0);
    AUDIO_PlayMusic(0x00, false, 2);
    PAL_ColorFade(7, 15, false);

    AUDIO_PlayMusic(-1, false, 0);
    AUDIO_PlayMusic(0x11, true, 0);

    PAL_CleanScreen();
    PAL_SetPalette(0, false);
    PAL_RNGPlay(11, 0, -1, 7);

    PAL_FadeOut(2);
#endif
#if 1 // GIRL 3
    PAL_CleanScreen();
    gpGlobals->wNumPalette = 8;
    gpGlobals->fNeedToFadeIn = true;
    PAL_RNGPlay(10, 0, -1, 6);

    PAL_ShowFBP(77, 10, 0);
    VIDEO_BackupScreen(gpScreen);

    PAL_ShowFBP(76, 7, 0x27b);
#endif
#if 1 // GIRL 2
    PAL_SetPalette(5, false);
    PAL_ShowFBP(73, 7, 0x27b);
    PAL_ScrollFBP(72, 0x27b);
    PAL_ShowFBP(71, 7, 0x27b);
    PAL_ShowFBP(68, 7, 0x27b);
    PAL_ShowFBP(68, 6, 0x000);
    UTIL_WaitKeys(0, 0);
    AUDIO_PlayMusic(0x00, false, 1);
    UTIL_Delay(500);
#endif
#if 1 // staff list
    AUDIO_PlayMusic(-1, false, 0);
    AUDIO_PlayMusic(9, true, 0);
    PAL_SetPalette(5, false);
    PAL_ScrollFBP(67, 0);
    PAL_ScrollFBP(66, 0); // GIRL 3
    PAL_ScrollFBP(65, 0);
    PAL_ScrollFBP(64, 0); // GIRL 2
    PAL_ScrollFBP(63, 0);
    PAL_ScrollFBP(62, 0); // GIRL 1
    PAL_ScrollFBP(61, 0);
    PAL_ScrollFBP(60, 0); // BOY
    PAL_ScrollFBP(59, 0);

    AUDIO_PlayMusic(0x00, false, 6);
    PAL_FadeOut(3); // FadeOut
#endif
}
