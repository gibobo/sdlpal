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
// palcfg.c: Configuration definition.
//  @Author: Lou Yihua <louyihua@21cn.com>, 2016.
//

#include "palcfg.h"
// #include "global.h"
#include "pal_config.h"
#include "palcommon.h"
// #include "common.h"
// #include <stdint.h>

void
PAL_FreeConfig(
	void
)
{
	free(gConfig.pszGamePath);
    free(gConfig.pszSavePath);
    free(gConfig.pszShaderPath);
	memset(&gConfig, 0, sizeof(CONFIGURATION));
}

void
PAL_LoadConfig(
	void
)
{
	// FILE     *fp;
	static const SCREENLAYOUT screen_layout = {
		// Equipment Screen
		.EquipImageBox     = PAL_XY(8, 8),
		.EquipRoleListBox  = PAL_XY(2, 95),
		.EquipItemName     = PAL_XY(5, 70),
		.EquipItemAmount   = PAL_XY(51, 57),
		.EquipLabels       = {
			PAL_XY(92, 11), PAL_XY(92, 33),
			PAL_XY(92, 55), PAL_XY(92, 77),
			PAL_XY(92, 99), PAL_XY(92, 121)
		},
		.EquipNames        = {
			PAL_XY(130, 11), PAL_XY(130, 33),
			PAL_XY(130, 55), PAL_XY(130, 77),
			PAL_XY(130, 99), PAL_XY(130, 121)
		},
		.EquipStatusLabels = {
			PAL_XY(226, 10), PAL_XY(226, 32),
			PAL_XY(226, 54), PAL_XY(226, 76),
			PAL_XY(226, 98)
		},
		.EquipStatusValues = {
			PAL_XY(260, 14), PAL_XY(260, 36),
			PAL_XY(260, 58), PAL_XY(260, 80),
			PAL_XY(260, 102)
		},

		// Status Screen
		.RoleName            = PAL_XY(110, 8),
		.RoleImage           = PAL_XY(110, 30),
		.RoleExpLabel        = PAL_XY(6, 6),
		.RoleStatusLabels    = {
			PAL_XY(6, 98),  PAL_XY(6, 118),
			PAL_XY(6, 138), PAL_XY(6, 158),
			PAL_XY(6, 178)
		},
		.RoleCurrExp         = PAL_XY(58, 6),
		.RoleNextExp         = PAL_XY(58, 15),
		.RoleExpSlash        = PAL_XY(0, 0),
		.RoleLevel           = PAL_XY(54, 35),
		.RoleCurHP           = PAL_XY(42, 56),
		.RoleMaxHP           = PAL_XY(63, 61),
		.RoleHPSlash         = PAL_XY(65, 58),
		.RoleCurMP           = PAL_XY(42, 78),
		.RoleMaxMP           = PAL_XY(63, 83),
		.RoleMPSlash         = PAL_XY(65, 80),
		.RoleStatusValues    = {
			PAL_XY(42, 102), PAL_XY(42, 122),
			PAL_XY(42, 142), PAL_XY(42, 162),
			PAL_XY(42, 182)
		},
		.RoleEquipImageBoxes = {
			PAL_XY(189, -1),  PAL_XY(247, 39),
			PAL_XY(251, 101), PAL_XY(201, 133),
			PAL_XY(141, 141), PAL_XY(81, 125)
		},
		.RoleEquipNames      = {
			PAL_XY(195, 38),  PAL_XY(253, 78),
			PAL_XY(257, 140), PAL_XY(207, 172),
			PAL_XY(147, 180), PAL_XY(87, 164)
		},
		.RolePoisonNames     = {
			PAL_XY(185, 58),  PAL_XY(185, 76),
			PAL_XY(185, 94),  PAL_XY(185, 112),
			PAL_XY(185, 130), PAL_XY(185, 148),
			PAL_XY(185, 166), PAL_XY(185, 184),
			PAL_XY(185, 184), PAL_XY(185, 184)
		},
	};

	//
	// Set configurable global options
	//
	if (!gConfig.pszSavePath) gConfig.pszSavePath = strdup(PAL_SAVE_PREFIX);
	if (!gConfig.pszGamePath) gConfig.pszGamePath = strdup(PAL_PREFIX);
    if (!gConfig.pszShaderPath) gConfig.pszShaderPath = strdup(gConfig.pszGamePath);
	gConfig.ScreenLayout = screen_layout;

	gConfig.fEnableKeyRepeat = FALSE;
	gConfig.fKeepAspectRatio = TRUE;
	gConfig.fFullScreen = FALSE;
	gConfig.iAudioChannels = TRUE ? 2 : 1;
	gConfig.fUseSurroundOPL = (gConfig.iAudioChannels==2) && TRUE;
	gConfig.iSurroundOPLOffset = 384;
	gConfig.iLogLevel = LOGLEVEL_MIN;
	gConfig.iAudioDevice = -1;

	gConfig.iSampleRate = 44100;
	gConfig.iOPLSampleRate = 49716;
	gConfig.wAudioBufferSize = 1024;
	gConfig.pszShader = SOURCE_DIR"/shaders/plain.glsl";
	gConfig.dwTextureWidth  = PAL_DEFAULT_WINDOW_WIDTH;
	gConfig.dwTextureHeight = PAL_DEFAULT_WINDOW_HEIGHT;
	gConfig.dwScreenWidth = PAL_DEFAULT_WINDOW_WIDTH;
	gConfig.dwScreenHeight = PAL_DEFAULT_WINDOW_HEIGHT;
}
