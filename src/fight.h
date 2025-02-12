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

#ifndef FIGHT_H
#define FIGHT_H

int
PAL_BattleSelectAutoTargetFrom(
   int
);

int
PAL_IsPlayerDying(
   unsigned short
);

int
PAL_IsPlayerHealthy(
   unsigned short     wPlayerRole
);

int
PAL_BattleSelectAutoTarget(
   void
);

void
PAL_BattleUpdateFighters(
   void
);

void
PAL_BattlePlayerCheckReady(
   void
);

void
PAL_BattleStartFrame(
   void
);

void
PAL_BattleCommitAction(
   int         fRepeat
);

void
PAL_BattlePlayerPerformAction(
   unsigned short         wPlayerIndex
);

void
PAL_BattleEnemyPerformAction(
   unsigned short         wEnemyIndex
);

void
PAL_BattleShowPlayerPreMagicAnim(
   unsigned short         wPlayerIndex,
   int         fSummon
);

void
PAL_BattleDelay(
   unsigned short       wDuration,
   unsigned short       wObjectID,
   int       fUpdateGesture
);

void
PAL_BattleStealFromEnemy(
   unsigned short           wTarget,
   unsigned short           wStealRate
);

void
PAL_BattleSimulateMagic(
   short      sTarget,
   unsigned short       wMagicObjectID,
   unsigned short       wBaseDamage
);

#endif
