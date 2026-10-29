// Replay video driver: records one line per presented frame (present index,
// virtual time, frame number, state hash, screen hash, palette hash) and can
// save chosen frames as PNG files for writing scenarios. The screen hash and the
// PNG files show the displayed picture, composed like the real video drivers.
//
// Environment: PAL_REPLAY_OUT (trace file, required), PAL_REPLAY_WORK_MS (virtual
// milliseconds of simulated work per present, default 0), PAL_REPLAY_DUMP
// (comma-separated present indices to save as <trace>.<index>.png).
#include "../../../src/driver.h"
#include "../../../src/global.h"
#include "../../../src/util.h"
#include "../../../src/video.h"
#include "../../../driver/DrvIf_internal.h"
#include "replay.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_DUMPS 64

static FILE *g_fpTrace = NULL;
static const char *g_pszTrace = NULL;
static uint32_t g_nPresents = 0;
static uint32_t g_dwWorkMs = 0;
static uint8_t g_rgPalette[256 * 3];
static uint8_t g_rgDisplay[SCREEN_SIZE];
static uint64_t g_qwPaletteHash = REPLAY_FNV_OFFSET;
static uint32_t g_rgDumps[MAX_DUMPS];
static int g_nDumps = 0;

// Present index of the last frame shown.
uint32_t REPLAY_GetPresentCount(void)
{
    return g_nPresents;
}

// Update a CRC-32 (PNG polynomial) with a byte range.
static uint32_t Crc32(uint32_t crc, const uint8_t *p, size_t n)
{
    size_t i;
    int k;
    crc = ~crc;
    for (i = 0; i < n; i++)
    {
        crc ^= p[i];
        for (k = 0; k < 8; k++)
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}

// Write a big-endian 32-bit value.
static void PutBE32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v >> 24);
    p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8);
    p[3] = (uint8_t)v;
}

// Write one PNG chunk with its length and CRC.
static void WriteChunk(FILE *fp, const char *type, const uint8_t *data, uint32_t len)
{
    uint8_t buf[4];
    uint32_t crc = Crc32(0, (const uint8_t *)type, 4);
    crc = Crc32(crc, data, len);
    PutBE32(buf, len);
    fwrite(buf, 1, 4, fp);
    fwrite(type, 1, 4, fp);
    fwrite(data, 1, len, fp);
    PutBE32(buf, crc);
    fwrite(buf, 1, 4, fp);
}

