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

#ifndef UIGAME_H
#define UIGAME_H

#ifdef __cplusplus
extern "C" {
#endif

void PAL_OpeningMenu(void);

#ifdef __cplusplus
}
#endif

unsigned short
PAL_SaveSlotMenu(
   unsigned short        wDefaultSlot
);

unsigned short
PAL_TripleMenu(
   unsigned short  wThirdWord
);

int
PAL_ConfirmMenu(
   void
);

int
PAL_SwitchMenu(
   int      fEnabled
);

void
PAL_InGameMagicMenu(
   void
);

void
PAL_InGameMenu(
   void
);

void
PAL_PlayerStatus(
   void
);

unsigned short
PAL_ItemUseMenu(
   unsigned short           wItemToUse
);

void
PAL_BuyMenu(
   unsigned short           wStoreNum
);

void
PAL_SellMenu(
   void
);

void
PAL_EquipItemMenu(
   unsigned short           wItem
);

void
PAL_QuitGame(
   void
);

#endif
