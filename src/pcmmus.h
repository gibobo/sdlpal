//
// Copyright (c) 2011-2024, SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 3
// as published by the Free Software Foundation.
//
// pcmmus.h - Pre-rendered PCM music player + on-disk format.
//

#ifndef PAL_PCMMUS_H
#define PAL_PCMMUS_H

#include "audio.h"
#include <stdint.h>

//
// Pre-rendered music files carry a 12-byte little-endian header, followed by raw
// interleaved signed-16-bit PCM samples:
//
//   offset  size  field
//   0       4     magic  = "PCM1"
//   4       2     version (=1)
//   6       2     channels (2 = stereo)
//   8       4     sample_rate (Hz; any value the Python tool chose)
//
// The header lets the runtime and tooling know the true rate/channels of a file
// without relying on the file name, so an arbitrary-rate file can be validated
// (and, once runtime rate-switching lands, played) after a plain SD swap.
//
#define PCMMUS_HEADER_SIZE 12
#define PCMMUS_MAGIC "PCM1"
#define PCMMUS_VERSION 1

/* Write the 12-byte header to an open (UTIL_fopen) stream. Returns 0 on success.
   Shared by the offline renderer (main.c) and any tool that emits these files. */
int PCMMUS_WriteHeader(void *fp, uint32_t sample_rate, uint16_t channels);

/* AUDIOPLAYER factory: streams pre-rendered PCM from <RESOURCE_PATH>/mus/.
   Returns NULL if no pre-rendered tracks exist (so the caller can fall back to RIX). */
AUDIOPLAYER *PCMMUS_Init(void);

#endif /* PAL_PCMMUS_H */
