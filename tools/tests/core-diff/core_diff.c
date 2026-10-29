// Differential tests: run development's core routines and the upstream
// (origin/master) implementations on the same game data and require identical
// results. Usage: core_diff <test>|all
#include "core_diff_api.h"
#include "global.h"
#include "palcommon.h"
#include "util.h"
#include "video.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_REPORTED_FAILURES 10
#define CANARY_SIZE           64
#define CANARY_BYTE           0xA5
#define SPRITE_PADDING        4096

typedef struct Tally
{
    const char *name;
    long cases;
    long failures;
} Tally;

static uint8_t g_masterPixels[SCREEN_SIZE];
static uint8_t g_devPixels[SCREEN_SIZE];
static uint8_t g_patternPixels[SCREEN_SIZE];
static MasterSurface g_masterSurface = {SCREEN_W, SCREEN_H, SCREEN_W, g_masterPixels};
static VIDEO_Surface g_devSurface = {SCREEN_W, SCREEN_H, g_devPixels};

// Record a failed case; only the first few are printed.
static void Fail(Tally *tally, const char *fmt, ...)
{
    va_list args;
    if (++tally->failures <= MAX_REPORTED_FAILURES)
    {
        fprintf(stderr, "  [%s] ", tally->name);
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
        fputc('\n', stderr);
    }
}

// Print the verdict of one test and return its exit status.
static int Report(const Tally *tally)
{
    printf("%s %s: %ld cases, %ld failures\n", tally->failures == 0 ? "PASS" : "FAIL", tally->name,
           tally->cases, tally->failures);
    return tally->failures == 0 ? 0 : 1;
}

// Open a game data file; missing data is a configuration error.
static FILE *OpenData(const char *name)
{
    char path[1024];
    FILE *fp;
    snprintf(path, sizeof(path), "%s/%s", PAL_TEST_RESOURCE_DIR, name);
    fp = fopen(path, "rb");
    if (fp == NULL)
    {
        fprintf(stderr, "cannot open game data file '%s'\n", path);
        exit(2);
    }
    return fp;
}

// Open a game data archive with the development MKF reader.
static PALMKF OpenArchive(const char *name)
{
    PALMKF mkf;
    PAL_MKFOpen(&mkf, OpenData(name));
    return mkf;
}

// Read a raw chunk with SPRITE_PADDING zero bytes appended; NULL for empty chunks.
static uint8_t *ReadRawChunk(const PALMKF *mkf, uint32_t chunk, uint32_t *len)
{
    uint8_t *buf;
    *len = PAL_MKFGetChunkSize(chunk, mkf);
    if (*len == 0)
        return NULL;
    buf = (uint8_t *)calloc(1, *len + SPRITE_PADDING);
    PAL_MKFReadChunk(buf, *len, chunk, mkf);
    return buf;
}

// Read the 32-bit little-endian value at p.
static uint32_t ReadLE32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

// Decompress a YJ2 stream with the upstream decompressor (reference output).
static uint8_t *MasterDecompress(const uint8_t *src, uint32_t *size)
{
    uint8_t *dst;
    *size = ReadLE32(src);
    dst = (uint8_t *)calloc(1, *size + SPRITE_PADDING);
    if (master_YJ2_Decompress(src, dst, (int)*size) != (int)*size)
    {
        free(dst);
        return NULL;
    }
    return dst;
}

// Compare the development decompressor with the upstream one on a YJ2 stream.
static void CompareYJ2Stream(Tally *tally, const uint8_t *src, const char *what, uint32_t index)
{
    uint32_t size;
    uint8_t *ref = MasterDecompress(src, &size);
    uint8_t *out;
    uint32_t i;

    tally->cases++;
    if (ref == NULL)
    {
        Fail(tally, "%s %u: upstream decompressor rejected the stream", what, index);
        return;
    }

    // Run the development decompressor with a canary after the destination.
    out = (uint8_t *)malloc(size + CANARY_SIZE);
    memset(out + size, CANARY_BYTE, CANARY_SIZE);
    if (YJ2_Decompress(src, out, size) != (int32_t)size)
        Fail(tally, "%s %u: YJ2_Decompress did not return the decompressed size %u", what, index, size);
    if (size > 0 && YJ2_Decompress(src, out, size - 1) != -1)
        Fail(tally, "%s %u: YJ2_Decompress accepted a destination one byte too small", what, index);
    if (memcmp(out, ref, size) != 0)
        Fail(tally, "%s %u: decompressed bytes differ", what, index);
    for (i = 0; i < CANARY_SIZE; i++)
    {
        if (out[size + i] != CANARY_BYTE)
        {
            Fail(tally, "%s %u: wrote past the %u-byte destination", what, index, size);
            break;
        }
    }
    free(out);
    free(ref);
}

