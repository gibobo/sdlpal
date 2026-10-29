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

#include "palcommon.h"
#include "driver.h"
#include "util.h"
#include <stdbool.h>
#include <string.h>

#define Check_fread(buf, elem, num, fp)                 \
    if (UTIL_fread((buf), (elem), (num), (fp)) < (num)) \
    return 0

int PAL_RLEBlitToSurface(
    const uint8_t *lpBitmapRLE,
    VIDEO_Surface *lpDstSurface,
    uint32_t pos)
{
    return PAL_RLEBlitToSurfaceWithShadow(lpBitmapRLE, lpDstSurface, pos, false);
}

int PAL_RLEBlitToSurfaceWithShadow(
    const uint8_t *lpBitmapRLE,
    VIDEO_Surface *lpDstSurface,
    uint32_t pos,
    int bShadow)
/*++
  Purpose:

    Blit an RLE-compressed bitmap to an SDL surface.
    NOTE: Assume the surface is already locked, and the surface is a 8-bit one.

  Parameters:

    [IN]  lpBitmapRLE - pointer to the RLE-compressed bitmap to be decoded.

    [OUT] lpDstSurface - pointer to the destination SDL surface.

    [IN]  pos - position of the destination area.

    [IN]  bShadow - flag to mention whether blit source color or just shadow.

  Return value:

    0 = success, -1 = error.

--*/
{
    int32_t i, j, k, sx;
    int32_t x, y;
    int32_t uiLen = 0;
    int32_t uiWidth = 0;
    int32_t uiHeight = 0;
    int32_t uiSrcX = 0;
    uint8_t T;
    int32_t dx = PAL_X(pos);
    int32_t dy = PAL_Y(pos);

    uint8_t *dstPixels;
    int32_t dst_w, dst_h;

    // Check for NULL pointer.
    if (lpBitmapRLE == NULL || lpDstSurface == NULL || lpDstSurface->pixels == NULL)
    {
        return -1;
    }

    dstPixels = lpDstSurface->pixels;
    dst_w = lpDstSurface->w;
    dst_h = lpDstSurface->h;

    // Skip the 0x00000002 in the file header.
    if (lpBitmapRLE[0] == 0x02 && lpBitmapRLE[1] == 0x00 &&
        lpBitmapRLE[2] == 0x00 && lpBitmapRLE[3] == 0x00)
    {
        lpBitmapRLE += 4;
    }

    // Get the width and height of the bitmap.
    uiWidth = lpBitmapRLE[0] | (int)((uint32_t)lpBitmapRLE[1] << 8);
    uiHeight = lpBitmapRLE[2] | (int)((uint32_t)lpBitmapRLE[3] << 8);

    // Check whether bitmap intersects the surface.
    if (uiWidth + dx <= 0 || dx >= dst_w ||
        uiHeight + dy <= 0 || dy >= dst_h)
    {
        return 0;
    }

    // Calculate the total length of the bitmap.
    // The bitmap is 8-bpp, each pixel will use 1 byte.
    uiLen = uiWidth * uiHeight;

    // Prepare a static shadow lookup table to avoid repeated bit ops in hot loop.
    static uint8_t shadow_tbl[256];
    static int shadow_tbl_inited = 0;
    if (bShadow && !shadow_tbl_inited)
    {
        for (int t = 0; t < 256; ++t)
            shadow_tbl[t] = (uint8_t)((t & 0xF0) | ((t & 0x0F) >> 1));
        shadow_tbl_inited = 1;
    }

    // Start decoding and blitting the bitmap.
    lpBitmapRLE += 4;
    for (i = 0; i < uiLen;)
    {
        T = *lpBitmapRLE++;
        if ((T & 0x80) && T <= 0x80 + uiWidth)
        {
            int32_t skip = T - 0x80;
            i += skip;
            uiSrcX += skip;
            if (uiSrcX >= uiWidth)
            {
                uiSrcX -= uiWidth;
                dy++;
            }
        }
        else
        {
            // Prepare coordinates.
            j = 0;
            sx = uiSrcX;
            x = dx + uiSrcX;
            y = dy;

            // Skip the points which are out of the surface.
            if (y < 0)
            {
                j += -y * uiWidth;
                y = 0;
            }
            else if (y >= dst_h)
            {
                return 0; // No more pixels needed, break out
            }

            while (j < T)
            {
                // Skip the points which are out of the surface.
                if (x < 0)
                {
                    int32_t skipx = -x;
                    j += skipx;
                    if (j >= T)
                        break;
                    sx += skipx;
                    x = 0;
                }
                else if (x >= dst_w)
                {
                    j += uiWidth - sx;
                    x -= sx;
                    sx = 0;
                    y++;
                    if (y >= dst_h)
                    {
                        return 0; // No more pixels needed, break out
                    }
                    continue;
                }

                // Put the pixels in row onto the surface
                k = T - j;
                if (dst_w - x < k)
                    k = dst_w - x;
                if (uiWidth - sx < k)
                    k = uiWidth - sx;

                // Row pointer
                uint8_t *row = dstPixels + (size_t)y * dst_w;
                if (bShadow)
                {
                    // Apply shadow to existing destination pixels.
                    uint8_t *dst = row + x;
                    int32_t kk = k;
                    // use lookup table per-byte for speed
                    while (kk--)
                    {
                        *dst = shadow_tbl[*dst];
                        dst++;
                    }
                    j += k;
                    x += k;
                }
                else
                {
                    // Fast bulk copy from RLE buffer to surface row.
                    uint8_t *dst = row + x;
                    const uint8_t *src = lpBitmapRLE + j;
                    // memcpy is faster for large k; safe since src and dst do not overlap.
                    memcpy(dst, src, (size_t)k);
                    j += k;
                    x += k;
                }

                sx += k;
                if (sx >= uiWidth)
                {
                    sx -= uiWidth;
                    x -= uiWidth;
                    y++;
                    if (y >= dst_h)
                    {
                        return 0; // No more pixels needed, break out
                    }
                }
            }
            lpBitmapRLE += T;
            i += T;
            uiSrcX += T;
            while (uiSrcX >= uiWidth)
            {
                uiSrcX -= uiWidth;
                dy++;
            }
        }
    }

    // Success
    return 0;
}

