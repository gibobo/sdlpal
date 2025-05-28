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

#include "map.h"
#include "palcommon.h"
#include "util.h"
#include <stdlib.h>

PALMAP *PAL_LoadMap(int iMapNum, void *fpMapMKF, void *fpGopMKF)
/*++
  Purpose:

    Load the specified map from the MKF file, as well as the tile bitmaps.

  Parameters:

    [IN]  iMapNum - Number of the map to load.

    [IN]  fpMapMKF - Pointer to the fopen'ed map.mkf file, which
                     contains the map tile data.

    [IN]  fpGopMKF - Pointer to the fopen'ed gop.mkf file, which
                     contains the tile bitmaps. The bitmap can be read
                     by PAL_SpriteGetFrame() function.

  Return value:

    Pointer to the loaded map. NULL if failed.

--*/
{
   int size;
   PALMAP *map;

   // Check for invalid map number.
   if (iMapNum <= 0)
     return NULL;

   // Create the map instance.
   map = (PALMAP *)UTIL_malloc(sizeof(PALMAP));

   PAL_MKFDecompressChunk((unsigned char **)&map->Tiles, 0, iMapNum, fpMapMKF);

   // Load the tile bitmaps.
   size = PAL_MKFGetChunkSize(iMapNum, fpGopMKF);
   if (size <= 0) {
      UTIL_free(map->Tiles);
      UTIL_free(map);
      return NULL;
   }

   map->pTileSprite = (unsigned char *)UTIL_malloc(size);
   if (PAL_MKFReadChunk(map->pTileSprite, size, iMapNum, fpGopMKF) < 0) {
      UTIL_free(map->pTileSprite);
      UTIL_free(map->Tiles);
      UTIL_free(map);
      return NULL;
   }

   // Done.
   map->iMapNum = iMapNum;

   return map;
}

void PAL_FreeMap(PALMAP *lpMap)
/*++
  Purpose:

    Free a loaded map, as well as the tile bitmaps.

  Parameters:

    [IN]  lpMap - Pointer to the loaded map structure.

  Return value:

    None.

--*/
{
   // Check for NULL pointer.
   if (lpMap == NULL)
   {
      return;
   }

   // Free the tile bitmaps.
   UTIL_free(lpMap->pTileSprite);
   
   // Free the tiles.
   UTIL_free(lpMap->Tiles);
   // Delete the instance.
   UTIL_free(lpMap);
}

const unsigned char *PAL_MapGetTileBitmap(
    unsigned char x,
    unsigned char y,
    unsigned char h,
    unsigned char ucLayer,
    PALMAP *lpMap)
/*++
  Purpose:

    Get the tile bitmap on the specified layer at the location (x, y, h).

  Parameters:

    [IN]  x - Column number of the tile.

    [IN]  y - Line number in the map.

    [IN]  h - Each line in the map has two lines of tiles, 0 and 1.
              (See map.h for details.)

    [IN]  ucLayer - The layer. 0 for bottom, 1 for top.

    [IN]  lpMap - Pointer to the loaded map.

  Return value:

    Pointer to the bitmap. NULL if failed.

--*/
{
   unsigned int d;

   // Check for invalid parameters.
   if (x >= PALMAP_X || y >= PALMAP_Y || h >= PALMAP_Z || lpMap == NULL) {
      return NULL;
   }

   // Get the tile data of the specified location.
   d = lpMap->Tiles[y * PALMAP_X * PALMAP_Z + x * PALMAP_Z + h];

   if (ucLayer == 0) {
      // Bottom layer
      return PAL_SpriteGetFrame(lpMap->pTileSprite, (int)(d & 0xFF) | ((d >> 4) & 0x100));
   } else {
      // Top layer
      d >>= 16;
      return PAL_SpriteGetFrame(lpMap->pTileSprite, (int)((d & 0xFF) | ((d >> 4) & 0x100)) - 1);
   }
}