// Read every chunk of every archive with the development reader, which keeps the
// offset table in memory, and with the upstream reader, which reads it from the
// file each time; then read every RNG frame and compare it with the frame taken
// from the animation chunk.
static int TestMKFRead(void)
{
    static const char *files[] = {"abc.mkf",  "ball.mkf", "data.mkf", "f.mkf",   "fbp.mkf",
                                  "fire.mkf", "gop.mkf",  "map.mkf",  "mgo.mkf", "mus.mkf",
                                  "pat.mkf",  "rgm.mkf",  "rng.mkf",  "sounds.mkf", "sss.mkf"};
    Tally tally = {"mkf_read", 0, 0};
    uint8_t probe[4];
    size_t f;

    for (f = 0; f < sizeof(files) / sizeof(files[0]); f++)
    {
        PALMKF mkf = OpenArchive(files[f]);
        FILE *fp = OpenData(files[f]);
        int count = master_PAL_MKFGetChunkCount(fp);
        uint32_t chunk;

        tally.cases++;
        if ((int)PAL_MKFGetChunkCount(&mkf) != count)
            Fail(&tally, "%s: %u chunks, upstream counts %d", files[f], PAL_MKFGetChunkCount(&mkf), count);
        for (chunk = 0; chunk < (uint32_t)count; chunk++)
        {
            int size = master_PAL_MKFGetChunkSize(chunk, fp);
            uint32_t len = PAL_MKFGetChunkSize(chunk, &mkf);
            uint8_t *ref, *buf;
            int refLen;
            uint32_t got;

            tally.cases++;
            if (size < 0 ? len != 0 : len != (uint32_t)size)
            {
                Fail(&tally, "%s %u: size %u, upstream %d", files[f], chunk, len, size);
                continue;
            }
            if (len == 0)
                continue;
            ref = (uint8_t *)malloc(len);
            buf = (uint8_t *)malloc(len);
            refLen = master_PAL_MKFReadChunk(ref, len, chunk, fp);
            got = PAL_MKFReadChunk(buf, len, chunk, &mkf);
            if (refLen != (int)len || got != len || memcmp(ref, buf, len) != 0)
                Fail(&tally, "%s %u: read %u bytes, upstream %d, or the bytes differ", files[f], chunk, got, refLen);
            if (len > 1 && PAL_MKFReadChunk(buf, len - 1, chunk, &mkf) != 0)
                Fail(&tally, "%s %u: read into a buffer that is too small", files[f], chunk);
            free(ref);
            free(buf);
        }

        // Past the last chunk there is nothing to read.
        tally.cases++;
        if (PAL_MKFGetChunkSize((uint32_t)count, &mkf) != 0 || PAL_MKFReadChunk(probe, sizeof(probe), (uint32_t)count, &mkf) != 0)
            Fail(&tally, "%s: chunk %d past the end is not empty", files[f], count);
        fclose(fp);
        PAL_MKFClose(&mkf);
    }

    // Every frame of every RNG animation, and one past the last frame.
    {
        PALMKF mkf = OpenArchive("rng.mkf");
        FILE *fp = OpenData("rng.mkf");
        int count = master_PAL_MKFGetChunkCount(fp), movie;

        for (movie = 0; movie < count; movie++)
        {
            int size = master_PAL_MKFGetChunkSize((unsigned int)movie, fp);
            uint8_t *animation, *buf = NULL;
            uint32_t frames, frame;

            if (size <= 0)
            {
                tally.cases++;
                if (PAL_RNGReadFrame(&buf, (uint32_t)movie, 0, &mkf) != -3)
                    Fail(&tally, "rng %d: an empty animation has frames", movie);
                continue;
            }
            animation = (uint8_t *)malloc((size_t)size);
            master_PAL_MKFReadChunk(animation, (unsigned int)size, (unsigned int)movie, fp);
            frames = ReadLE32(animation) / 4 - 1;
            for (frame = 0; frame <= frames; frame++)
            {
                int32_t got = PAL_RNGReadFrame(&buf, (uint32_t)movie, frame, &mkf);
                tally.cases++;
                if (frame == frames)
                {
                    if (got != -4)
                        Fail(&tally, "rng %d: frame %u past the end returned %d", movie, frame, got);
                }
                else
                {
                    uint32_t start = ReadLE32(animation + 4 * frame);
                    uint32_t len = ReadLE32(animation + 4 * frame + 4) - start;
                    if (got != (int32_t)len || (len != 0 && memcmp(buf, animation + start, len) != 0))
                        Fail(&tally, "rng %d frame %u: read %d bytes, expected %u", movie, frame, got, len);
                }
            }
            UTIL_free(buf);
            free(animation);
        }
        fclose(fp);
        PAL_MKFClose(&mkf);
    }
    return Report(&tally);
}


