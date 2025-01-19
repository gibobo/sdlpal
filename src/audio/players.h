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
// players.h: Common definition of sound/music players.
//   @Author: Lou Yihua <louyihua@21cn.com>, 2015-07-28.
//

#ifndef PLAYERS_H
#define PLAYERS_H

typedef struct tagAUDIOPLAYER {
#define AUDIOPLAYER_COMMONS             \
  int iMusic;                           \
  int fLoop;                            \
  void (*Shutdown)(void *);             \
  int (*Play)(void *, int, int, float); \
  void (*FillBuffer)(void *, unsigned char *, int)
  AUDIOPLAYER_COMMONS;
} AUDIOPLAYER;

#ifdef __cplusplus
extern "C" {
#endif
/* RIX */
AUDIOPLAYER *RIX_Init(const char *szFileName);
AUDIOPLAYER *SOUND_Init(void);

#ifdef __cplusplus
}
#endif

#endif