int PAL_RLEBlitWithColorShift(
    const uint8_t *lpBitmapRLE,
    VIDEO_Surface *lpDstSurface,
    uint32_t pos,
    int iColorShift)
/*++
  Purpose:

    Blit an RLE-compressed bitmap to an SDL surface.
    NOTE: Assume the surface is already locked, and the surface is a 8-bit one.

  Parameters:

    [IN]  lpBitmapRLE - pointer to the RLE-compressed bitmap to be decoded.

    [OUT] lpDstSurface - pointer to the destination SDL surface.

    [IN]  pos - position of the destination area.

    [IN]  iColorShift - shift the color by this value.

  Return value:

    0 = success, -1 = error.

--*/
{
    int32_t i, j, k, sx;
    int32_t x, y;
    int32_t uiLen = 0;
    int32_t uiWidth = 0;
    int32_t uiHeight = 0;
    int32_t uiSrcX = 0;
    uint8_t T, b;
    int32_t dx = PAL_X(pos);
    int32_t dy = PAL_Y(pos);
    uint8_t *p;

    //
    // Check for NULL pointer.
    //
    if (lpBitmapRLE == NULL || lpDstSurface == NULL)
    {
        return -1;
    }

    //
    // Skip the 0x00000002 in the file header.
    //
    if (lpBitmapRLE[0] == 0x02 && lpBitmapRLE[1] == 0x00 &&
        lpBitmapRLE[2] == 0x00 && lpBitmapRLE[3] == 0x00)
    {
        lpBitmapRLE += 4;
    }

    //
    // Get the width and height of the bitmap.
    //
    uiWidth = lpBitmapRLE[0] | (int)((uint32_t)lpBitmapRLE[1] << 8);
    uiHeight = lpBitmapRLE[2] | (int)((uint32_t)lpBitmapRLE[3] << 8);

    //
    // Check whether bitmap intersects the surface.
    //
    if (uiWidth + dx <= 0 || dx >= lpDstSurface->w ||
        uiHeight + dy <= 0 || dy >= lpDstSurface->h)
    {
        return 0;
    }

    //
    // Calculate the total length of the bitmap.
    // The bitmap is 8-bpp, each pixel will use 1 byte.
    //
    uiLen = uiWidth * uiHeight;

    //
    // Start decoding and blitting the bitmap.
    //
    lpBitmapRLE += 4;
    for (i = 0; i < uiLen;)
    {
        T = *lpBitmapRLE++;
        if ((T & 0x80) && T <= 0x80 + uiWidth)
        {
            i += T - 0x80;
            uiSrcX += T - 0x80;
            if (uiSrcX >= uiWidth)
            {
                uiSrcX -= uiWidth;
                dy++;
            }
        }
        else
        {
            //
            // Prepare coordinates.
            //
            j = 0;
            sx = uiSrcX;
            x = dx + uiSrcX;
            y = dy;

            //
            // Skip the points which are out of the surface.
            //
            if (y < 0)
            {
                j += -y * uiWidth;
                y = 0;
            }
            else if (y >= lpDstSurface->h)
            {
                return 0; // No more pixels needed, break out
            }

            while (j < T)
            {
                //
                // Skip the points which are out of the surface.
                //
                if (x < 0)
                {
                    j += -x;
                    if (j >= T)
                        break;
                    sx += -x;
                    x = 0;
                }
                else if (x >= lpDstSurface->w)
                {
                    j += uiWidth - sx;
                    x -= sx;
                    sx = 0;
                    y++;
                    if (y >= lpDstSurface->h)
                    {
                        return 0; // No more pixels needed, break out
                    }
                    continue;
                }

                // Put the pixels in row onto the surface
                k = T - j;
                if (lpDstSurface->w - x < k)
                    k = lpDstSurface->w - x;
                if (uiWidth - sx < k)
                    k = uiWidth - sx;
                sx += k;
                p = lpDstSurface->pixels + y * lpDstSurface->w;
                for (; k != 0; k--)
                {
                    b = (lpBitmapRLE[j] & 0x0F);
                    if ((int)b + iColorShift > 0x0F)
                    {
                        b = 0x0F;
                    }
                    else if ((int)b + iColorShift < 0)
                    {
                        b = 0;
                    }
                    else
                    {
                        b += iColorShift;
                    }

                    p[x] = (b | (lpBitmapRLE[j] & 0xF0));
                    j++;
                    x++;
                }

                if (sx >= uiWidth)
                {
                    sx -= uiWidth;
                    x -= uiWidth;
                    y++;
                    if (y >= lpDstSurface->h)
                    {
                        return 0; // No more pixels needed, break out
                    }
                }
            }
            lpBitmapRLE += T;
            i += T;
            uiSrcX += T;
            while (uiSrcX >= uiWidth)
            {
                uiSrcX -= uiWidth;
                dy++;
            }
        }
    }

    // Success
    return 0;
}