// YJ2 decompression of every compressed MKF chunk and every RNG frame.
static int TestYJ2Chunks(void)
{
    static const char *files[] = {"abc.mkf", "f.mkf", "fbp.mkf", "fire.mkf", "map.mkf", "mgo.mkf"};
    Tally tally = {"yj2_chunks", 0, 0};
    size_t f;

    for (f = 0; f < sizeof(files) / sizeof(files[0]); f++)
    {
        PALMKF mkf = OpenArchive(files[f]);
        uint32_t count = PAL_MKFGetChunkCount(&mkf);
        uint32_t chunk;
        for (chunk = 0; chunk < count; chunk++)
        {
            uint32_t len, size;
            uint8_t *raw = ReadRawChunk(&mkf, chunk, &len);
            uint8_t *buf = NULL;
            uint8_t *ref;
            uint32_t ret;
            if (raw == NULL)
                continue;
            CompareYJ2Stream(&tally, raw, files[f], chunk);

            // Check the chunk API end to end: upstream returns the decompressed size.
            ref = MasterDecompress(raw, &size);
            ret = PAL_MKFDecompressChunk(&buf, 0, chunk, &mkf);
            tally.cases++;
            if (ref == NULL || buf == NULL || memcmp(buf, ref, size) != 0)
                Fail(&tally, "%s %u: PAL_MKFDecompressChunk output differs", files[f], chunk);
            if (ret != size)
                Fail(&tally, "%s %u: PAL_MKFDecompressChunk returned %u, upstream returns the decompressed size %u",
                     files[f], chunk, ret, size);
            UTIL_free(buf);
            free(ref);
            free(raw);
        }
        PAL_MKFClose(&mkf);
    }

    // Decompress every frame of every RNG movie.
    {
        PALMKF mkf = OpenArchive("rng.mkf");
        uint32_t count = PAL_MKFGetChunkCount(&mkf);
        uint32_t movie;
        for (movie = 0; movie < count; movie++)
        {
            uint32_t len, frames, frame;
            uint8_t *raw = ReadRawChunk(&mkf, movie, &len);
            if (raw == NULL)
                continue;
            frames = (ReadLE32(raw) >> 2) - 1;
            for (frame = 0; frame < frames; frame++)
            {
                uint32_t off = ReadLE32(raw + 4 * frame);
                uint32_t next = ReadLE32(raw + 4 * (frame + 1));
                if (next > off)
                    CompareYJ2Stream(&tally, raw + off, "rng frame", movie * 1000 + frame);
            }
            free(raw);
        }
        PAL_MKFClose(&mkf);
    }

    return Report(&tally);
}

// Blit one RLE bitmap with every variant at one position on both canvases.
static void BlitCases(Tally *tally, const uint8_t *rle, int x, int y, const char *what, uint32_t index)
{
    int h = master_PAL_RLEGetHeight(rle);
    int r0 = y < 0 ? 0 : (y > SCREEN_H ? SCREEN_H : y);
    int r1 = y + h < 0 ? 0 : (y + h > SCREEN_H ? SCREEN_H : y + h);
    uint32_t pos = PAL_XY(x, y);
    int variant;

    for (variant = 0; variant < 6; variant++)
    {
        int m = 0, d = 0;
        size_t first = (size_t)r0 * SCREEN_W, bytes = (size_t)(r1 - r0) * SCREEN_W;

        // Restore the rows this blit can touch, then blit with both versions.
        memcpy(g_masterPixels + first, g_patternPixels + first, bytes);
        memcpy(g_devPixels + first, g_patternPixels + first, bytes);
        switch (variant)
        {
        case 0:
            m = master_PAL_RLEBlitToSurface(rle, &g_masterSurface, pos);
            d = PAL_RLEBlitToSurface(rle, &g_devSurface, pos);
            break;
        case 1:
            m = master_PAL_RLEBlitToSurfaceWithShadow(rle, &g_masterSurface, pos, 1);
            d = PAL_RLEBlitToSurfaceWithShadow(rle, &g_devSurface, pos, 1);
            break;
        case 2:
            m = master_PAL_RLEBlitWithColorShift(rle, &g_masterSurface, pos, -3);
            d = PAL_RLEBlitWithColorShift(rle, &g_devSurface, pos, -3);
            break;
        case 3:
            m = master_PAL_RLEBlitWithColorShift(rle, &g_masterSurface, pos, 2);
            d = PAL_RLEBlitWithColorShift(rle, &g_devSurface, pos, 2);
            break;
        case 4:
            m = master_PAL_RLEBlitMonoColor(rle, &g_masterSurface, pos, 0x1B, 0);
            d = PAL_RLEBlitMonoColor(rle, &g_devSurface, pos, 0x1B, 0);
            break;
        default:
            m = master_PAL_RLEBlitMonoColor(rle, &g_masterSurface, pos, 0x5B, -2);
            d = PAL_RLEBlitMonoColor(rle, &g_devSurface, pos, 0x5B, -2);
            break;
        }

        // Compare the return value and every row the blit could have changed.
        tally->cases++;
        if (m != d)
            Fail(tally, "%s %u at (%d,%d) variant %d: returned %d, upstream %d", what, index, x, y, variant, d, m);
        if (bytes != 0 && memcmp(g_masterPixels + first, g_devPixels + first, bytes) != 0)
            Fail(tally, "%s %u at (%d,%d) variant %d: pixels differ", what, index, x, y, variant);
    }
}

