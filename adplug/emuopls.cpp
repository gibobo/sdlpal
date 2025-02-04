/*
 * Adplug - Replayer for many OPL2/OPL3 audio file formats.
 * Copyright (C) 1999 - 2005 Simon Peter, <dn.tlp@gmx.net>, et al.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * -------------------------------------------------------------------------
 * SDLPAL
 * Copyright (c) 2011-2024, SDLPAL development team.
 * All rights reserved.
 *
 * This file is part of SDLPAL.
 *
 * SDLPAL is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * -------------------------------------------------------------------------
 *
 * wrappers.cpp - Wrapper of several OPL2 emulators by Lou Yihua.
 *
 */

#include "emuopls.h"
#include "nuked/opl3.h"

#define OPL3_EXTREG_BASE 0x100
#define OPL3_4OP_REGISTER 0x104
#define OPL3_MODE_REGISTER 0x105

class NUKEDOPL3 : public OPLCORE {
public:
  NUKEDOPL3(uint32_t samplerate) : OPLCORE(samplerate) {}

  void Reset() { OPL3_Reset(&chip, rate); }
  void Write(uint32_t reg, uint8_t val) {
    if (reg == OPL3_4OP_REGISTER || reg == OPL3_MODE_REGISTER) {
      OPL3_WriteReg(&chip, (uint16_t)reg, val);
    } else {
      OPL3_WriteRegBuffered(&chip, (uint16_t)reg, val);
    }
  }
  void Generate(short *buf, int samples) { OPL3_GenerateStream(&chip, buf, samples); }
  OPLCORE *Duplicate() { return new NUKEDOPL3(rate); }

private:
  opl3_chip chip;
};

Copl *CEmuopl::CreateEmuopl(int rate) {
  return new CEmuopl(new NUKEDOPL3(rate), TYPE_OPL3);
}

CEmuopl::CEmuopl(OPLCORE *core, ChipType type) : Copl(type), opl3mode(false) {
  opl[0] = core;
  opl[1] = nullptr;
  init();
}

CEmuopl::~CEmuopl() {
  delete opl[0];
}

void CEmuopl::update(short *buf, int samples) {
  opl[0]->Generate(buf, samples);
}

void CEmuopl::write(int reg, int val) {
  if (reg == 0x105 && currType == TYPE_OPL3) {
    opl3mode = ((val & 0x1) == 0x1);
  } else {
    reg &= opl3mode ? 0x1FF : 0xFF;
  }
  opl[currChip]->Write(reg, (uint8_t)val);
}

void CEmuopl::init() {
  opl[0]->Reset();
  if (opl3mode) {
    opl[0]->Write(0x105, 1);
  }
}