int PAL_RLEBlitMonoColor(
    const uint8_t *lpBitmapRLE,
    VIDEO_Surface *lpDstSurface,
    uint32_t pos,
    uint8_t bColor,
    int iColorShift)
/*++
  Purpose:

    Blit an RLE-compressed bitmap to an SDL surface in mono-color form.
    NOTE: Assume the surface is already locked, and the surface is a 8-bit one.

  Parameters:

    [IN]  lpBitmapRLE - pointer to the RLE-compressed bitmap to be decoded.

    [OUT] lpDstSurface - pointer to the destination SDL surface.

    [IN]  pos - position of the destination area.

    [IN]  bColor - the color to be used while drawing.

    [IN]  iColorShift - shift the color by this value.

  Return value:

    0 = success, -1 = error.

--*/
{
    int32_t i, j, k, sx;
    int32_t x, y;
    int32_t uiLen = 0;
    int32_t uiWidth = 0;
    int32_t uiHeight = 0;
    int32_t uiSrcX = 0;
    uint8_t T, b;
    int32_t dx = PAL_X(pos);
    int32_t dy = PAL_Y(pos);
    uint8_t *p;

    // Check for NULL pointer.
    if (lpBitmapRLE == NULL || lpDstSurface == NULL)
    {
        return -1;
    }

    // Skip the 0x00000002 in the file header.
    if (lpBitmapRLE[0] == 0x02 && lpBitmapRLE[1] == 0x00 &&
        lpBitmapRLE[2] == 0x00 && lpBitmapRLE[3] == 0x00)
    {
        lpBitmapRLE += 4;
    }

    // Get the width and height of the bitmap.
    uiWidth = lpBitmapRLE[0] | (int)((uint32_t)lpBitmapRLE[1] << 8);
    uiHeight = lpBitmapRLE[2] | (int)((uint32_t)lpBitmapRLE[3] << 8);

    // Check whether bitmap intersects the surface.
    if (uiWidth + dx <= 0 || dx >= lpDstSurface->w ||
        uiHeight + dy <= 0 || dy >= lpDstSurface->h)
    {
        return 0;
    }

    // Calculate the total length of the bitmap.
    // The bitmap is 8-bpp, each pixel will use 1 byte.
    uiLen = uiWidth * uiHeight;

    // Start decoding and blitting the bitmap.
    lpBitmapRLE += 4;
    bColor &= 0xF0;
    for (i = 0; i < uiLen;)
    {
        T = *lpBitmapRLE++;
        if ((T & 0x80) && T <= 0x80 + uiWidth)
        {
            i += T - 0x80;
            uiSrcX += T - 0x80;
            if (uiSrcX >= uiWidth)
            {
                uiSrcX -= uiWidth;
                dy++;
            }
        }
        else
        {
            // Prepare coordinates.
            j = 0;
            sx = uiSrcX;
            x = dx + uiSrcX;
            y = dy;

            // Skip the points which are out of the surface.
            if (y < 0)
            {
                j += -y * uiWidth;
                y = 0;
            }
            else if (y >= lpDstSurface->h)
            {
                return 0; // No more pixels needed, break out
            }

            while (j < T)
            {
                // Skip the points which are out of the surface.
                if (x < 0)
                {
                    j += -x;
                    if (j >= T)
                        break;
                    sx += -x;
                    x = 0;
                }
                else if (x >= lpDstSurface->w)
                {
                    j += uiWidth - sx;
                    x -= sx;
                    sx = 0;
                    y++;
                    if (y >= lpDstSurface->h)
                    {
                        return 0; // No more pixels needed, break out
                    }
                    continue;
                }

                //
                // Put the pixels in row onto the surface
                //
                k = T - j;
                if (lpDstSurface->w - x < k)
                    k = lpDstSurface->w - x;
                if (uiWidth - sx < k)
                    k = uiWidth - sx;
                sx += k;
                p = lpDstSurface->pixels + y * lpDstSurface->w;
                for (; k != 0; k--)
                {
                    b = lpBitmapRLE[j] & 0x0F;
                    if ((int)b + iColorShift > 0x0F)
                    {
                        b = 0x0F;
                    }
                    else if ((int)b + iColorShift < 0)
                    {
                        b = 0;
                    }
                    else
                    {
                        b += iColorShift;
                    }

                    p[x] = (b | bColor);
                    j++;
                    x++;
                }

                if (sx >= uiWidth)
                {
                    sx -= uiWidth;
                    x -= uiWidth;
                    y++;
                    if (y >= lpDstSurface->h)
                    {
                        return 0; // No more pixels needed, break out
                    }
                }
            }
            lpBitmapRLE += T;
            i += T;
            uiSrcX += T;
            while (uiSrcX >= uiWidth)
            {
                uiSrcX -= uiWidth;
                dy++;
            }
        }
    }

    // Success
    return 0;
}