// Blit one RLE bitmap at edge and interior positions; then check the whole canvas.
static void BlitAllPositions(Tally *tally, const uint8_t *rle, const char *what, uint32_t index)
{
    int w = master_PAL_RLEGetWidth(rle);
    int h = master_PAL_RLEGetHeight(rle);
    int positions[][2] = {
        {0, 0}, {SCREEN_W / 2 - w / 2, SCREEN_H / 2 - h / 2}, {-w / 2, -h / 2}, {SCREEN_W - w / 2, SCREEN_H - h / 2},
        {SCREEN_W - w, SCREEN_H - h}, {-w + 1, 50}, {100, -h + 1}, {SCREEN_W - 1, SCREEN_H - 1},
        {(int)(index * 37 % SCREEN_W) - w / 3, (int)(index * 53 % SCREEN_H) - h / 3},
    };
    size_t p;

    tally->cases++;
    if (w != PAL_RLEGetWidth(rle) || h != PAL_RLEGetHeight(rle))
        Fail(tally, "%s %u: size %ux%u, upstream %dx%d", what, index, PAL_RLEGetWidth(rle), PAL_RLEGetHeight(rle), w, h);
    if (w <= 0 || h <= 0)
        return;

    for (p = 0; p < sizeof(positions) / sizeof(positions[0]); p++)
        BlitCases(tally, rle, positions[p][0], positions[p][1], what, index);

    // Catch writes outside the rows each case compared, then resynchronize.
    if (memcmp(g_masterPixels, g_devPixels, SCREEN_SIZE) != 0)
    {
        Fail(tally, "%s %u: pixels outside the blitted rows differ", what, index);
        memcpy(g_devPixels, g_masterPixels, SCREEN_SIZE);
    }
}

// Walk every frame of a sprite sheet with both frame-lookup implementations.
static void BlitSheet(Tally *tally, const uint8_t *sheet, uint32_t size, const char *what, uint32_t chunk)
{
    uint16_t frames = master_PAL_SpriteGetNumFrames(sheet);
    uint16_t i;

    tally->cases++;
    if (frames != PAL_SpriteGetNumFrames(sheet))
        Fail(tally, "%s %u: %u frames, upstream %u", what, chunk, PAL_SpriteGetNumFrames(sheet), frames);

    for (i = 0; i < frames; i++)
    {
        const uint8_t *m = master_PAL_SpriteGetFrame(sheet, i);
        const uint8_t *d = PAL_SpriteGetFrame(sheet, i);
        tally->cases++;
        if (m != d)
        {
            Fail(tally, "%s %u frame %u: frame lookup differs", what, chunk, i);
            continue;
        }
        if (m == NULL || (uint32_t)(m - sheet) >= size)
            continue;
        BlitAllPositions(tally, m, what, chunk * 1000 + i);
    }
}

