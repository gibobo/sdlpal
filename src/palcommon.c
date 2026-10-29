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
#include "profile.h"
#include "util.h"
#include <stdbool.h>
#include <string.h>

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

void PAL_MKFOpen(
    PALMKF *lpMKF,
    void *fp)
/*++
  Purpose:

    Read the chunk offset table of an MKF archive into memory. The handle
    takes ownership of the file.

  Parameters:

    [OUT] lpMKF - the handle to fill in.

    [IN]  fp - pointer to an fopen'ed MKF file.

  Return value:

    None.

--*/
{
    uint32_t uiHeaderSize = 0;

    lpMKF->fp = fp;
    lpMKF->uiChunkCount = 0;
    lpMKF->rguiOffsets = NULL;

    // The first offset is the size of the offset table, which ends with the archive size.
    UTIL_fseek(fp, 0, SEEK_SET);
    if (UTIL_fread(&uiHeaderSize, sizeof(uint32_t), 1, fp) != 1 || uiHeaderSize < 2 * sizeof(uint32_t))
        return;

    lpMKF->rguiOffsets = (uint32_t *)UTIL_malloc(uiHeaderSize);
    UTIL_fseek(fp, 0, SEEK_SET);
    if (UTIL_fread(lpMKF->rguiOffsets, 1, uiHeaderSize, fp) != uiHeaderSize)
        TerminateOnError("%s() failed: cannot read the %u byte MKF offset table\n", __func__, uiHeaderSize);
    lpMKF->uiChunkCount = uiHeaderSize / sizeof(uint32_t) - 1;
}

void PAL_MKFClose(
    PALMKF *lpMKF)
/*++
  Purpose:

    Close an MKF archive and free its offset table.

  Parameters:

    [IN]  lpMKF - the handle to close.

  Return value:

    None.

--*/
{
    if (lpMKF->fp != NULL)
        UTIL_fclose(lpMKF->fp);
    UTIL_free(lpMKF->rguiOffsets);
    lpMKF->fp = NULL;
    lpMKF->uiChunkCount = 0;
    lpMKF->rguiOffsets = NULL;
}

uint32_t PAL_MKFGetChunkCount(
    const PALMKF *lpMKF)
/*++
  Purpose:

    Get the number of chunks in an MKF archive.

  Parameters:

    [IN]  lpMKF - the open MKF archive.

  Return value:

    Integer value which indicates the number of chunks in the specified MKF file.

--*/
{
    return lpMKF != NULL ? lpMKF->uiChunkCount : 0;
}

uint32_t PAL_MKFGetChunkSize(
    uint32_t uiChunkNum,
    const PALMKF *lpMKF)
/*++
  Purpose:

    Get the size of a chunk in an MKF archive.

  Parameters:

    [IN]  uiChunkNum - the number of the chunk in the MKF archive.

    [IN]  lpMKF - the open MKF archive.

  Return value:

    Integer value which indicates the size of the chunk.
    0 if the chunk does not exist.

--*/
{
    if (lpMKF == NULL || uiChunkNum >= lpMKF->uiChunkCount)
        return 0;

    if (lpMKF->rguiOffsets[uiChunkNum] > lpMKF->rguiOffsets[uiChunkNum + 1])
        return 0;

    return lpMKF->rguiOffsets[uiChunkNum + 1] - lpMKF->rguiOffsets[uiChunkNum];
}

uint32_t PAL_MKFReadChunk(
    void *lpBuffer,
    uint32_t uiBufferSize,
    uint32_t uiChunkNum,
    const PALMKF *lpMKF)
/*++
  Purpose:

    Read a chunk from an MKF archive into lpBuffer.

  Parameters:

    [OUT] lpBuffer - pointer to the destination buffer.

    [IN]  uiBufferSize - size of the destination buffer.

    [IN]  uiChunkNum - the number of the chunk in the MKF archive to read.

    [IN]  lpMKF - the open MKF archive.

  Return value:

    The number of bytes read; 0 if the chunk does not exist, is empty or
    does not fit in the buffer.

--*/
{
    uint32_t uiChunkLen;

    if (lpBuffer == NULL || lpMKF == NULL || lpMKF->fp == NULL || uiBufferSize == 0)
        return 0;

    uiChunkLen = PAL_MKFGetChunkSize(uiChunkNum, lpMKF);
    if (uiChunkLen == 0 || uiChunkLen > uiBufferSize)
        return 0;

    PROF_COUNT(PROF_CHUNK_READ, 1);
    UTIL_fseek(lpMKF->fp, lpMKF->rguiOffsets[uiChunkNum], SEEK_SET);
    return UTIL_fread(lpBuffer, 1, uiChunkLen, lpMKF->fp);
}

uint32_t PAL_MKFPrepareBuffer(
    uint8_t **lpBuffer,
    uint32_t uiBufferSize,
    uint32_t uiSize,
    uint32_t uiChunkNum)