int PAL_FBPBlitToSurface(
    uint8_t *lpBitmapFBP,
    VIDEO_Surface *lpDstSurface)
/*++
  Purpose:

    Blit an uncompressed bitmap in FBP.MKF to an SDL surface.
    NOTE: Assume the surface is already locked, and the surface is a 8-bit 320x200 one.

  Parameters:

    [IN]  lpBitmapFBP - pointer to the RLE-compressed bitmap to be decoded.

    [OUT] lpDstSurface - pointer to the destination SDL surface.

  Return value:

    0 = success, -1 = error.

--*/
{
    if (lpBitmapFBP == NULL || lpDstSurface == NULL ||
        lpDstSurface->w != SCREEN_W || lpDstSurface->h != SCREEN_H)
    {
        return -1;
    }

    // simply copy everything to the surface
    memcpy(lpDstSurface->pixels, lpBitmapFBP, lpDstSurface->w * lpDstSurface->h);
    return 0;
}

uint16_t PAL_RLEGetWidth(
    const uint8_t *lpBitmapRLE)
/*++
  Purpose:

    Get the width of an RLE-compressed bitmap.

  Parameters:

    [IN]  lpBitmapRLE - pointer to an RLE-compressed bitmap.

  Return value:

    Integer value which indicates the height of the bitmap.

--*/
{
    if (lpBitmapRLE == NULL)
    {
        return 0;
    }

    // Skip the 0x00000002 in the file header.
    if (lpBitmapRLE[0] == 0x02 && lpBitmapRLE[1] == 0x00 &&
        lpBitmapRLE[2] == 0x00 && lpBitmapRLE[3] == 0x00)
    {
        lpBitmapRLE += 4;
    }

    // Return the width of the bitmap.
    return lpBitmapRLE[0] | ((uint16_t)lpBitmapRLE[1] << 8);
}

