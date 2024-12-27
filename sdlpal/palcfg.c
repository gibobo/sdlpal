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
#include "global.h"
#include "pal_config.h"
#include "palcommon.h"
#include "resampler.h"
#include <stdint.h>

#define MAKE_BOOLEAN(defv, minv, maxv)  { .bValue = defv }, { .bValue = minv }, { .bValue = maxv }
#define MAKE_INTEGER(defv, minv, maxv)  { .iValue = defv }, { .iValue = minv }, { .iValue = maxv }
#define MAKE_UNSIGNED(defv, minv, maxv) { .uValue = defv }, { .uValue = minv }, { .uValue = maxv }
#define MAKE_STRING(defv)               { .sValue = defv }, { .sValue = NULL }, { .sValue = NULL }

static const ConfigItem gConfigItems[PALCFG_ALL_MAX] = {
	{ PALCFG_FULLSCREEN,        PALCFG_BOOLEAN,  "FullScreen",        10, MAKE_BOOLEAN(FALSE,                         FALSE,                 TRUE) },
	{ PALCFG_KEEPASPECTRATIO,   PALCFG_BOOLEAN,  "KeepAspectRatio",   15, MAKE_BOOLEAN(TRUE,                          FALSE,                 TRUE) },
	{ PALCFG_STEREO,            PALCFG_BOOLEAN,  "Stereo",             6, MAKE_BOOLEAN(TRUE,                          FALSE,                 TRUE) },                  // Default for stereo audio
	{ PALCFG_USESURROUNDOPL,    PALCFG_BOOLEAN,  "UseSurroundOPL",    14, MAKE_BOOLEAN(TRUE,                          FALSE,                 TRUE) },                  // Default for using surround opl
	{ PALCFG_ENABLEKEYREPEAT,   PALCFG_BOOLEAN,  "EnableKeyRepeat",   15, MAKE_BOOLEAN(FALSE,                         FALSE,                 TRUE) },
	{ PALCFG_ENABLEGLSL,        PALCFG_BOOLEAN,  "EnableGLSL",        10, MAKE_BOOLEAN(TRUE,                          FALSE,                 TRUE) },

	{ PALCFG_SURROUNDOPLOFFSET, PALCFG_INTEGER,  "SurroundOPLOffset", 17, MAKE_INTEGER(384,                           INT32_MIN,             INT32_MAX) },
	{ PALCFG_LOGLEVEL,          PALCFG_INTEGER,  "LogLevel",           8, MAKE_INTEGER(PAL_DEFAULT_LOGLEVEL,          LOGLEVEL_MIN,          LOGLEVEL_MAX) },
	{ PALCFG_AUDIODEVICE,       PALCFG_INTEGER,  "AudioDevice",       11, MAKE_INTEGER(-1,                            INT32_MIN,             INT32_MAX) },

	{ PALCFG_AUDIOBUFFERSIZE,   PALCFG_UNSIGNED, "AudioBufferSize",   15, MAKE_UNSIGNED(PAL_AUDIO_DEFAULT_BUFFER_SIZE, 2,                     32768) },
	{ PALCFG_OPLSAMPLERATE,     PALCFG_UNSIGNED, "OPLSampleRate",     13, MAKE_UNSIGNED(49716,                         0,                     UINT32_MAX) },
	{ PALCFG_RESAMPLEQUALITY,   PALCFG_UNSIGNED, "ResampleQuality",   15, MAKE_UNSIGNED(RESAMPLER_QUALITY_MAX,         RESAMPLER_QUALITY_MIN, RESAMPLER_QUALITY_MAX) }, // Default for best quality
	{ PALCFG_SAMPLERATE,        PALCFG_UNSIGNED, "SampleRate",        10, MAKE_UNSIGNED(44100,                         0,                     PAL_MAX_SAMPLERATE) },
	{ PALCFG_MUSICVOLUME,       PALCFG_UNSIGNED, "MusicVolume",       11, MAKE_UNSIGNED(PAL_MAX_VOLUME,                0,                     PAL_MAX_VOLUME) },        // Default for maximum volume
	{ PALCFG_SOUNDVOLUME,       PALCFG_UNSIGNED, "SoundVolume",       11, MAKE_UNSIGNED(PAL_MAX_VOLUME,                0,                     PAL_MAX_VOLUME) },        // Default for maximum volume
	{ PALCFG_WINDOWHEIGHT,      PALCFG_UNSIGNED, "WindowHeight",      12, MAKE_UNSIGNED(PAL_DEFAULT_WINDOW_HEIGHT,     0,                     UINT32_MAX) },
	{ PALCFG_WINDOWWIDTH,       PALCFG_UNSIGNED, "WindowWidth",       11, MAKE_UNSIGNED(PAL_DEFAULT_WINDOW_WIDTH,      0,                     UINT32_MAX) },
    { PALCFG_TEXTUREHEIGHT,     PALCFG_UNSIGNED, "TextureHeight",     13, MAKE_UNSIGNED(PAL_DEFAULT_TEXTURE_HEIGHT,    0,                     UINT32_MAX) },
    { PALCFG_TEXTUREWIDTH,      PALCFG_UNSIGNED, "TextureWidth",      12, MAKE_UNSIGNED(PAL_DEFAULT_TEXTURE_WIDTH,     0,                     UINT32_MAX) },

	{ PALCFG_GAMEPATH,          PALCFG_STRING,   "GamePath",           8, MAKE_STRING(NULL) },
    { PALCFG_SAVEPATH,          PALCFG_STRING,   "SavePath",           8, MAKE_STRING(NULL) },
    { PALCFG_SHADERPATH,        PALCFG_STRING,   "ShaderPath",        10, MAKE_STRING(NULL) },
	{ PALCFG_SCALEQUALITY,      PALCFG_STRING,   "ScaleQuality",      12, MAKE_STRING("0") },
	{ PALCFG_SHADER,            PALCFG_STRING,   "Shader",             6, MAKE_STRING(SOURCE_DIR"/shaders/plain.glsl") },
};