// RLE blitting of every sprite: all variants, interior and clipped positions.
static int TestRLEBlits(void)
{
    static const char *compressed[] = {"mgo.mkf", "abc.mkf", "f.mkf", "fire.mkf"};
    Tally tally = {"rle_blits", 0, 0};
    size_t f;
    uint32_t i;

    for (i = 0; i < SCREEN_SIZE; i++)
        g_patternPixels[i] = (uint8_t)(i * 7 + i / SCREEN_W * 13);
    memcpy(g_masterPixels, g_patternPixels, SCREEN_SIZE);
    memcpy(g_devPixels, g_patternPixels, SCREEN_SIZE);

    // Compressed sprite sheets.
    for (f = 0; f < sizeof(compressed) / sizeof(compressed[0]); f++)
    {
        PALMKF mkf = OpenArchive(compressed[f]);
        uint32_t count = PAL_MKFGetChunkCount(&mkf), chunk;
        for (chunk = 0; chunk < count; chunk++)
        {
            uint32_t len, size;
            uint8_t *raw = ReadRawChunk(&mkf, chunk, &len);
            uint8_t *sheet;
            if (raw == NULL)
                continue;
            sheet = MasterDecompress(raw, &size);
            if (sheet != NULL)
                BlitSheet(&tally, sheet, size, compressed[f], chunk);
            free(sheet);
            free(raw);
        }
        PAL_MKFClose(&mkf);
    }

    // Raw sprite sheets: map tiles (every 8th tile set) and the UI sprites.
    {
        PALMKF mkf = OpenArchive("gop.mkf");
        uint32_t count = PAL_MKFGetChunkCount(&mkf), chunk;
        for (chunk = 0; chunk < count; chunk += 8)
        {
            uint32_t len;
            uint8_t *raw = ReadRawChunk(&mkf, chunk, &len);
            if (raw != NULL)
                BlitSheet(&tally, raw, len, "gop.mkf", chunk);
            free(raw);
        }
        PAL_MKFClose(&mkf);

        mkf = OpenArchive("data.mkf");
        {
            uint32_t len;
            uint8_t *raw = ReadRawChunk(&mkf, 9, &len);
            if (raw != NULL)
                BlitSheet(&tally, raw, len, "data.mkf", 9);
            free(raw);
        }
        PAL_MKFClose(&mkf);
    }

    // Single raw bitmaps: item images and faces.
    {
        static const char *single[] = {"ball.mkf", "rgm.mkf"};
        for (f = 0; f < sizeof(single) / sizeof(single[0]); f++)
        {
            PALMKF mkf = OpenArchive(single[f]);
            uint32_t count = PAL_MKFGetChunkCount(&mkf), chunk;
            for (chunk = 0; chunk < count; chunk++)
            {
                uint32_t len;
                uint8_t *raw = ReadRawChunk(&mkf, chunk, &len);
                if (raw != NULL)
                    BlitAllPositions(&tally, raw, single[f], chunk);
                free(raw);
            }
            PAL_MKFClose(&mkf);
        }
    }

    // Full-screen backgrounds.
    {
        PALMKF mkf = OpenArchive("fbp.mkf");
        uint32_t count = PAL_MKFGetChunkCount(&mkf), chunk;
        for (chunk = 0; chunk < count; chunk++)
        {
            uint32_t len, size;
            uint8_t *raw = ReadRawChunk(&mkf, chunk, &len);
            uint8_t *fbp;
            int m, d;
            if (raw == NULL)
                continue;
            fbp = MasterDecompress(raw, &size);
            if (fbp != NULL && size >= SCREEN_SIZE)
            {
                m = master_PAL_FBPBlitToSurface(fbp, &g_masterSurface);
                d = PAL_FBPBlitToSurface(fbp, &g_devSurface);
                tally.cases++;
                if (m != d || memcmp(g_masterPixels, g_devPixels, SCREEN_SIZE) != 0)
                {
                    Fail(&tally, "fbp.mkf %u: background blit differs (returned %d, upstream %d)", chunk, d, m);
                    memcpy(g_devPixels, g_masterPixels, SCREEN_SIZE);
                }
            }
            free(fbp);
            free(raw);
        }
        PAL_MKFClose(&mkf);
    }

    return Report(&tally);
}