uint16_t PAL_RLEGetHeight(
    const uint8_t *lpBitmapRLE)
/*++
  Purpose:

    Get the height of an RLE-compressed bitmap.

  Parameters:

    [IN]  lpBitmapRLE - pointer of an RLE-compressed bitmap.

  Return value:

    Integer value which indicates the height of the bitmap.

--*/
{
    if (lpBitmapRLE == NULL)
    {
        return 0;
    }

    // Skip the 0x00000002 in the file header.
    if (lpBitmapRLE[0] == 0x02 && lpBitmapRLE[1] == 0x00 &&
        lpBitmapRLE[2] == 0x00 && lpBitmapRLE[3] == 0x00)
    {
        lpBitmapRLE += 4;
    }

    // Return the height of the bitmap.
    return lpBitmapRLE[2] | ((uint16_t)lpBitmapRLE[3] << 8);
}

uint16_t
PAL_SpriteGetNumFrames(
    const uint8_t *lpSprite)
/*++
  Purpose:

    Get the total number of frames of a sprite.

  Parameters:

    [IN]  lpSprite - pointer to the sprite.

  Return value:

    Number of frames of the sprite.

--*/
{
    if (lpSprite == NULL)
    {
        return 0;
    }

    return (lpSprite[0] | (int)((uint32_t)lpSprite[1] << 8)) - 1;
}

uint8_t *
PAL_SpriteGetFrame(
    const uint8_t *lpSprite,
    int iFrameNum)
/*++
  Purpose:

    Get the pointer to the specified frame from a sprite.

  Parameters:

    [IN]  lpSprite - pointer to the sprite.

    [IN]  iFrameNum - number of the frame.

  Return value:

    Pointer to the specified frame. NULL if the frame does not exist.

--*/
{
    uint32_t imagecount, offset;

    if (lpSprite == NULL)
    {
        return NULL;
    }

    imagecount = lpSprite[0] | (uint32_t)lpSprite[1] << 8;

    if (iFrameNum < 0 || (uint32_t)iFrameNum >= imagecount)
    {
        // The frame does not exist
        return NULL;
    }

    // Get the offset of the frame
    iFrameNum = (uint32_t)iFrameNum << 1;
    offset = (lpSprite[iFrameNum] | (uint32_t)lpSprite[iFrameNum + 1] << 8) << 1;
    if (offset == 0x18444)
        offset = (uint16_t)offset;
    return (uint8_t *)&lpSprite[offset];
}

uint32_t PAL_MKFGetChunkCount(void *fp)
/*++
  Purpose:

    Get the number of chunks in an MKF archive.

  Parameters:

    [IN]  fp - pointer to an fopen'ed MKF file.

  Return value:

    Integer value which indicates the number of chunks in the specified MKF file.

--*/
{
    uint32_t iNumChunk = 0;
    if (fp)
    {
        UTIL_fseek(fp, 0, SEEK_SET);
        if (UTIL_fread(&iNumChunk, sizeof(uint32_t), 1, fp) == 1)
            return (iNumChunk >> 2) - 1;
    }
    return 0;
}