/*++
  Purpose:

    Make sure lpBuffer can take a decompressed chunk of uiSize bytes.

  Parameters:

    [IN/OUT] lpBuffer - the destination buffer; allocated when NULL or when
                        uiBufferSize is 0.

    [IN]  uiBufferSize - size of the destination buffer.

    [IN]  uiSize - size of the decompressed chunk.

    [IN]  uiChunkNum - the number of the chunk, for the error message.

  Return value:

    The size of the destination buffer.
    Terminates if a caller-provided buffer is too small.

--*/
{
    if (uiBufferSize == 0 || *lpBuffer == NULL)
    {
        UTIL_free(*lpBuffer);
        uiBufferSize = uiSize;
        *lpBuffer = UTIL_malloc(uiBufferSize);
    }

    if (uiSize > uiBufferSize)
        TerminateOnError("%s() failed: chunk %u needs %u bytes, the buffer holds %u\n", __func__, uiChunkNum, uiSize, uiBufferSize);
    return uiBufferSize;
}

uint32_t PAL_MKFDecompressChunk(
    uint8_t **lpBuffer,
    uint32_t uiBufferSize,
    uint32_t uiChunkNum,
    const PALMKF *lpMKF)
/*++
  Purpose:

    Decompress a compressed chunk from an MKF archive into lpBuffer.

  Parameters:

    [OUT] lpBuffer - pointer to the destination buffer; allocated when NULL or
                     when uiBufferSize is 0.

    [IN]  uiBufferSize - size of the destination buffer.

    [IN]  uiChunkNum - the number of the chunk in the MKF archive to read.

    [IN]  lpMKF - the open MKF archive.

  Return value:

    The decompressed size of the chunk; 0 if the chunk is empty.
    Terminates if a caller-provided buffer is too small.

--*/
{
    // The compressed chunk is read into a buffer that only grows, kept between
    // calls where memory is plentiful.
    static uint8_t *buf = NULL;
    static uint32_t buf_size = 0;
    uint32_t len = PAL_MKFGetChunkSize(uiChunkNum, lpMKF);
    uint32_t size = 0;

    if (len > 0)
    {
        if (len > buf_size)
        {
            UTIL_free(buf);
            buf = (uint8_t *)UTIL_malloc(len);
            buf_size = len;
        }

        PAL_MKFReadChunk(buf, len, uiChunkNum, lpMKF);
        size = *(uint32_t *)buf;

        uiBufferSize = PAL_MKFPrepareBuffer(lpBuffer, uiBufferSize, size, uiChunkNum);
        YJ2_Decompress(buf, *lpBuffer, uiBufferSize);

        // Give internal RAM back: a board without PSRAM cannot spare up to
        // 126 KB (the largest GOP chunk) between loads.
        if (!UTIL_IsPlentiful(buf))
        {
            UTIL_free(buf);
            buf = NULL;
            buf_size = 0;
        }
    }

    return size;
}

int32_t PAL_RNGReadFrame(
    uint8_t **lpBuffer,
    uint32_t uiRngNum,
    uint32_t uiFrameNum,
    const PALMKF *lpRngMKF)
/*++
  Purpose:

    Read a frame from a RNG animation.

  Parameters:

    [OUT] lpBuffer - receives a newly allocated buffer with the compressed
                     frame; the previous buffer is freed.

    [IN]  uiRngNum - the number of the RNG animation in the MKF archive.

    [IN]  uiFrameNum - frame number in the RNG animation.

    [IN]  lpRngMKF - the open RNG archive.

  Return value:

    The size of the frame; 0 if the frame is empty.
    -1 if the parameters are invalid or the archive is truncated.
    -2 if there is no such animation, -3 if the animation is empty,
    -4 if there is no such frame.

--*/
{
    uint32_t uiOffset = 0;
    uint32_t rguiSubOffsets[2] = {0, 0};
    uint32_t uiChunkCount = 0;
    int32_t iChunkLen = 0;
    void *fp;

    if (lpRngMKF == NULL || lpRngMKF->fp == NULL || lpBuffer == NULL)
    {
        return -1;
    }
    fp = lpRngMKF->fp;

    UTIL_free(*lpBuffer);
    *lpBuffer = NULL;

    // Get the offset of the animation from the offset table in memory.
    if (uiRngNum >= lpRngMKF->uiChunkCount)
    {
        return -2;
    }
    uiOffset = lpRngMKF->rguiOffsets[uiRngNum];
    if (lpRngMKF->rguiOffsets[uiRngNum + 1] == uiOffset)
    {
        return -3;
    }

    // Get the number of frames from the animation's own offset table.
    UTIL_fseek(fp, uiOffset, SEEK_SET);
    if (UTIL_fread(&uiChunkCount, sizeof(uint32_t), 1, fp) != 1)
    {
        return -1;
    }
    uiChunkCount = (uiChunkCount >> 2) - 1;
    if (uiFrameNum >= uiChunkCount)
    {
        return -4;
    }

    // Get the offsets of the frame and the next one.
    UTIL_fseek(fp, uiOffset + 4 * uiFrameNum, SEEK_SET);
    if (UTIL_fread(rguiSubOffsets, sizeof(uint32_t), 2, fp) != 2)
    {
        return -1;
    }

    // Get the length of the frame.
    iChunkLen = rguiSubOffsets[1] - rguiSubOffsets[0];

    if (iChunkLen != 0)
    {
        *lpBuffer = (uint8_t *)UTIL_malloc(iChunkLen);
        UTIL_fseek(fp, uiOffset + rguiSubOffsets[0], SEEK_SET);
        return (int)UTIL_fread(*lpBuffer, 1, iChunkLen, fp);
    }

    return 0;
}
