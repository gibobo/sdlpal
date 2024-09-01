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
// win32/pal_config.h: WIN32-specific header.
//            @Author: Lou Yihua <louyihua@21cn.com>, 2016.
//

#pragma once

#define PAL_PREFIX "E:/Pal98rqptw/"
#define PAL_SAVE_PREFIX "E:/Pal98rqptw/"

#define PAL_DEFAULT_WINDOW_WIDTH 640
#define PAL_DEFAULT_WINDOW_HEIGHT 400
#define PAL_DEFAULT_FULLSCREEN_HEIGHT 480

#define PAL_VIDEO_INIT_FLAGS (SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | (gConfig.fFullScreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0))

#ifdef _M_ARM
#define PAL_SDL_INIT_FLAGS (SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE)
#else
#ifndef PAL_HAS_JOYSTICKS
#define PAL_HAS_JOYSTICKS 1
#endif
#define PAL_SDL_INIT_FLAGS (SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE | SDL_INIT_JOYSTICK)
#endif

#define PAL_PLATFORM NULL
#define PAL_CREDIT NULL
#define PAL_PORTYEAR NULL


// #define PAL_HAS_CONFIG_PAGE 1

#define PAL_FILESYSTEM_IGNORE_CASE 1

#define PAL_PATH_SEPARATORS "\\/"

#define PAL_IS_PATH_SEPARATOR(x) ((x) == '\\' || (x) == '/')

#ifdef __MINGW__
#define PAL_CONVERT_UTF8 SDL_iconv_utf8_locale
#else
#define strtok_r strtok_s
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define strdup _strdup //https://msdn.microsoft.com/en-us/library/ms235454(v=vs.140).aspx
#endif

// #include <malloc.h>
// #include <stdbool.h>
