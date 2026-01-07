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

#include <stdint.h>

int PAL_BattleSelectAutoTargetFrom(
    int);

int PAL_IsPlayerDying(
    uint16_t);

int PAL_IsPlayerHealthy(
    uint16_t wPlayerRole);

int PAL_BattleSelectAutoTarget(
    void);

void PAL_BattleUpdateFighters(
    void);

void PAL_BattlePlayerCheckReady(
    void);

void PAL_BattleStartFrame(
    void);

void PAL_BattleCommitAction(
    int fRepeat);

void PAL_BattlePlayerPerformAction(
    uint16_t wPlayerIndex);

void PAL_BattleEnemyPerformAction(
    uint16_t wEnemyIndex);

void PAL_BattleShowPlayerPreMagicAnim(
    uint16_t wPlayerIndex,
    int fSummon);

void PAL_BattleDelay(
    uint16_t wDuration,
    uint16_t wObjectID,
    int fUpdateGesture);

void PAL_BattleStealFromEnemy(
    uint16_t wTarget,
    uint16_t wStealRate);

void PAL_BattleSimulateMagic(
    int16_t sTarget,
    uint16_t wMagicObjectID,
    uint16_t wBaseDamage);

#endif