// PAL_XY_OFFSET against the coordinate arithmetic upstream scripts and callers perform.
static int TestXYOffset(void)
{
    static const int ys[] = {-400, -16, -8, -1, 0, 1, 8, 16, 400};
    Tally tally = {"xy_offset", 0, 0};
    uint32_t bases[1007] = {
        PAL_XY(0, 0), PAL_XY(160, 112), PAL_XY(2047, 2047), PAL_XY(-1, -1),
        PAL_XY(0x7FFF, 0x7FFF), PAL_XY(0x8000, 0x8000), PAL_XY(-160, -112),
    };
    uint32_t b, seed = 12345;

    for (b = 7; b < sizeof(bases) / sizeof(bases[0]); b++)
    {
        seed = seed * 1103515245u + 12345u;
        bases[b] = PAL_XY((int)(seed >> 16) % 2560 - 256, (int)(seed & 0xFFFF) % 2560 - 256);
    }

    // The reference adds offsets to each coordinate independently, e.g. script 0x006E.
    for (b = 0; b < sizeof(bases) / sizeof(bases[0]); b++)
    {
        int x;
        size_t yi;
        for (x = -400; x <= 400; x++)
        {
            for (yi = 0; yi < sizeof(ys) / sizeof(ys[0]); yi++)
            {
                uint32_t got = PAL_XY_OFFSET(bases[b], x, ys[yi]);
                uint32_t want = PAL_XY(PAL_X(bases[b]) + x, PAL_Y(bases[b]) + ys[yi]);
                tally.cases++;
                if (got != want)
                    Fail(&tally, "(%d,%d) + (%d,%d): got (%d,%d), want (%d,%d)", PAL_X(bases[b]), PAL_Y(bases[b]), x,
                         ys[yi], PAL_X(got), PAL_Y(got), PAL_X(want), PAL_Y(want));
            }
        }
    }
    return Report(&tally);
}

// RandomLong / RandomFloat sequences from identically seeded generators.
static int TestRNGStream(void)
{
    static const int froms[] = {0, 1, -1, 100};
    static const float floatRanges[][2] = {{10.0f, 11.0f}, {0.9f, 1.1f}, {1.0f, 1.125f}, {0.0f, 1.0f}};
    static const int largeRanges[][2] = {{0, 1499},  {0, 2047},   {0, 4095},      {0, 9998},
                                         {0, 32767}, {0, 65534},  {0, 65535},     {-1024, 0},
                                         {-37, 0},   {-1, 65535}, {1000, 65535}};
    Tally tally = {"rng_stream", 0, 0};
    size_t fi;
    int n, k;

    for (fi = 0; fi < sizeof(froms) / sizeof(froms[0]); fi++)
    {
        for (n = 1; n <= 1024; n++)
        {
            int from = froms[fi], to = from + n - 1;
            master_SeedRandom(0x1234u + (unsigned)n);
            UTIL_SetRandomSeed(0x1234u + (unsigned)n);
            for (k = 0; k < 2000; k++)
            {
                int m = master_RandomLong(from, to);
                int d = RandomLong(from, to);
                tally.cases++;
                if (m != d)
                    Fail(&tally, "RandomLong(%d,%d) draw %d: got %d, upstream %d", from, to, k, d, m);
            }
            if (master_GetRandomState() != UTIL_GetRandomState())
                Fail(&tally, "RandomLong(%d,%d): generator states diverged", from, to);
        }
    }

    // Data-driven ranges reach 65535 (script operands, defense) and negative bounds (battle blow).
    for (fi = 0; fi < sizeof(largeRanges) / sizeof(largeRanges[0]); fi++)
    {
        int from = largeRanges[fi][0], to = largeRanges[fi][1];
        master_SeedRandom(0x5678u + (unsigned)fi);
        UTIL_SetRandomSeed(0x5678u + (unsigned)fi);
        for (k = 0; k < 200000; k++)
        {
            int m = master_RandomLong(from, to);
            int d = RandomLong(from, to);
            tally.cases++;
            if (m != d)
                Fail(&tally, "RandomLong(%d,%d) draw %d: got %d, upstream %d", from, to, k, d, m);
        }
    }

    for (fi = 0; fi < sizeof(floatRanges) / sizeof(floatRanges[0]); fi++)
    {
        master_SeedRandom(0xBEEFu);
        UTIL_SetRandomSeed(0xBEEFu);
        for (k = 0; k < 1000000; k++)
        {
            float m = master_RandomFloat(floatRanges[fi][0], floatRanges[fi][1]);
            float d = RandomFloat(floatRanges[fi][0], floatRanges[fi][1]);
            tally.cases++;
            if (memcmp(&m, &d, sizeof(float)) != 0)
                Fail(&tally, "RandomFloat(%g,%g) draw %d: got %.9g, upstream %.9g", floatRanges[fi][0],
                     floatRanges[fi][1], k, d, m);
        }
    }

    // Degenerate ranges return the lower bound without consuming a number.
    master_SeedRandom(7u);
    UTIL_SetRandomSeed(7u);
    tally.cases++;
    if (master_RandomLong(5, 5) != RandomLong(5, 5) || master_RandomLong(5, 3) != RandomLong(5, 3) ||
        master_GetRandomState() != UTIL_GetRandomState())
        Fail(&tally, "degenerate ranges differ");

    return Report(&tally);
}

