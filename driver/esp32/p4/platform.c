// SDLPAL P4 lifecycle and SD compatibility; GPL-3.0-or-later.
#include "../backend_select.h"
#if defined(ARDUINO_ARCH_ESP32) && PAL_ESP32_BACKEND_P4
#include "platform.h"
#include "../../../src/driver.h"
#include "../../../src/pcmmus.h"
#include <esp_system.h>
#include <stdio.h>
#include <string.h>

void P4_PrepareShutdown(void)
{
    // Stop the audio worker before releasing resources it can still access.
    DRIVER_DeInit();
}

void P4_FinishShutdown(int exit_code)
{
    (void)exit_code;
    // Arduino setup() has no desktop main() setjmp context.
    esp_restart();
}

void *P4_OpenMusicFile(const char *path, int32_t track, uint32_t rate, uint16_t channels)
{
    void *fp = UTIL_fopen_without_checking(path, "rb");
    if (fp == NULL)
    {
        // Reuse existing SD data without moving the user's desktop PCM files.
        char fallback[256];
        snprintf(fallback, sizeof(fallback), "%s/mus_pcm/%d_%u.pcm", RESOURCE_PATH,
                 (int)track, (unsigned)rate);
        fp = UTIL_fopen_without_checking(fallback, "rb");
        if (fp != NULL)
        {
            uint8_t header[PCMMUS_HEADER_SIZE];
            uint32_t count = UTIL_fread(header, 1, sizeof(header), fp);
            uint32_t actual_rate = count == sizeof(header) ?
                ((uint32_t)header[8] | ((uint32_t)header[9] << 8) |
                 ((uint32_t)header[10] << 16) | ((uint32_t)header[11] << 24)) : 0;
            if (count != sizeof(header) || memcmp(header, PCMMUS_MAGIC, 4) != 0 ||
                header[4] != PCMMUS_VERSION || header[5] != 0 ||
                header[6] != channels || header[7] != 0 || actual_rate != rate)
            {
                UTIL_fclose(fp);
                return NULL;
            }
            UTIL_fseek(fp, 0, SEEK_SET);
        }
    }
    static int reported;
    if (fp != NULL && !reported)
    {
        printf("P4 music: pre-rendered PCM available\n");
        reported = 1;
    }
    return fp;
}
#endif
