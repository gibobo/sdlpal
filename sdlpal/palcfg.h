/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
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
// palcfg.h: Configuration definition.
//  @Author: Lou Yihua <louyihua@21cn.com>, 2016.
//

#ifndef CONFIG_H
#define CONFIG_H

#include "global.h"
#include "util.h"

typedef struct tagSCREENLAYOUT {
  unsigned int EquipImageBox;
  unsigned int EquipRoleListBox;
  unsigned int EquipItemName;
  unsigned int EquipItemAmount;
  unsigned int EquipLabels[MAX_PLAYER_EQUIPMENTS];
  unsigned int EquipNames[MAX_PLAYER_EQUIPMENTS];
  unsigned int EquipStatusLabels[5];
  unsigned int EquipStatusValues[5];
  unsigned int RoleName;
  unsigned int RoleImage;
  unsigned int RoleExpLabel;
  unsigned int RoleLevelLabel;
  unsigned int RoleHPLabel;
  unsigned int RoleMPLabel;
  unsigned int RoleStatusLabels[5];
  unsigned int RoleCurrExp;
  unsigned int RoleNextExp;
  unsigned int RoleExpSlash;
  unsigned int RoleLevel;
  unsigned int RoleCurHP;
  unsigned int RoleMaxHP;
  unsigned int RoleHPSlash;
  unsigned int RoleCurMP;
  unsigned int RoleMaxMP;
  unsigned int RoleMPSlash;
  unsigned int RoleStatusValues[5];
  unsigned int RoleEquipImageBoxes[MAX_PLAYER_EQUIPMENTS];
  unsigned int RoleEquipNames[MAX_PLAYER_EQUIPMENTS];
  unsigned int RolePoisonNames[MAX_POISONS];
  unsigned int ExtraItemDescLines;
  unsigned int ExtraMagicDescLines;
  unsigned int MagicMPDescLines;
  unsigned int MagicMPSlashPos;
  unsigned int MagicMPNeededPos;
  unsigned int MagicMPCurrentPos;
  unsigned int MagicDescMsgPos;
} SCREENLAYOUT;

typedef struct tagCONFIGURATION {
  union {
    SCREENLAYOUT ScreenLayout;
    unsigned int ScreenLayoutArray[sizeof(SCREENLAYOUT) / sizeof(unsigned int)];
  };

  /* Configurable options */
  char *pszGamePath;
  char *pszSavePath;
  char *pszShader;
  unsigned int dwTextureWidth;
  unsigned int dwTextureHeight;
  int iAudioDevice;
  int iSurroundOPLOffset;
  int iAudioChannels;
  int iSampleRate;
  int iOPLSampleRate;
  unsigned short wAudioBufferSize;
  int fUseSurroundOPL;
  int fKeepAspectRatio;
  int fUseCustomScreenLayout;
  int fEnableKeyRepeat;
} CONFIGURATION;

#ifdef __cplusplus
extern "C" {
#endif

void
PAL_LoadConfig(
	void
);

void
PAL_FreeConfig(
	void
);

extern CONFIGURATION gConfig;

#ifdef __cplusplus
}
#endif

#endif
