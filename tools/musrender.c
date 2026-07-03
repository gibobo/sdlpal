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
// musrender.c - Standalone offline music pre-render tool (own main()).
//
// Synthesizes every RIX track once via the engine's exact Nuked-OPL3 / RIX code
// (reused as-is, so the output is bit-identical to the game's live audio), then
// resamples to one or more target rates with the engine resampler and writes
// headered PCM files (see pcmmus.h) named <song>_<rate>.pcm, ready to drop into
// <SD>/mus/. Built as the `palmusrender` CMake target with the headless Dummy
// driver; it does NOT share main() with the game (src/main.c is compiled with
// PAL_NO_MAIN). Only the resource layer + the RIX player are initialized -- no
// video/text/font/UI subsystems (which need extra data files a music-only tool
// does not care about).
//
// Usage:
//   palmusrender --data <game-data-dir> --out <out-dir> [--rate R]... [--quality Q]
//     --data     directory containing MUS.MKF (e.g. .../Pal98rqptw)
//     --out      output directory (created if missing)
//     --rate     target sample rate in Hz; repeatable; default 22050 and 44100
//     --quality  resampler quality 0..4 (default 4 = SINC)
//

#include "audio.h"
#include "pcmmus.h"
#include "resource.h"
#include "sound/resampler.h"
#include "util.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#if defined(_WIN32)
#include <direct.h>
#define chdir _chdir
#else
#include <unistd.h>
#endif

#define MAX_RATES 8

/* Resample one channel of int16 samples by `factor` (= src_rate / dst_rate).
   Returns a malloc'd buffer of *n_out samples (caller frees). */
static int16_t *resample_channel(const int16_t *in, size_t n_in, double factor, int quality, size_t *n_out)
{
    void *r = resampler_create();
    size_t cap = (size_t)((double)n_in / factor) + 256;
    int16_t *out = (int16_t *)malloc(cap * sizeof(int16_t));
    size_t o = 0, i = 0;

    resampler_set_quality(r, quality);
    resampler_set_rate(r, factor);

    while (i < n_in || resampler_get_sample_count(r) > 0)
    {
        while (i < n_in && resampler_get_free_count(r) > 0)
            resampler_write_sample(r, in[i++]);
        while (resampler_get_sample_count(r) > 0)
        {
            if (o >= cap)
            {
                cap *= 2;
                out = (int16_t *)realloc(out, cap * sizeof(int16_t));
            }
            out[o++] = (int16_t)resampler_get_sample(r);
            resampler_remove_sample(r);
        }
        if (i >= n_in && resampler_get_sample_count(r) == 0)
            break; /* input exhausted and tail flushed */
    }

    resampler_delete(r);
    *n_out = o;
    return out;
}

/* Resample an interleaved master to `dst_rate` and write it (with header) to path. */
static void write_resampled(const char *path, const int16_t *master, size_t frames,
                            int channels, uint32_t base_rate, uint32_t dst_rate, int quality)
{
    void *fp = UTIL_fopen(path, "wb");
    if (fp == NULL)
    {
        fprintf(stderr, "render: cannot open %s\n", path);
        return;
    }
    PCMMUS_WriteHeader(fp, dst_rate, (uint16_t)channels);

    if (dst_rate == base_rate)
    {
        UTIL_fwrite((void *)master, sizeof(int16_t), frames * channels, fp);
    }
    else
    {
        double factor = (double)base_rate / (double)dst_rate;
        int16_t *chan_in = (int16_t *)malloc(frames * sizeof(int16_t));
        int16_t *chan_out[PAL_AUDIO_CHANNEL_COUNT];
        size_t out_frames = 0;
        int c;

        for (c = 0; c < channels; c++)
        {
            size_t f, n_out = 0;
            for (f = 0; f < frames; f++)
                chan_in[f] = master[f * channels + c];
            chan_out[c] = resample_channel(chan_in, frames, factor, quality, &n_out);
            if (n_out > out_frames)
                out_frames = n_out;
        }
        free(chan_in);

        {
            int16_t frame[PAL_AUDIO_CHANNEL_COUNT];
            size_t f;
            for (f = 0; f < out_frames; f++)
            {
                for (c = 0; c < channels; c++)
                    frame[c] = chan_out[c][f];
                UTIL_fwrite(frame, sizeof(int16_t), channels, fp);
            }
        }
        for (c = 0; c < channels; c++)
            free(chan_out[c]);
    }

    UTIL_fclose(fp);
}

