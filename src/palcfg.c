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
#include "palcommon.h"
#include "common.h"
// #include <stdint.h>

void
PAL_FreeConfig(
	void
)
{
	free(gConfig.pszGamePath);
    free(gConfig.pszSavePath);
	memset(&gConfig, 0, sizeof(CONFIGURATION));
}

void
PAL_LoadConfig(
	void
)
{
	// Set configurable global options
	if (!gConfig.pszSavePath)	gConfig.pszSavePath = strdup(SOURCE_DIR "/Pal98rqptw/");
	if (!gConfig.pszGamePath)	gConfig.pszGamePath = strdup(SOURCE_DIR "/Pal98rqptw/");
	if (!gConfig.pszShader)		gConfig.pszShader 	= strdup(SOURCE_DIR "/shaders/plain.glsl");

	gConfig.fEnableKeyRepeat = FALSE;
	gConfig.iAudioChannels = TRUE ? 2 : 1;
	gConfig.iAudioDevice = -1;

	gConfig.iSampleRate = 44100;
	gConfig.iOPLSampleRate = 49716;
	gConfig.wAudioBufferSize = 1024;
	gConfig.dwTextureWidth  = 640;
	gConfig.dwTextureHeight = 400;
}
