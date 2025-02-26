#include "opl.h"
#include "opl3.h"
#include "../util.h"
#include <stdio.h>
#include <string.h>

static short *buffer = NULL;
static unsigned int bufsamples = 0;
static unsigned int rate;
static opl3_chip chip;
static unsigned char stereo_flag = 0;

// Same specification
void update_direct(short *buf, unsigned int samples) {
  OPL3_GenerateStream(&chip, buf, samples);
}

// 16bit, stereo -> 16bit, mono
void update_16s_16m(short *buf, unsigned int samples) {
  // Resize the internal buffer is necessary before update data into it
  if (bufsamples < samples) {
    bufsamples = samples;
    UTIL_free(buffer);
    buffer = (short *)UTIL_calloc(samples * 2, sizeof(short));
  }
  update_direct(buffer, samples);

  for (unsigned int i = 0, j = 0; i < samples; i++, j += 2) {
    buf[i] = (buffer[j] >> 1) + (buffer[j + 1] >> 1);
  }
}

void Copl_init(unsigned int samplerate, unsigned char stereo) {
  rate = samplerate;
  stereo_flag = stereo;
  Copl_reset();
}

void Copl_deinit(void) {
  UTIL_free(buffer);
  buffer = NULL;
}

// reinitialize OPL chip(s)
void Copl_reset() {
  OPL3_Reset(&chip, rate);
};

// combined register select + data write
void Copl_write(int reg, int val) {
  OPL3_WriteRegBuffered(&chip, ((uint16_t)reg) & 0xFF, val);
};

// Emulation only: fill buffer
void Copl_update(short *buf, int samples) {
  if (stereo_flag)
    update_direct(buf, samples);
  else
    update_16s_16m(buf, samples);
}
