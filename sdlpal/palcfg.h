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

#define     PAL_MAX_SAMPLERATE           49716
#define     PAL_MAX_VOLUME               100

typedef enum tagPALCFG_ITEM
{
	PALCFG_ALL_MIN = 0,

	PALCFG_BOOLEAN_MIN = PALCFG_ALL_MIN,
	/* Booleans */
	PALCFG_FULLSCREEN = PALCFG_BOOLEAN_MIN,
	PALCFG_KEEPASPECTRATIO,
	PALCFG_STEREO,
	PALCFG_USESURROUNDOPL,
	PALCFG_ENABLEKEYREPEAT,
    PALCFG_ENABLEGLSL,
	/* Booleans */
	PALCFG_BOOLEAN_MAX,

	PALCFG_INTEGER_MIN = PALCFG_BOOLEAN_MAX,
	/* Integers */
	PALCFG_SURROUNDOPLOFFSET = PALCFG_INTEGER_MIN,
	PALCFG_LOGLEVEL,
	PALCFG_AUDIODEVICE,
	/* Integers */
	PALCFG_INTEGER_MAX,

	PALCFG_UNSIGNED_MIN = PALCFG_INTEGER_MAX,
	/* Unsigneds */
	PALCFG_AUDIOBUFFERSIZE = PALCFG_UNSIGNED_MIN,
	PALCFG_OPLSAMPLERATE,
	PALCFG_RESAMPLEQUALITY,
	PALCFG_SAMPLERATE,
	PALCFG_MUSICVOLUME,
	PALCFG_SOUNDVOLUME,
	PALCFG_WINDOWHEIGHT,
	PALCFG_WINDOWWIDTH,
    PALCFG_TEXTUREHEIGHT,
    PALCFG_TEXTUREWIDTH,
	/* Unsigneds */
	PALCFG_UNSIGNED_MAX,

	PALCFG_STRING_MIN = PALCFG_UNSIGNED_MAX,
	/* Strings */
	PALCFG_GAMEPATH = PALCFG_STRING_MIN,
    PALCFG_SAVEPATH,
    PALCFG_SHADERPATH,
	PALCFG_SCALEQUALITY,
	PALCFG_SHADER,
	/* Strings */
	PALCFG_STRING_MAX,

	PALCFG_ALL_MAX = PALCFG_STRING_MAX
} PALCFG_ITEM;

typedef enum tagPALCFG_TYPE
{
	PALCFG_STRING,
	PALCFG_BOOLEAN,
	PALCFG_INTEGER,
	PALCFG_UNSIGNED,
} PALCFG_TYPE;

typedef union tagConfigValue
{
	const char*   sValue;
	unsigned int    uValue;
	int      iValue;
	int     bValue;
} ConfigValue;

typedef struct tagConfigItem
{
	PALCFG_ITEM        Item;
	PALCFG_TYPE        Type;
	const char*        Name;
	int                NameLength;
	const ConfigValue  DefaultValue;
	const ConfigValue  MinValue;
	const ConfigValue  MaxValue;
} ConfigItem;

typedef struct tagSCREENLAYOUT
{
	unsigned int          EquipImageBox;
	unsigned int          EquipRoleListBox;
	unsigned int          EquipItemName;
	unsigned int          EquipItemAmount;
	unsigned int          EquipLabels[MAX_PLAYER_EQUIPMENTS];
	unsigned int          EquipNames[MAX_PLAYER_EQUIPMENTS];
	unsigned int          EquipStatusLabels[5];
	unsigned int          EquipStatusValues[5];

	unsigned int          RoleName;
	unsigned int          RoleImage;
	unsigned int          RoleExpLabel;
	unsigned int          RoleLevelLabel;
	unsigned int          RoleHPLabel;
	unsigned int          RoleMPLabel;
	unsigned int          RoleStatusLabels[5];
	unsigned int          RoleCurrExp;
	unsigned int          RoleNextExp;
	unsigned int          RoleExpSlash;
	unsigned int          RoleLevel;
	unsigned int          RoleCurHP;
	unsigned int          RoleMaxHP;
	unsigned int          RoleHPSlash;
	unsigned int          RoleCurMP;
	unsigned int          RoleMaxMP;
	unsigned int          RoleMPSlash;
	unsigned int          RoleStatusValues[5];
	unsigned int          RoleEquipImageBoxes[MAX_PLAYER_EQUIPMENTS];
	unsigned int          RoleEquipNames[MAX_PLAYER_EQUIPMENTS];
	unsigned int          RolePoisonNames[MAX_POISONS];

	unsigned int          ExtraItemDescLines;
	unsigned int          ExtraMagicDescLines;

	unsigned int			 MagicMPDescLines;
	unsigned int			 MagicMPSlashPos;
	unsigned int			 MagicMPNeededPos;
	unsigned int			 MagicMPCurrentPos;

	unsigned int			 MagicDescMsgPos;
} SCREENLAYOUT;

typedef struct tagCONFIGURATION
{
	union {
		SCREENLAYOUT     ScreenLayout;
		unsigned int          ScreenLayoutArray[sizeof(SCREENLAYOUT) / sizeof(unsigned int)];
	};
	enum {
		USE_8x8_FONT = 1,
		DISABLE_SHADOW = 2,
	}                ScreenLayoutFlag[sizeof(SCREENLAYOUT) / sizeof(unsigned int)];

	/* Configurable options */
	char            *pszGamePath;
	char            *pszSavePath;
    char            *pszShaderPath;
	char            *pszShader;
	unsigned int            dwWordLength;
	unsigned int            dwScreenWidth;
	unsigned int            dwScreenHeight;
    unsigned int            dwTextureWidth;
    unsigned int            dwTextureHeight;
	int              iAudioDevice;
	int              iSurroundOPLOffset;
	int              iAudioChannels;
	int              iSampleRate;
	int              iOPLSampleRate;
	int              iResampleQuality;
	int              iMusicVolume;
	int              iSoundVolume;
	LOGLEVEL         iLogLevel;
	unsigned short             wAudioBufferSize;
	int             fIsWIN95;
	int             fUseSurroundOPL;
	int             fKeepAspectRatio;
	int             fFullScreen;
	int             fEnableJoyStick;
	int             fUseCustomScreenLayout;
	int             fLaunchSetting;
	int             fEnableKeyRepeat;
} CONFIGURATION, *LPCONFIGURATION;

#ifdef __cplusplus
extern "C" {
#endif

extern CONFIGURATION gConfig;

void
PAL_LoadConfig(
	void
);

void
PAL_FreeConfig(
	void
);

#ifdef __cplusplus
}
#endif

#endif