uint32_t PAL_MKFGetChunkSize(uint32_t uiChunkNum, void *fp)
/*++
  Purpose:

    Get the size of a chunk in an MKF archive.

  Parameters:

    [IN]  uiChunkNum - the number of the chunk in the MKF archive.

    [IN]  fp - pointer to the fopen'ed MKF file.

  Return value:

    Integer value which indicates the size of the chunk.
    -1 if the chunk does not exist.

--*/
{
    uint32_t uiOffset = 0;
    uint32_t uiNextOffset = 0;
    uint32_t uiChunkCount = 0;

    if (fp == NULL)
        return 0;
    //
    // Get the total number of chunks.
    //
    uiChunkCount = PAL_MKFGetChunkCount(fp);
    if (uiChunkNum >= uiChunkCount)
        return 0;

    //
    // Get the offset of the specified chunk and the next chunk.
    //
    UTIL_fseek(fp, uiChunkNum * sizeof(uint32_t), SEEK_SET);
    Check_fread(&uiOffset, sizeof(uint32_t), 1, fp);
    Check_fread(&uiNextOffset, sizeof(uint32_t), 1, fp);

    if (uiOffset > uiNextOffset)
        return 0;
    //
    // Return the length of the chunk.
    //
    return uiNextOffset - uiOffset;
}

//TO DO
uint32_t PAL_MKFReadChunk(
    void *lpBuffer,
    uint32_t uiBufferSize,
    uint32_t uiChunkNum,
    void *fp)
/*++
  Purpose:

    Read a chunk from an MKF archive into lpBuffer.

  Parameters:

    [OUT] lpBuffer - pointer to the destination buffer.

    [IN]  uiBufferSize - size of the destination buffer.

    [IN]  uiChunkNum - the number of the chunk in the MKF archive to read.

    [IN]  fp - pointer to the fopen'ed MKF file.

  Return value:

    Integer value which indicates the size of the chunk.
    -1 if there are error in parameters.
    -2 if buffer size is not enough.

--*/
{
    uint32_t uiOffset = 0;
    uint32_t uiNextOffset = 0;
    uint32_t uiChunkCount;
    uint32_t uiChunkLen;

    if (lpBuffer == NULL || fp == NULL || uiBufferSize == 0)
        return 0;

    //
    // Get the total number of chunks.
    //
    uiChunkCount = PAL_MKFGetChunkCount(fp);
    if (uiChunkNum >= uiChunkCount)
        return 0;

    // Get the offset of the chunk.
    UTIL_fseek(fp, sizeof(uint32_t) * uiChunkNum, SEEK_SET);
    Check_fread(&uiOffset, sizeof(uint32_t), 1, fp);
    Check_fread(&uiNextOffset, sizeof(uint32_t), 1, fp);

    if (uiOffset > uiNextOffset)
        return 0;

    // Get the length of the chunk.
    uiChunkLen = uiNextOffset - uiOffset;

    if (uiChunkLen > uiBufferSize)
        return 0;

    UTIL_fseek(fp, uiOffset, SEEK_SET);
    return (int)UTIL_fread(lpBuffer, 1, uiChunkLen, fp);
}

uint32_t PAL_MKFDecompressChunk(
    uint8_t **lpBuffer,
    uint32_t uiBufferSize,
    uint32_t uiChunkNum,
    void *fp)
/*++
  Purpose:

    Decompress a compressed chunk from an MKF archive into lpBuffer.

  Parameters:

    [OUT] lpBuffer - pointer to the destination buffer.

    [IN]  uiBufferSize - size of the destination buffer.

    [IN]  uiChunkNum - the number of the chunk in the MKF archive to read.

    [IN]  fp - pointer to the fopen'ed MKF file.

  Return value:

    Integer value which indicates the size of the chunk.
    -1 if there are error in parameters, or buffer size is not enough.
    -3 if cannot allocate memory for decompression.

--*/
{
    uint32_t len = PAL_MKFGetChunkSize(uiChunkNum, fp);

    if (len > 0)
    {
        uint8_t *buf = (uint8_t *)UTIL_malloc(len);

        PAL_MKFReadChunk(buf, len, uiChunkNum, fp);

        if (uiBufferSize == 0 || *lpBuffer == NULL)
        {
            UTIL_free(*lpBuffer);
            uiBufferSize = *(uint32_t *)buf;
            *lpBuffer = UTIL_malloc(uiBufferSize);
        }

        YJ2_Decompress(buf, *lpBuffer);
        UTIL_free(buf);
    }

    return len;
}