int PAL_MapTileIsBlocked(
    unsigned char x,
    unsigned char y,
    unsigned char h,
    PALMAP *lpMap)
/*++
  Purpose:

    Check if the tile at the specified location is blocked.

  Parameters:

    [IN]  x - Column number of the tile.

    [IN]  y - Line number in the map.

    [IN]  h - Each line in the map has two lines of tiles, 0 and 1.
              (See map.h for details.)

    [IN]  lpMap - Pointer to the loaded map.

  Return value:

    TRUE if the tile is blocked, FALSE if not.

--*/
{
   // Check for invalid parameters.
   if (x >= PALMAP_X || y >= PALMAP_Y || h >= PALMAP_Z || lpMap == NULL)
   {
      return 1;
   }

   return (lpMap->Tiles[y * PALMAP_X * PALMAP_Z + x * PALMAP_Z + h] & 0x2000) >> 13;
}

unsigned char
PAL_MapGetTileHeight(
    unsigned char x,
    unsigned char y,
    unsigned char h,
    unsigned char ucLayer,
    PALMAP *lpMap)
/*++
  Purpose:

    Get the logical height value of the specified tile. This value is used
    to judge whether the tile bitmap should cover the sprites or not.

  Parameters:

    [IN]  x - Column number of the tile.

    [IN]  y - Line number in the map.

    [IN]  h - Each line in the map has two lines of tiles, 0 and 1.
              (See map.h for details.)

    [IN]  ucLayer - The layer. 0 for bottom, 1 for top.

    [IN]  lpMap - Pointer to the loaded map.

  Return value:

    The logical height value of the specified tile.

--*/
{
   unsigned int d;

   //
   // Check for invalid parameters.
   //
   if (x >= PALMAP_X || y >= PALMAP_Y || h >= PALMAP_Z || lpMap == NULL)
   {
      return 0;
   }

   d = lpMap->Tiles[y * PALMAP_X * PALMAP_Z + x * PALMAP_Z + h];

   if (ucLayer)
   {
      d >>= 16;
   }

   d >>= 8;
   return (unsigned char)(d & 0xf);
}

void PAL_MapBlitToSurface(
    PALMAP *lpMap,
    const PAL_Rect *lpSrcRect,
    unsigned char ucLayer)
/*++
  Purpose:

    Blit the specified map area to a SDL Surface.

  Parameters:

    [IN]  lpMap - Pointer to the map.

    [IN]  lpSrcRect - Pointer to the source area.

    [IN]  ucLayer - The layer. 0 for bottom, 1 for top.

  Return value:

    None.

--*/
{
   int sx, sy, dx, dy, x, y, h, xPos, yPos;
   const unsigned char *lpBitmap = NULL;

   //
   // Convert the coordinate
   //
   sy = lpSrcRect->y / 16 - 1;
   dy = (lpSrcRect->y + lpSrcRect->h) / 16 + 2;
   sx = lpSrcRect->x / 32 - 1;
   dx = (lpSrcRect->x + lpSrcRect->w) / 32 + 2;

   //
   // Do the drawing.
   //
   yPos = sy * 16 - 8 - lpSrcRect->y;
   for (y = sy; y < dy; y++)
   {
      for (h = 0; h < 2; h++, yPos += 8)
      {
         xPos = sx * 32 + h * 16 - 16 - lpSrcRect->x;
         for (x = sx; x < dx; x++, xPos += 32)
         {
            lpBitmap = PAL_MapGetTileBitmap(x, y, h, ucLayer, lpMap);
            if (lpBitmap == NULL)
            {
               if (ucLayer)
               {
                  continue;
               }
               lpBitmap = PAL_MapGetTileBitmap(0, 0, 0, ucLayer, lpMap);
            }
            PAL_RLEBlitToSurfaceWithShadow(lpBitmap, gpScreen, PAL_XY(xPos, yPos), 0);
         }
      }
   }
}
