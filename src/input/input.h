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

#ifndef INPUT_H
#define INPUT_H

#define PAL_HAS_JOYSTICKS

enum PALDIRECTION
{
   kDirSouth = 0,
   kDirWest,
   kDirNorth,
   kDirEast,
   kDirUnknown
};

enum PALKEY
{
   kKeyNone        = 0,
   kKeyMenu        = (1 << 0),
   kKeySearch      = (1 << 1),
   kKeyDown        = (1 << 2),
   kKeyLeft        = (1 << 3),
   kKeyUp          = (1 << 4),
   kKeyRight       = (1 << 5),
   kKeyPgUp        = (1 << 6),
   kKeyPgDn        = (1 << 7),
   kKeyRepeat      = (1 << 8),
   kKeyAuto        = (1 << 9),
   kKeyDefend      = (1 << 10),
   kKeyUseItem     = (1 << 11),
   kKeyThrowItem   = (1 << 12),
   kKeyFlee        = (1 << 13),
   kKeyStatus      = (1 << 14),
   kKeyForce       = (1 << 15),
   kKeyHome        = (1 << 16),
   kKeyEnd         = (1 << 17),
};

typedef struct tagPALINPUTSTATE
{
   unsigned char          dir;
   unsigned int           dwKeyPress;
   unsigned int           dwKeyOrder[4];
   unsigned int           dwKeyMaxCount;
} PALINPUTSTATE;

#ifdef __cplusplus
extern "C" {
#endif

void
PAL_ClearKeyState(
   void
);

void
PAL_InitInput(
   void
);

void
PAL_ProcessEvent(
   void
);

void
PAL_ShutdownInput(
   void
);

void
PAL_SetKeyInput(
   unsigned int key
);

unsigned int
PAL_GetKeyInput(
   void
);

void
PAL_SetDirInput(
   unsigned char dir
);

unsigned char
PAL_GetDirInput(
   void
);

#ifdef __cplusplus
}
#endif

#endif