// Load the object, magic and battlefield tables used by the damage formulas.
static void LoadDamageData(void)
{
    PALMKF sss = OpenArchive("sss.mkf");
    PALMKF data = OpenArchive("data.mkf");
    uint32_t len;
    uint8_t *raw;

    gpGlobals = (GLOBALVARS *)calloc(1, sizeof(GLOBALVARS));
    raw = ReadRawChunk(&sss, 2, &len);
    memcpy(gpGlobals->g.rgObject, raw, len < sizeof(gpGlobals->g.rgObject) ? len : sizeof(gpGlobals->g.rgObject));
    free(raw);
    gpGlobals->g.lprgMagic = (MAGIC *)ReadRawChunk(&data, 4, &len);
    gpGlobals->g.nMagic = (int32_t)(len / sizeof(MAGIC));
    gpGlobals->g.lprgBattleField = (BATTLEFIELD *)ReadRawChunk(&data, 5, &len);
    gpGlobals->g.nBattleField = (int32_t)(len / sizeof(BATTLEFIELD));
    PAL_MKFClose(&sss);
    PAL_MKFClose(&data);
}

// Base, physical and magic damage formulas over the game's value ranges.
static int TestDamage(void)
{
    static const uint16_t strengths[] = {0, 1, 10, 50, 99, 100, 150, 200, 255, 300, 400, 500, 700, 999, 1500};
    static const uint16_t defenses[] = {0, 10, 50, 100, 200, 400, 999};
    static const uint16_t resistances[][NUM_MAGIC_ELEMENTAL] = {{0, 0, 0, 0, 0}, {1, 2, 3, 4, 5}, {10, 5, 0, 7, 3}};
    static const uint16_t poisons[] = {0, 5, 10};
    static const uint16_t multipliers[] = {1, 10};
    Tally tally = {"damage", 0, 0};
    uint32_t a, d, r, seed = 1;
    uint16_t obj;

    LoadDamageData();

    // Base and physical damage: every strength/defense pair the stats allow.
    for (a = 0; a < 2048; a++)
    {
        for (d = 0; d < 2048; d++)
        {
            short m = master_CalcBaseDamage((uint16_t)a, (uint16_t)d);
            short v = dev_CalcBaseDamage((uint16_t)a, (uint16_t)d);
            tally.cases++;
            if (m != v)
                Fail(&tally, "base(%u,%u): got %d, upstream %d", a, d, v, m);
            if (a % 3 == 0 && d % 3 == 0)
            {
                for (r = 0; r <= 4; r++)
                {
                    m = master_CalcPhysicalAttackDamage((uint16_t)a, (uint16_t)d, (uint16_t)r);
                    v = dev_CalcPhysicalAttackDamage((uint16_t)a, (uint16_t)d, (uint16_t)r);
                    tally.cases++;
                    if (m != v)
                        Fail(&tally, "physical(%u,%u,%u): got %d, upstream %d", a, d, r, v, m);
                }
            }
        }
    }

    // Magic damage: every object whose magic number is valid, with both generators in step.
    for (obj = 0; obj < MAX_OBJECTS; obj++)
    {
        size_t si, di, ri, pi, mi;
        uint16_t field;
        if (gpGlobals->g.rgObject[obj].magic.wMagicNumber >= gpGlobals->g.nMagic)
            continue;
        for (field = 0; field < 4 && field < gpGlobals->g.nBattleField; field++)
        {
            gpGlobals->wNumBattleField = field;
            for (si = 0; si < sizeof(strengths) / sizeof(strengths[0]); si++)
                for (di = 0; di < sizeof(defenses) / sizeof(defenses[0]); di++)
                    for (ri = 0; ri < sizeof(resistances) / sizeof(resistances[0]); ri++)
                        for (pi = 0; pi < sizeof(poisons) / sizeof(poisons[0]); pi++)
                            for (mi = 0; mi < sizeof(multipliers) / sizeof(multipliers[0]); mi++)
                            {
                                short m, v;
                                master_SeedRandom(seed);
                                UTIL_SetRandomSeed(seed);
                                seed++;
                                m = master_CalcMagicDamage(strengths[si], defenses[di], resistances[ri], poisons[pi],
                                                           multipliers[mi], obj);
                                v = dev_CalcMagicDamage(strengths[si], defenses[di], resistances[ri], poisons[pi],
                                                        multipliers[mi], obj);
                                tally.cases++;
                                if (m != v)
                                    Fail(&tally, "magic(obj %u, str %u, def %u, res %zu, poison %u, mult %u, field %u): got %d, upstream %d",
                                         obj, strengths[si], defenses[di], ri, poisons[pi], multipliers[mi], field, v, m);
                            }
        }
    }

    // Magic strength scaling rounds through float, so sweep every strength for a few magics.
    gpGlobals->wNumBattleField = 0;
    for (obj = 0, r = 0; obj < MAX_OBJECTS && r < 16; obj++)
    {
        uint32_t s, k;
        if (gpGlobals->g.rgObject[obj].magic.wMagicNumber >= gpGlobals->g.nMagic)
            continue;
        r++;
        for (s = 0; s < 2048; s++)
        {
            for (k = 0; k < 4; k++)
            {
                short m, v;
                master_SeedRandom(seed);
                UTIL_SetRandomSeed(seed);
                seed++;
                m = master_CalcMagicDamage((uint16_t)s, 100, resistances[1], 5, 10, obj);
                v = dev_CalcMagicDamage((uint16_t)s, 100, resistances[1], 5, 10, obj);
                tally.cases++;
                if (m != v)
                    Fail(&tally, "magic sweep(obj %u, str %u): got %d, upstream %d", obj, s, v, m);
            }
        }
    }
    return Report(&tally);
}