static const char *music_types[] = { "MIDI", "RIX", "MP3", "OGG", "OPUS", "RAW" };
static const char *opl_chips[] = { "OPL2", "OPL3" };

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
	ConfigValue  values[PALCFG_ALL_MAX];
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
		.RoleLevelLabel      = PAL_XY(6, 32),
		.RoleHPLabel         = PAL_XY(6, 54),
		.RoleMPLabel         = PAL_XY(6, 76),
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

		// Extra Lines
		.ExtraItemDescLines  = PAL_XY(0, 0),
		.ExtraMagicDescLines = PAL_XY(0, 0),

		// Magic Menu Desc
		.MagicMPDescLines	= PAL_XY(5, 0),
		.MagicMPSlashPos	= PAL_XY(45, 14),
		.MagicMPNeededPos	= PAL_XY(15, 14),
		.MagicMPCurrentPos	= PAL_XY(50, 14),

		// Magic Desc Message Pos
		.MagicDescMsgPos	= PAL_XY(102, 0),
	};

	for (PALCFG_ITEM i = PALCFG_ALL_MIN; i < PALCFG_ALL_MAX; i++) values[i] = gConfigItems[i].DefaultValue;

	//
	// Set configurable global options
	//
	if (!gConfig.pszSavePath) gConfig.pszSavePath = gConfig.pszGamePath ? strdup(gConfig.pszGamePath) : strdup(PAL_SAVE_PREFIX);
	if (!gConfig.pszGamePath) gConfig.pszGamePath = strdup(PAL_PREFIX);
    if (!gConfig.pszShaderPath) gConfig.pszShaderPath = strdup(gConfig.pszGamePath);
	gConfig.dwWordLength = 10;	// This is the default value for Chinese version
	gConfig.ScreenLayout = screen_layout;

	gConfig.fIsWIN95 = FALSE;	// Default for DOS version
	gConfig.fUseSurroundOPL = values[PALCFG_STEREO].bValue && values[PALCFG_USESURROUNDOPL].bValue;
	gConfig.fEnableKeyRepeat = values[PALCFG_ENABLEKEYREPEAT].bValue;
	gConfig.fKeepAspectRatio = values[PALCFG_KEEPASPECTRATIO].bValue;
	gConfig.fFullScreen = values[PALCFG_FULLSCREEN].bValue;
	gConfig.iAudioChannels = values[PALCFG_STEREO].bValue ? 2 : 1;

	gConfig.iSurroundOPLOffset = values[PALCFG_SURROUNDOPLOFFSET].iValue;
	gConfig.iLogLevel = values[PALCFG_LOGLEVEL].iValue;
	gConfig.iAudioDevice = values[PALCFG_AUDIODEVICE].iValue;

	gConfig.iSampleRate = values[PALCFG_SAMPLERATE].uValue;
	gConfig.iOPLSampleRate = values[PALCFG_OPLSAMPLERATE].uValue;
	gConfig.iResampleQuality = values[PALCFG_RESAMPLEQUALITY].uValue;
	gConfig.wAudioBufferSize = (unsigned short)values[PALCFG_AUDIOBUFFERSIZE].uValue;
	gConfig.iMusicVolume = values[PALCFG_MUSICVOLUME].uValue;
	gConfig.iSoundVolume = values[PALCFG_SOUNDVOLUME].uValue;
	gConfig.pszShader = (char *)values[PALCFG_SHADER].sValue;
	gConfig.dwTextureWidth  = values[PALCFG_TEXTUREWIDTH].uValue;
	gConfig.dwTextureHeight = values[PALCFG_TEXTUREHEIGHT].uValue;
	gConfig.dwScreenWidth = values[PALCFG_WINDOWWIDTH].uValue;
	gConfig.dwScreenHeight = values[PALCFG_WINDOWHEIGHT].uValue;
}