// Save the 8-bit frame through the current palette as an uncompressed RGB PNG.
static void DumpFrame(const uint8_t *frame, const char *path)
{
    static const uint8_t signature[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    const uint32_t rowBytes = SCREEN_W * 3 + 1, rawSize = rowBytes * SCREEN_H;
    const uint32_t blocks = (rawSize + 65534) / 65535;
    uint8_t *raw = (uint8_t *)malloc(rawSize);
    uint8_t *idat = (uint8_t *)malloc(2 + rawSize + blocks * 5 + 4);
    uint8_t ihdr[13] = {0};
    uint32_t x, y, pos = 0, a = 1, b = 0, i;
    FILE *fp = fopen(path, "wb");
    if (fp == NULL || raw == NULL || idat == NULL)
    {
        fprintf(stderr, "cannot write frame dump '%s'\n", path);
        exit(2);
    }

    // Expand the palette indices to RGB rows, each with filter type 0.
    for (y = 0; y < SCREEN_H; y++)
    {
        raw[y * rowBytes] = 0;
        for (x = 0; x < SCREEN_W; x++)
            memcpy(raw + y * rowBytes + 1 + x * 3, g_rgPalette + frame[y * SCREEN_W + x] * 3, 3);
    }

    // Wrap the rows in a zlib stream of stored deflate blocks.
    idat[pos++] = 0x78;
    idat[pos++] = 0x01;
    for (i = 0; i < rawSize; i += 65535)
    {
        uint32_t len = rawSize - i < 65535 ? rawSize - i : 65535;
        idat[pos++] = (uint8_t)(i + len == rawSize);
        idat[pos++] = (uint8_t)len;
        idat[pos++] = (uint8_t)(len >> 8);
        idat[pos++] = (uint8_t)~len;
        idat[pos++] = (uint8_t)(~len >> 8);
        memcpy(idat + pos, raw + i, len);
        pos += len;
    }
    for (i = 0; i < rawSize; i++)
    {
        a = (a + raw[i]) % 65521;
        b = (b + a) % 65521;
    }
    PutBE32(idat + pos, (b << 16) | a);
    pos += 4;

    // Emit the PNG: 8-bit RGB, no interlace.
    PutBE32(ihdr, SCREEN_W);
    PutBE32(ihdr + 4, SCREEN_H);
    ihdr[8] = 8;
    ihdr[9] = 2;
    fwrite(signature, 1, sizeof(signature), fp);
    WriteChunk(fp, "IHDR", ihdr, sizeof(ihdr));
    WriteChunk(fp, "IDAT", idat, pos);
    WriteChunk(fp, "IEND", NULL, 0);
    fclose(fp);
    free(raw);
    free(idat);
}

// Open the trace and read the work and dump settings.
int DRIVER_Init_Video(void)
{
    const char *work = getenv("PAL_REPLAY_WORK_MS");
    const char *dumps = getenv("PAL_REPLAY_DUMP");

    g_pszTrace = getenv("PAL_REPLAY_OUT");
    if (g_pszTrace == NULL || (g_fpTrace = fopen(g_pszTrace, "w")) == NULL)
    {
        fprintf(stderr, "PAL_REPLAY_OUT must name a writable trace file\n");
        exit(2);
    }
    g_dwWorkMs = work != NULL ? (uint32_t)strtoul(work, NULL, 10) : 0;
    while (dumps != NULL && *dumps != '\0' && g_nDumps < MAX_DUMPS)
    {
        char *end;
        g_rgDumps[g_nDumps++] = (uint32_t)strtoul(dumps, &end, 10);
        dumps = (*end == ',') ? end + 1 : end;
    }
    return 0;
}

// Close the trace.
void DRIVER_DeInit_Video(void)
{
    if (g_fpTrace != NULL)
        fclose(g_fpTrace);
    g_fpTrace = NULL;
}

// Remember the palette, which the next present displays.
void DRIVER_UpdatePalette(const uint8_t *rgPalette)
{
    memcpy(g_rgPalette, rgPalette, sizeof(g_rgPalette));
    g_qwPaletteHash = REPLAY_HashBytes(REPLAY_FNV_OFFSET, g_rgPalette, sizeof(g_rgPalette));
}

// Copy the region of interest to the displayed picture, as the video drivers do:
// without padding, rows outside the region keep what was shown before; with
// padding, the region shows consecutive frame rows from the first one and the
// rest of the picture is black.
static void ComposeDisplay(const uint8_t *frame, uint16_t roi_x, uint16_t roi_y, uint16_t roi_w, uint16_t roi_h,
                           uint8_t padding_flag)
{
    const uint8_t *src = frame;
    uint32_t x, y;

    for (y = 0; y < SCREEN_H; y++)
    {
        uint8_t *dst = g_rgDisplay + y * SCREEN_W;
        if (y >= roi_y && y < (uint32_t)roi_y + roi_h)
        {
            for (x = 0; x < SCREEN_W; x++)
            {
                if (x >= roi_x && x < (uint32_t)roi_x + roi_w)
                    dst[x] = src[x];
                else if (padding_flag)
                    dst[x] = 0;
            }
            src += SCREEN_W;
        }
        else if (padding_flag)
            memset(dst, 0, SCREEN_W);
        else
            src += SCREEN_W;
    }
}

// Record the presented frame and charge the simulated work to the virtual clock.
void DRIVER_FrameShow(
    uint8_t *frame,
    const uint16_t roi_x,
    const uint16_t roi_y,
    const uint16_t roi_w,
    const uint16_t roi_h,
    const uint8_t padding_flag)
{
    uint64_t screen;
    int i;

    ComposeDisplay(frame, roi_x, roi_y, roi_w, roi_h, padding_flag);
    screen = REPLAY_HashBytes(REPLAY_FNV_OFFSET, g_rgDisplay, SCREEN_SIZE);
    g_nPresents++;
    REPLAY_AdvanceClock(g_dwWorkMs);
    fprintf(g_fpTrace, "%u %u %u %016llx %016llx %016llx\n", g_nPresents,
            UTIL_GetMilliseconds() - REPLAY_CLOCK_START, gpGlobals != NULL ? gpGlobals->dwFrameNum : 0,
            (unsigned long long)REPLAY_StateHash(), (unsigned long long)screen, (unsigned long long)g_qwPaletteHash);

    // Save requested frames for inspection.
    for (i = 0; i < g_nDumps; i++)
    {
        if (g_rgDumps[i] == g_nPresents)
        {
            char path[1024];
            snprintf(path, sizeof(path), "%s.%u.png", g_pszTrace, g_nPresents);
            DumpFrame(g_rgDisplay, path);
        }
    }
}
