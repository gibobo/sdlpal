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

#ifndef MAIN_H
#define MAIN_H

#include "audio.h"
#include "battle.h"
#include "ending.h"
#include "fight.h"
#include "font.h"
#include "game.h"
#include "global.h"
#include "input.h"
#include "itemmenu.h"
#include "magicmenu.h"
#include "map.h"
#include "palcfg.h"
#include "palcommon.h"
#include "palette.h"
#include "play.h"
#include "players.h"
#include "res.h"
#include "rngplay.h"
#include "scene.h"
#include "script.h"
#include "text.h"
#include "ui.h"
#include "uibattle.h"
#include "uigame.h"
#include "util.h"
#include "video.h"

VOID PAL_Shutdown(
    int exit_code);

#endif
