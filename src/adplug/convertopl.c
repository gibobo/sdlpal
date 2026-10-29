#if defined(ESP_PLATFORM)
#pragma GCC optimize ("O2")
#endif

#include "convertopl.h"
#include "../util.h"
#include "nuked/opl3.h"
#include <stdio.h>
#include <string.h>

static short *buffer = NULL;
static uint32_t bufsamples = 0;
static uint32_t rate;
static uint8_t stereo_flag = 0;

void update_to_internal_buffer(uint32_t samples);
void update_direct(short *buf, uint32_t samples);
void update_16s_16m(short *buf, uint32_t samples);
void update_16s_8s(short *buf, uint32_t samples);
void update_16s_8m(short *buf, uint32_t samples);

// Resize the internal buffer is necessary before update data into it
void update_to_internal_buffer(uint32_t samples)
{
    if (bufsamples < samples)
    {
        bufsamples = samples;
        UTIL_free(buffer);
        buffer = (short *)UTIL_calloc(samples * 2, sizeof(short));
    }
}

// Same specification
void update_direct(short *buf, uint32_t samples)
{
    OPL3_GenerateStream(buf, samples);
}

// 16bit, stereo -> 16bit, mono
void update_16s_16m(short *buf, uint32_t samples)
{
    update_to_internal_buffer(samples);
    update_direct(buffer, samples);

    for (uint32_t i = 0, j = 0; i < samples; i++, j += 2)
    {
        buf[i] = (buffer[j] >> 1) + (buffer[j + 1] >> 1);
    }
}

// 16bit, stereo -> 8bit, stereo
void update_16s_8s(short *buf, uint32_t samples)
{
    update_to_internal_buffer(samples);
    update_direct(buffer, samples);

    for (uint32_t i = 0; i < samples * 2; i++)
    {
        ((char *)buf)[i] = (buffer[i] >> 8) ^ 0x80;
    }
}

// 16bit, stereo -> 8bit, stereo
void update_16s_8m(short *buf, uint32_t samples)
{
    update_to_internal_buffer(samples);
    update_direct(buffer, samples);

    for (uint32_t i = 0, j = 0; i < samples; i++, j += 2)
    {
        ((char *)buf)[i] = (((buffer[j] >> 1) + (buffer[j + 1] >> 1)) >> 8) ^ 0x80;
    }
}

void Copl_Init(uint32_t samplerate, uint8_t stereo)
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
void Copl_Reset(void)
{
    OPL3_Reset(rate);
}

// combined register select + data write
void Copl_Write(uint16_t reg, uint8_t val)
{
    OPL3_WriteRegBuffered((uint8_t)(reg & 0x00FF), val);
}

// Emulation only: fill buffer
void Copl_Generate(int16_t *buf, uint32_t samples)
{
    if (stereo_flag)
        update_direct(buf, samples);
    else
        update_16s_16m(buf, samples);
}
