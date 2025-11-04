#include "convertopl.h"
#include "../util.h"
#include "nuked/opl3.h"
#include <stdio.h>
#include <string.h>

static short *buffer = NULL;
static unsigned int bufsamples = 0;
static unsigned int rate;
static unsigned char stereo_flag = 0;

// Resize the internal buffer is necessary before update data into it
void update_to_internal_buffer(unsigned int samples)
{
    if (bufsamples < samples)
    {
        bufsamples = samples;
        UTIL_free(buffer);
        buffer = (short *)UTIL_calloc(samples * 2, sizeof(short));
    }
}

// Same specification
void update_direct(short *buf, unsigned int samples)
{
    OPL3_GenerateStream(buf, samples);
}

// 16bit, stereo -> 16bit, mono
void update_16s_16m(short *buf, unsigned int samples)
{
    update_to_internal_buffer(samples);
    update_direct(buffer, samples);

    for (unsigned int i = 0, j = 0; i < samples; i++, j += 2)
    {
        buf[i] = (buffer[j] >> 1) + (buffer[j + 1] >> 1);
    }
}

// 16bit, stereo -> 8bit, stereo
void update_16s_8s(short *buf, unsigned int samples)
{
    update_to_internal_buffer(samples);
    update_direct(buffer, samples);

    for (unsigned int i = 0; i < samples * 2; i++)
    {
        ((char *)buf)[i] = (buffer[i] >> 8) ^ 0x80;
    }
}

// 16bit, stereo -> 8bit, stereo
void update_16s_8m(short *buf, unsigned int samples)
{
    update_to_internal_buffer(samples);
    update_direct(buffer, samples);

    for (unsigned int i = 0, j = 0; i < samples; i++, j += 2)
    {
        ((char *)buf)[i] = (((buffer[j] >> 1) + (buffer[j + 1] >> 1)) >> 8) ^ 0x80;
    }
}

void Copl_Init(unsigned int samplerate, unsigned char stereo)
{
    rate = samplerate;
    stereo_flag = stereo;
    Copl_Reset();
}

void Copl_Deinit(void)
{
    UTIL_free(buffer);
    buffer = NULL;
}

// reinitialize OPL chip(s)
void Copl_Reset()
{
    OPL3_Reset(rate);
};

// combined register select + data write
void Copl_Write(unsigned short reg, unsigned char val)
{
    OPL3_WriteRegBuffered(reg & 0xFF, val);
};

// Emulation only: fill buffer
void Copl_Generate(short *buf, unsigned int samples)
{
    if (stereo_flag)
        update_direct(buf, samples);
    else
        update_16s_16m(buf, samples);
}
