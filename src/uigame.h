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

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

    void PAL_OpeningMenu(void);

#ifdef __cplusplus
}
#endif

uint16_t PAL_SaveSlotMenu(uint16_t wDefaultSlot);

uint16_t PAL_TripleMenu(uint16_t wThirdWord);

uint8_t PAL_ConfirmMenu(void);

uint8_t PAL_SwitchMenu(uint8_t fEnabled);

void PAL_InGameMagicMenu(void);

void PAL_InGameMenu(void);

void PAL_PlayerStatus(void);

uint16_t PAL_ItemUseMenu(uint16_t wItemToUse);

void PAL_BuyMenu(uint16_t wStoreNum);

void PAL_SellMenu(void);

void PAL_EquipItemMenu(uint16_t wItem);

void PAL_QuitGame(void);

#endif