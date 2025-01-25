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
#include "global.h"
#include "input.h"
#include "palcommon.h"
#include "palette.h"
#include "util.h"
#include "video.h"
#include <stdlib.h>

#define Check_fread(buf, elem, num, fp) if (fread((buf), (elem), (num), (fp)) < (num)) return -1

static int PAL_RNGReadFrame(unsigned char **lpBuffer, unsigned int uiRngNum, unsigned int uiFrameNum, FILE *fpRngMKF)
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
    unsigned int uiOffset = 0;
    unsigned int uiSubOffset = 0;
    unsigned int uiNextOffset = 0;
    unsigned int uiChunkCount = 0;
    int iChunkLen = 0;

    if (fpRngMKF == NULL || lpBuffer == NULL ) {
        return -1;
    }

    free(*lpBuffer);
    *lpBuffer = NULL;

    // Get the total number of chunks.
    uiChunkCount = PAL_MKFGetChunkCount(fpRngMKF);
    if (uiRngNum >= uiChunkCount) {
        return -1;
    }

    // Get the offset of the chunk.
    PAL_fseek(fpRngMKF, 4 * uiRngNum, SEEK_SET);
    Check_fread(&uiOffset, sizeof(unsigned int), 1, fpRngMKF);
    Check_fread(&uiNextOffset, sizeof(unsigned int), 1, fpRngMKF);

    // Get the length of the chunk.
    iChunkLen = uiNextOffset - uiOffset;
    if (iChunkLen != 0) {
        PAL_fseek(fpRngMKF, uiOffset, SEEK_SET);
    } else {
        return -1;
    }

    // Get the number of sub chunks.
    Check_fread(&uiChunkCount, sizeof(unsigned int), 1, fpRngMKF);
    uiChunkCount = (uiChunkCount >> 2) - 1;
    if (uiFrameNum >= uiChunkCount) {
        return -1;
    }

    // Get the offset of the sub chunk.
    PAL_fseek(fpRngMKF, uiOffset + 4 * uiFrameNum, SEEK_SET);
    Check_fread(&uiSubOffset, sizeof(unsigned int), 1, fpRngMKF);
    Check_fread(&uiNextOffset, sizeof(unsigned int), 1, fpRngMKF);

    // Get the length of the sub chunk.
    iChunkLen = uiNextOffset - uiSubOffset;

    if (iChunkLen != 0) {
        *lpBuffer = (unsigned char *)UTIL_malloc(iChunkLen);
        PAL_fseek(fpRngMKF, uiOffset + uiSubOffset, SEEK_SET);
        return (int)fread(*lpBuffer, 1, iChunkLen, fpRngMKF);
    }

    return -1;
}

static int
PAL_RNGBlitToSurface(
    const unsigned char *rng,
    int length,
    PAL_Surface *lpDstSurface)
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
   int ptr = 0;
   unsigned int i, n, data;
   unsigned char *dst = NULL;

   // Check for invalid parameters.
   if (lpDstSurface == NULL || length < 0)
   {
      return -1;
   }
   dst = lpDstSurface->pixels;

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
         n = rng[ptr] | (rng[ptr + 1] << 8);
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
         n = rng[ptr] | (rng[ptr + 1] << 8);
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
         n = (rng[ptr] | (rng[ptr + 1] << 8)) + 1;
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

void
PAL_RNGPlay(
   int           iNumRNG,
   int           iStartFrame,
   int           iEndFrame,
   int           iSpeed
)
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
   FILE *fp = NULL;
   unsigned char *rng = NULL;
   unsigned char *buf = NULL;
   int rng_size = 0;
   int buf_size = 0;
   unsigned int iDelay = 1000 / (iSpeed > 0 ? iSpeed : 16);
   unsigned int iTime = UTIL_GetTicks();

   // Avoid losing the last frame
   if (iEndFrame > 0) iEndFrame++;

   // buf = (unsigned char *)malloc(65000);
   fp = PAL_fopen(RESOURCE_PATH "/rng.mkf", "rb");

   for (; fp && iStartFrame != iEndFrame; iStartFrame++) {
     iTime += iDelay;
     // Read, decompress and render the frame
     buf_size = PAL_RNGReadFrame(&buf, iNumRNG, iStartFrame, fp);
     if (buf_size < 0)
       break; // Failed to get the frame, don't go further

     free(rng);
     rng_size = *(unsigned int *)buf;
     rng = (unsigned char *)UTIL_malloc(rng_size);
     if (PAL_RNGBlitToSurface(rng, Decompress(buf, rng, rng_size), gpScreen) < 0)
       break; // Failed to get the frame, don't go further

     // Update the screen
     VIDEO_UpdateScreen(NULL);

     // Fade in the screen if needed
     if (gpGlobals->fNeedToFadeIn) {
       PAL_FadeIn(gpGlobals->wNumPalette, gpGlobals->fNightPalette, 1);
       gpGlobals->fNeedToFadeIn = 0;
     }

     // Delay for a while
     PAL_DelayUntil(iTime);
   }

   PAL_fclose(fp);
   free(rng);
   free(buf);
}