int main(int argc, char *argv[])
{
    const char *data_dir = NULL;
    const char *out_dir = NULL;
    uint32_t rates[MAX_RATES];
    int n_rates = 0;
    int quality = RESAMPLER_QUALITY_SINC;
    int i, song;

    AUDIOPLAYER *p;
    int channels = PAL_AUDIO_CHANNEL_COUNT;
    uint32_t base_rate = PAL_AUDIO_SAMPLING_RATE;
    uint32_t chunk_samples = PAL_AUDIO_SAMPLES_PER_CHUNK * PAL_AUDIO_CHANNEL_COUNT;
    short *chunk;
    char out_abs[1024];

    /* Parse arguments. */
    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--data") == 0 && i + 1 < argc)
            data_dir = argv[++i];
        else if (strcmp(argv[i], "--out") == 0 && i + 1 < argc)
            out_dir = argv[++i];
        else if (strcmp(argv[i], "--rate") == 0 && i + 1 < argc && n_rates < MAX_RATES)
            rates[n_rates++] = (uint32_t)atoi(argv[++i]);
        else if (strcmp(argv[i], "--quality") == 0 && i + 1 < argc)
            quality = atoi(argv[++i]);
        else
        {
            fprintf(stderr, "usage: %s --data <dir> --out <dir> [--rate R]... [--quality 0..4]\n", argv[0]);
            return 2;
        }
    }
    if (data_dir == NULL || out_dir == NULL)
    {
        fprintf(stderr, "usage: %s --data <dir> --out <dir> [--rate R]... [--quality 0..4]\n", argv[0]);
        return 2;
    }
    if (n_rates == 0)
    {
        rates[n_rates++] = 22050;
        rates[n_rates++] = 44100;
    }

    /* Resolve the output directory to an absolute path (before chdir) and create it. */
#if defined(_WIN32)
    if (_fullpath(out_abs, out_dir, sizeof(out_abs)) == NULL)
    {
        strncpy(out_abs, out_dir, sizeof(out_abs) - 1);
        out_abs[sizeof(out_abs) - 1] = '\0';
    }
    _mkdir(out_abs);
#else
    if (out_dir[0] == '/')
    {
        strncpy(out_abs, out_dir, sizeof(out_abs) - 1);
        out_abs[sizeof(out_abs) - 1] = '\0';
    }
    else if (getcwd(out_abs, sizeof(out_abs)) != NULL)
    {
        size_t l = strlen(out_abs);
        snprintf(out_abs + l, sizeof(out_abs) - l, "/%s", out_dir);
    }
    else
    {
        strncpy(out_abs, out_dir, sizeof(out_abs) - 1);
        out_abs[sizeof(out_abs) - 1] = '\0';
    }
    mkdir(out_abs, 0755);
#endif

    if (chdir(data_dir) != 0)
    {
        fprintf(stderr, "render: cannot chdir to data dir '%s'\n", data_dir);
        return 1;
    }

    resampler_init();

    /* Minimal init: open the resource archives (for MUS.MKF), then create the
       RIX/OPL3 player. No video/text/font/UI -- a music-only tool needs none. */
    if (PAL_LoadConsolidatedResources() != 0)
    {
        fprintf(stderr, "render: failed to open game resources (is --data the game-data dir?)\n");
        return 1;
    }
    p = RIX_Init();
    if (p == NULL)
    {
        fprintf(stderr, "render: RIX player init failed\n");
        return 1;
    }

    chunk = (short *)UTIL_malloc(chunk_samples * sizeof(short));
    printf("base rate %u Hz, %d ch; targets:", base_rate, channels);
    for (i = 0; i < n_rates; i++)
        printf(" %u", rates[i]);
    printf("\n");

    for (song = 0; song <= 99; song++)
    {
        int16_t *master = NULL;
        size_t master_len = 0, master_cap = 0;
        int started = 0, guard = 0;
        const int guard_max = (int)PAL_AUDIO_CHUNK_PER_SECOND * 60 * 8; /* ~8 min cap */

        if (RES_MKFGetChunkSize((uint32_t)song, Res_MUS) == 0)
            continue;

        /* Start the track (no loop, no fade) and pump the RIX player one pass. */
        p->Play(p, song, false, 0.0f);
        do
        {
            int cur;
            memset(chunk, 0, chunk_samples * sizeof(short));
            p->FillBuffer(p, (uint8_t *)chunk, chunk_samples * sizeof(short));
            if (master_len + chunk_samples > master_cap)
            {
                master_cap = (master_cap == 0) ? (chunk_samples * 64) : (master_cap * 2);
                master = (int16_t *)realloc(master, master_cap * sizeof(int16_t));
            }
            memcpy(master + master_len, chunk, chunk_samples * sizeof(short));
            master_len += chunk_samples;

            cur = p->iMusic;
            if (cur == song)
                started = 1;
            guard++;
            if (started && cur != song)
                break; /* track started, then ended (iMusic -> -1) */
        } while (guard < guard_max);

        {
            size_t frames = master_len / channels;
            char path[512];
            for (i = 0; i < n_rates; i++)
            {
                snprintf(path, sizeof(path), "%s/%d_%u.pcm", out_abs, song, rates[i]);
                write_resampled(path, master, frames, channels, base_rate, rates[i], quality);
            }
            printf("song %3d: %d chunks (%zu frames @ %u Hz)\n", song, guard, frames, base_rate);
        }
        free(master);
    }

    UTIL_free(chunk);
    p->Shutdown(p);
    printf("done. copy %s/*.pcm into <SD>/mus/\n", out_abs);
    return 0;
}
