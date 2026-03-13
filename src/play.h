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

#ifndef PLAY_H
#define PLAY_H

#include <stdint.h>

typedef struct tagTRIGGERRANGE
{
    uint32_t rgPos[13];
} TRIGGERRANGE;

void PAL_GameUpdate(unsigned char fTrigger);

void PAL_GameUseItem(void);

void PAL_GameEquipItem(void);

#ifdef __cplusplus
extern "C" {
#endif

TRIGGERRANGE PAL_GetSearchTriggerRange(void);

void PAL_StartFrame(void);

#ifdef __cplusplus
}
#endif

#endif
