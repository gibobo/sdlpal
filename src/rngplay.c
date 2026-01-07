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

#define Check_fread(buf, elem, num, fp)                 \
    if (UTIL_fread((buf), (elem), (num), (fp)) < (num)) \
    return -1

int32_t PAL_RNGReadFrame(uint8_t **lpBuffer, uint32_t uiRngNum, uint32_t uiFrameNum, void *fpRngMKF)
/*++
  Purpose:

    Read a frame from a RNG animation.

  Parameters:

    [OUT] lpBuffer - pointer to the destination buffer.

    [IN]  uiBufferSize - size of the destination buffer.

    [IN]  uiRngNum - the number of the RNG animation in the MKF archive.

    [IN]  uiFrameNum - frame number in the RNG animation.

    [IN]  fpRngMKF - pointer to the fopen'ed MKF file.

  Return value:

    Integer value which indicates the size of the chunk.
    -1 if there are error in parameters.
    -2 if buffer size is not enough.

--*/
{
    uint32_t uiOffset = 0;
    uint32_t uiSubOffset = 0;
    uint32_t uiNextOffset = 0;
    uint32_t uiChunkCount = 0;
    int32_t iChunkLen = 0;

    if (fpRngMKF == NULL || lpBuffer == NULL)
    {
        return -1;
    }

    UTIL_free(*lpBuffer);
    *lpBuffer = NULL;

    // Get the total number of chunks.
    uiChunkCount = PAL_MKFGetChunkCount(fpRngMKF);
    if (uiRngNum >= uiChunkCount)
    {
        return -2;
    }

    // Get the offset of the chunk.
    UTIL_fseek(fpRngMKF, 4 * uiRngNum, SEEK_SET);
    Check_fread(&uiOffset, sizeof(uint32_t), 1, fpRngMKF);
    Check_fread(&uiNextOffset, sizeof(uint32_t), 1, fpRngMKF);

    // Get the length of the chunk.
    iChunkLen = uiNextOffset - uiOffset;
    if (iChunkLen != 0)
    {
        UTIL_fseek(fpRngMKF, uiOffset, SEEK_SET);
    }
    else
    {
        return -3;
    }

    // Get the number of sub chunks.
    Check_fread(&uiChunkCount, sizeof(uint32_t), 1, fpRngMKF);
    uiChunkCount = (uiChunkCount >> 2) - 1;
    if (uiFrameNum >= uiChunkCount)
    {
        return -4;
    }

    // Get the offset of the sub chunk.
    UTIL_fseek(fpRngMKF, uiOffset + 4 * uiFrameNum, SEEK_SET);
    Check_fread(&uiSubOffset, sizeof(uint32_t), 1, fpRngMKF);
    Check_fread(&uiNextOffset, sizeof(uint32_t), 1, fpRngMKF);

    // Get the length of the sub chunk.
    iChunkLen = uiNextOffset - uiSubOffset;

    if (iChunkLen != 0)
    {
        *lpBuffer = (uint8_t *)UTIL_malloc(iChunkLen);
        UTIL_fseek(fpRngMKF, uiOffset + uiSubOffset, SEEK_SET);
        return (int)UTIL_fread(*lpBuffer, 1, iChunkLen, fpRngMKF);
    }

    return 0;
}

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
    uint32_t iDelay = 1000 / (iSpeed > 0 ? iSpeed : 16);

    // Avoid losing the last frame
    if (iEndFrame > 0)
        iEndFrame++;

    while (iStartFrame != iEndFrame)
    {
        int32_t RNGBlit_len = RES_RNGReadFrame(&rng, iNumRNG, iStartFrame++, Res_RNG);
        if (RNGBlit_len <= 0)
            break; // Failed to get the frame, don't go further
        if (PAL_RNGBlitToSurface(rng, RNGBlit_len, gpScreen->pixels) < 0)
            break; // Failed to get the frame, don't go further

        if (gpGlobals)
        {
            // Fade in the screen if needed
            if (gpGlobals->fNeedToFadeIn)
                PAL_FadeIn(gpGlobals->wNumPalette, gpGlobals->fNightPalette, 1);
            gpGlobals->fNeedToFadeIn = 0;
        }
        // Update the screen
        VIDEO_UpdateScreen(NULL);
        // Delay for a while
        UTIL_Delay(iDelay);
    }
    UTIL_free(rng);
}