// RNG movie playback: every frame applied in order to persistent canvases.
static int TestRNGBlit(void)
{
    Tally tally = {"rng_blit", 0, 0};
    PALMKF mkf = OpenArchive("rng.mkf");
    uint32_t count = PAL_MKFGetChunkCount(&mkf), movie;

    for (movie = 0; movie < count; movie++)
    {
        uint32_t len, frames, frame;
        uint8_t *raw = ReadRawChunk(&mkf, movie, &len);
        if (raw == NULL)
            continue;
        memset(g_masterPixels, 0, SCREEN_SIZE);
        memset(g_devPixels, 0, SCREEN_SIZE);
        frames = (ReadLE32(raw) >> 2) - 1;
        for (frame = 0; frame < frames; frame++)
        {
            uint32_t off = ReadLE32(raw + 4 * frame), next = ReadLE32(raw + 4 * (frame + 1)), size;
            uint8_t *rng;
            int m, d;
            if (next <= off)
                continue;
            rng = MasterDecompress(raw + off, &size);
            if (rng == NULL)
                continue;
            m = master_RNGBlitToSurface(rng, (int)size, &g_masterSurface);
            d = dev_RNGBlitToSurface(rng, (int32_t)size, g_devPixels);
            tally.cases++;
            if (m != d || memcmp(g_masterPixels, g_devPixels, SCREEN_SIZE) != 0)
            {
                Fail(&tally, "movie %u frame %u: returned %d, upstream %d%s", movie, frame, d, m,
                     memcmp(g_masterPixels, g_devPixels, SCREEN_SIZE) != 0 ? "; pixels differ" : "");
                memcpy(g_devPixels, g_masterPixels, SCREEN_SIZE);
            }
            free(rng);
        }
        free(raw);
    }
    PAL_MKFClose(&mkf);
    return Report(&tally);
}

typedef struct TestEntry
{
    const char *name;
    int (*run)(void);
} TestEntry;

static const TestEntry g_tests[] = {
    {"yj2_chunks", TestYJ2Chunks}, {"rle_blits", TestRLEBlits}, {"xy_offset", TestXYOffset},
    {"rng_stream", TestRNGStream}, {"damage", TestDamage},       {"rng_blit", TestRNGBlit},
    {"mkf_read", TestMKFRead},
};

// Run the named test, or all of them.
int main(int argc, char *argv[])
{
    const char *which = argc > 1 ? argv[1] : "all";
    size_t i;
    int status = 0, found = 0;

    // Configure the upstream code for the WIN95 data set.
    master_gConfig.fIsWIN95 = 1;
    master_Decompress = master_YJ2_Decompress;

    printf("upstream %s, data %s\n", PAL_MASTER_COMMIT, PAL_TEST_RESOURCE_DIR);
    for (i = 0; i < sizeof(g_tests) / sizeof(g_tests[0]); i++)
    {
        if (strcmp(which, "all") == 0 || strcmp(which, g_tests[i].name) == 0)
        {
            found = 1;
            status |= g_tests[i].run();
        }
    }
    if (!found)
    {
        fprintf(stderr, "unknown test '%s'\n", which);
        return 2;
    }
    return status;
}
