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

#ifndef RES_H
#define RES_H

enum tagLOADRESFLAG
{
    kLoadNone = 0,                // no need to load anything
    kLoadGlobalData = (1 << 0),   // load global data
    kLoadScene = (1 << 1),        // load a scene
    kLoadPlayerSprite = (1 << 2), // load player sprites
};

void PAL_InitResources(void);

void PAL_FreeResources(void);

void PAL_SetLoadFlags(unsigned char bFlags);

void *PAL_GetCurrentMap(void);

unsigned char *PAL_GetPlayerSprite(unsigned char bPlayerIndex);

unsigned char *PAL_GetEventObjectSprite(unsigned short wEventObjectID);

#ifdef __cplusplus
extern "C" {
#endif

void PAL_LoadResources(void);

#ifdef __cplusplus
}
#endif

#endif
