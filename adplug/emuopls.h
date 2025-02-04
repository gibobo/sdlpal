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
 * emuopls.h - Wrapper of several OPL2 emulators by Lou Yihua.
 *
 */

#ifndef SDLPAL_EMUOPLS_H
#define SDLPAL_EMUOPLS_H

#include "opl.h"
#include <stdint.h>

// CEmuopl implements the base class of a OPL wrapper
// The DUALOPL2 mode should be implemented by a OPL3 core

class OPLCORE
{
public:
    OPLCORE(uint32_t rate) : rate(rate) {}
    virtual ~OPLCORE() {}
    virtual void Reset() = 0;
    virtual void Write(uint32_t reg, uint8_t val) = 0;
    virtual void Generate(short *buf, int samples) = 0;
    virtual OPLCORE *Duplicate() = 0;

protected:
    uint32_t rate;
};

class CEmuopl : public Copl
{
public:
    static Copl *CreateEmuopl(int rate);

    ~CEmuopl();

    // Assumes a 16-bit, mono output sample buffer @ OPL2 mode
    // Assumes a 16-bit, stereo output sample buffer @ OPL3/DUAL_OPL2 mode
    void update(short *buf, int samples);

    void write(int reg, int val);

    void init();

protected:
    CEmuopl(OPLCORE *core, ChipType type);

    OPLCORE *opl[2];
    bool opl3mode;
};

#endif
