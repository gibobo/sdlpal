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
// Portions based on PalLibrary by Lou Yihua <louyihua@21cn.com>.
// Copyright (c) 2006-2007, Lou Yihua.
//

#include "rngplay.h"
#include "driver.h"
#include "global.h"
#include "input.h"
#include "palcommon.h"
#include "palette.h"
#include "resource.h"
#include "util.h"
#include "video.h"
#include <stdlib.h>

static int
PAL_RNGBlitToSurface(
    const uint8_t *rng,
    int32_t length,
    uint8_t *dstSurface)
/*++
  Purpose:

    Blit one frame in an RNG animation to an SDL surface.
    The surface should contain the last frame of the RNG, or blank if it's the first
    frame.

    NOTE: Assume the surface is already locked, and the surface is a 320x200 8-bit one.

  Parameters:

    [IN]  rng - Pointer to the RNG data.

    [IN]  length - Length of the RNG data.

    [OUT] lpDstSurface - pointer to the destination SDL surface.

  Return value:

    0 = success, -1 = error.

--*/
{
    int32_t ptr = 0;
    uint32_t i, n, data;
    uint8_t *dst = NULL;

    // Check for invalid parameters.
    if (dstSurface == NULL || length < 0 || rng == NULL)
    {
        return -1;
    }
    dst = dstSurface;

    // Draw the frame to the surface.
    while (ptr < length)
    {
        data = rng[ptr++];
        n = 0;
        switch (data)
        {
            case 0x00:
            case 0x13:
                // End
                ptr = length;
                break;

            case 0x02:
                dst += 2;
                break;

            case 0x03:
                n = rng[ptr++];
                dst += (n + 1) * 2;
                break;

            case 0x04:
                n = rng[ptr] | ((uint32_t)rng[ptr + 1] << 8);
                ptr += 2;
                dst += (n + 1) * 2;
                break;

            case 0x0a:
                *dst++ = rng[ptr++];
                *dst++ = rng[ptr++];

            case 0x09:
                *dst++ = rng[ptr++];
                *dst++ = rng[ptr++];

            case 0x08:
                *dst++ = rng[ptr++];
                *dst++ = rng[ptr++];

            case 0x07:
                *dst++ = rng[ptr++];
                *dst++ = rng[ptr++];

            case 0x06:
                *dst++ = rng[ptr++];
                *dst++ = rng[ptr++];
                break;

            case 0x0b:
                n = rng[ptr++];
                for (i = 0; i <= n; i++)
                {
                    *dst++ = rng[ptr++];
                    *dst++ = rng[ptr++];
                }
                break;

            case 0x0c:
                n = rng[ptr] | ((uint32_t)rng[ptr + 1] << 8);
                ptr += 2;
                for (i = 0; i <= n; i++)
                {
                    *dst++ = rng[ptr++];
                    *dst++ = rng[ptr++];
                }
                break;

            case 0x0d:
            case 0x0e:
            case 0x0f:
            case 0x10:
                for (i = 0; i < data - 11; i++)
                {
                    *dst++ = rng[ptr];
                    *dst++ = rng[ptr + 1];
                }
                ptr += 2;
                break;

            case 0x11:
                n = rng[ptr++];
                for (i = 0; i <= n; i++)
                {
                    *dst++ = rng[ptr];
                    *dst++ = rng[ptr + 1];
                }
                ptr += 2;
                break;

            case 0x12:
                n = (rng[ptr] | ((uint32_t)rng[ptr + 1] << 8)) + 1;
                ptr += 2;
                for (i = 0; i < n; i++)
                {
                    *dst++ = rng[ptr];
                    *dst++ = rng[ptr + 1];
                }
                ptr += 2;
                break;
        }
    }

    return 0;
}

void PAL_RNGPlay(
    int32_t iNumRNG,
    int32_t iStartFrame,
    int32_t iEndFrame,
    int32_t iSpeed)
/*++
  Purpose:

    Play a RNG movie.

  Parameters:

    [IN]  iNumRNG - number of the RNG movie.

    [IN]  iStartFrame - start frame number.

    [IN]  iEndFrame - end frame number.

    [IN]  iSpeed - speed of playing.

  Return value:

    None.

--*/
{
    uint8_t *rng = NULL;
    uint32_t dwFramesPerSecond = (uint32_t)(iSpeed > 0 ? iSpeed : 16);
    uint32_t dwFrame = 0;
    UTIL_TransitionClock clock;

    // Avoid losing the last frame
    if (iEndFrame > 0)
        iEndFrame++;

    UTIL_TransitionBegin(&clock);
    while (iStartFrame != iEndFrame)
    {
        // Frame n ends n * 1000 / fps ms after the start, so whole-ms steps do not drift.
        uint32_t duration = ((dwFrame + 1) * 1000U) / dwFramesPerSecond - (dwFrame * 1000U) / dwFramesPerSecond;
        int32_t RNGBlit_len = RES_RNGReadFrame((void **)&rng, iNumRNG, iStartFrame++, Res_RNG);
        dwFrame++;
        if (RNGBlit_len <= 0)
            break; // Failed to get the frame, don't go further
        if (PAL_RNGBlitToSurface(rng, RNGBlit_len, gpScreen->pixels) < 0)
            break; // Failed to get the frame, don't go further

        // Update the screen, unless this frame is already late
        if (UTIL_TransitionStep(&clock, duration))
            VIDEO_UpdateScreen(NULL);

        // Fade in the screen if needed, then restart the timeline half a frame later
        if (gpGlobals && gpGlobals->fNeedToFadeIn)
        {
            PAL_FadeIn(gpGlobals->wNumPalette, gpGlobals->fNightPalette, 1);
            gpGlobals->fNeedToFadeIn = 0;
            clock.deadline = UTIL_GetMilliseconds() + duration / 2;
        }

        // Delay until the frame's deadline
        UTIL_TransitionWait(&clock);
    }
    UTIL_TransitionFinish(&clock, "rng");
    UTIL_free(rng);
}
