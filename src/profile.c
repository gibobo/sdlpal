#include "profile.h"

#ifdef PAL_PROFILE
#include "util.h"
#include <stdio.h>
#include <string.h>
#if defined(_WIN32)
#include <malloc.h>
#include <windows.h>
#elif defined(ARDUINO_ARCH_ESP32)
#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <fcntl.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <unistd.h>
#else
#include <time.h>
#if defined(__GLIBC__)
#include <malloc.h>
#endif
#endif

// The boards print a report of each window of this length; the desktop prints
// only the totals at shutdown.
#ifndef PAL_PROFILE_INTERVAL_MS
#if defined(ARDUINO_ARCH_ESP32)
#define PAL_PROFILE_INTERVAL_MS 2000
#else
#define PAL_PROFILE_INTERVAL_MS 0
#endif
#endif

typedef struct
{
    uint32_t count;
    uint64_t total;
    uint32_t max;
} PROF_TIME_STAT;

typedef struct
{
    uint64_t counters[PROF_COUNTER_COUNT];
    PROF_TIME_STAT timers[PROF_TIMER_COUNT];
    uint64_t heap_peak;
} PROF_STATS;

static const char *const g_rgCounterNames[PROF_COUNTER_COUNT] = {
    "fseek", "fread", "fread_bytes", "fwrite", "fwrite_bytes", "chunk_read", "chunk_cache_hit",
    "decompress", "decompress_bytes", "malloc", "malloc_bytes", "late_wait", "glyph_read",
};
static const char *const g_rgTimerNames[PROF_TIMER_COUNT] = {
    "decompress_us", "scene_us", "battle_scene_us", "work_us", "load_res_us", "load_game_us",
    "save_game_us", "battle_load_us", "sound_us", "sound_lock_us", "music_us", "attack_us",
    "read_us",
};

static PROF_STATS g_Total, g_Window;
static uint64_t g_qwHeapLive = 0;
static uint32_t g_dwWindowStart = 0;
static uint32_t g_dwWaitEnd = 0;
static int g_fWaited = 0;

uint32_t PROF_Microseconds(void)
{
#if defined(_WIN32)
    static LARGE_INTEGER frequency;
    LARGE_INTEGER counter;
    if (frequency.QuadPart == 0)
        QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return (uint32_t)((counter.QuadPart / frequency.QuadPart) * 1000000 +
                      (counter.QuadPart % frequency.QuadPart) * 1000000 / frequency.QuadPart);
#elif defined(ARDUINO_ARCH_ESP32)
    return (uint32_t)esp_timer_get_time();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint32_t)((uint64_t)ts.tv_sec * 1000000u + (uint64_t)ts.tv_nsec / 1000u);
#endif
}

// The allocated size of a heap block, or 0 where the platform cannot tell. The
// boards report the free heap instead.
static uint64_t BlockSize(void *ptr)
{
#if defined(_WIN32)
    return _msize(ptr);
#elif defined(__GLIBC__)
    return malloc_usable_size(ptr);
#else
    (void)ptr;
    return 0;
#endif
}

#if defined(ARDUINO_ARCH_ESP32)
static TaskHandle_t g_hGameTask = NULL;
#endif

// Remember the game task; call it from the task that runs the game.
void PROF_Init(void)
{
#if defined(ARDUINO_ARCH_ESP32)
    g_hGameTask = xTaskGetCurrentTaskHandle();
#endif
}

// Whether the caller is the measured task. The desktop measures every thread;
// the replay tests run the game on one.
static int OnGameTask(void)
{
#if defined(ARDUINO_ARCH_ESP32)
    return g_hGameTask == NULL || xTaskGetCurrentTaskHandle() == g_hGameTask;
#else
    return 1;
#endif
}

void PROF_Count(PROF_COUNTER counter, uint32_t n)
{
    if (!OnGameTask())
        return;
    g_Total.counters[counter] += n;
    g_Window.counters[counter] += n;
}

static void AddTime(PROF_TIME_STAT *stat, uint32_t us)
{
    stat->count++;
    stat->total += us;
    if (us > stat->max)
        stat->max = us;
}

void PROF_AddTime(PROF_TIMER timer, uint32_t us)
{
    if (!OnGameTask())
        return;
    AddTime(&g_Total.timers[timer], us);
    AddTime(&g_Window.timers[timer], us);
}

void PROF_HeapAlloc(void *ptr)
{
    if (!OnGameTask())
        return;
    g_qwHeapLive += BlockSize(ptr);
    if (g_qwHeapLive > g_Total.heap_peak)
        g_Total.heap_peak = g_qwHeapLive;
    if (g_qwHeapLive > g_Window.heap_peak)
        g_Window.heap_peak = g_qwHeapLive;
}

void PROF_HeapFree(void *ptr)
{
    if (!OnGameTask())
        return;
    g_qwHeapLive -= BlockSize(ptr);
}

void PROF_WaitBegin(int late)
{
    // Time the work done since the previous wait ended.
    if (g_fWaited)
        PROF_AddTime(PROF_TIME_WORK, PROF_Microseconds() - g_dwWaitEnd);
    PROF_Count(PROF_LATE_WAIT, late ? 1 : 0);
    PROF_Poll();
}

void PROF_WaitEnd(void)
{
    g_dwWaitEnd = PROF_Microseconds();
    g_fWaited = 1;
}

// Print one PALPROF line: counters, timers as count/total/max microseconds, and
// the heap.
static void PrintStats(const char *label, const PROF_STATS *stats)
{
    int i;

    printf("PALPROF %s", label);
    for (i = 0; i < PROF_COUNTER_COUNT; i++)
        printf(" %s=%llu", g_rgCounterNames[i], (unsigned long long)stats->counters[i]);
    for (i = 0; i < PROF_TIMER_COUNT; i++)
        printf(" %s=%lu/%llu/%lu", g_rgTimerNames[i], (unsigned long)stats->timers[i].count,
               (unsigned long long)stats->timers[i].total, (unsigned long)stats->timers[i].max);
    if (stats->heap_peak != 0)
        printf(" heap_peak=%llu", (unsigned long long)stats->heap_peak);
#if defined(ARDUINO_ARCH_ESP32)
    printf(" free_internal=%lu free_spiram=%lu largest_spiram=%lu min_free=%lu",
           (unsigned long)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
           (unsigned long)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
           (unsigned long)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM),
           (unsigned long)heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT));
#endif
    printf("\n");
    fflush(stdout);
}

void PROF_Report(const char *label)
{
    PrintStats(label, &g_Total);
}

void PROF_Poll(void)
{
#if PAL_PROFILE_INTERVAL_MS > 0
    uint32_t now = UTIL_GetMilliseconds();

    if (now - g_dwWindowStart >= PAL_PROFILE_INTERVAL_MS)
    {
        PrintStats("window", &g_Window);
        memset(&g_Window, 0, sizeof(g_Window));
        g_Window.heap_peak = g_qwHeapLive;
        g_dwWindowStart = now;
    }
#endif
}

#if defined(ARDUINO_ARCH_ESP32)
#define PROF_READ_WAYS 6
#else
#define PROF_READ_WAYS 2
#endif
// The size of the stream buffers and staging buffer the benchmark tries.
#define PROF_READ_STAGE 32768

// Read size bytes at offset from path one way; returns the bytes read.
static uint32_t ReadOneWay(int way, const char *path, long offset, uint8_t *buffer, uint32_t size)
{
    uint32_t got = 0;

    if (way < 3)
    {
        // Streams: the default buffer, a 32 KB buffer, or a 32 KB buffer in DMA memory.
        FILE *fp = fopen(path, "rb");
#if defined(ARDUINO_ARCH_ESP32)
        void *own = NULL;
#endif
        if (fp == NULL)
            return 0;
        if (way == 1)
            setvbuf(fp, NULL, _IOFBF, PROF_READ_STAGE);
#if defined(ARDUINO_ARCH_ESP32)
        else if (way == 2 && (own = heap_caps_aligned_alloc(128, PROF_READ_STAGE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL)) != NULL)
            setvbuf(fp, (char *)own, _IOFBF, PROF_READ_STAGE);
#endif
        fseek(fp, offset, SEEK_SET);
        got = (uint32_t)fread(buffer, 1, size, fp);
        fclose(fp);
#if defined(ARDUINO_ARCH_ESP32)
        heap_caps_free(own);
#endif
    }
#if defined(ARDUINO_ARCH_ESP32)
    else
    {
        // Descriptors: straight into PSRAM, or in 32 KB pieces through DMA memory.
        int fd = open(path, O_RDONLY);
        uint8_t *stage = (way == 4) ? (uint8_t *)heap_caps_aligned_alloc(128, PROF_READ_STAGE, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL) : NULL;
        if (fd >= 0 && lseek(fd, offset, SEEK_SET) == offset && (way != 4 || stage != NULL))
        {
            while (got < size)
            {
                uint32_t want = (way == 4 && size - got > PROF_READ_STAGE) ? PROF_READ_STAGE : size - got;
                int n = (int)read(fd, way == 4 ? stage : buffer + got, want);
                if (n <= 0)
                    break;
                if (way == 4)
                    memcpy(buffer + got, stage, (size_t)n);
                got += (uint32_t)n;
            }
        }
        if (fd >= 0)
            close(fd);
        heap_caps_free(stage);
    }
#endif
    return got;
}

/*++
  Read 512 KB of gop.mkf in each way below and print bytes/us for each, to find
  how the boards read the SD card fastest. Each way reads its own region, so
  none profits from data another left in a cache.

    default     a stream with its default buffer, from an odd offset
    buffer32k   a stream with a 32 KB buffer, from an odd offset
    dmabuf32k   a stream with a 32 KB buffer in DMA memory, from an odd offset
    fd_psram    one read() straight into PSRAM, from an odd offset
    fd_stage    read() in 32 KB pieces through DMA memory, from a sector boundary
    fd_aligned  one read() into 128-byte aligned PSRAM, from a sector boundary
    util        UTIL_fread, the way the game reads, from an odd offset
--*/
void PROF_ReadBenchmark(void)
{
    static const char *const names[] = {"default", "buffer32k", "dmabuf32k", "fd_psram", "fd_stage", "fd_aligned"};
    const uint32_t size = 512 * 1024;
    const char *path = UTIL_Filename("%s/gop.mkf", RESOURCE_PATH);
    uint8_t *buffer = (uint8_t *)UTIL_TryMalloc(size);
    int i;

    if (buffer == NULL)
        return;
    printf("PALPROF readbench");
    for (i = 0; i < PROF_READ_WAYS; i++)
    {
        long offset = 1000003L + (long)i * 1048576L;
        uint8_t *target = buffer;
        uint32_t start, got;

#if defined(ARDUINO_ARCH_ESP32)
        // The last two ways start on a sector boundary; the last reads into aligned PSRAM.
        if (i >= 4)
            offset &= ~511L;
        if (i == 5 && (target = (uint8_t *)heap_caps_aligned_alloc(128, size, MALLOC_CAP_SPIRAM)) == NULL)
            break;
#endif
        start = PROF_Microseconds();
        got = ReadOneWay(i, path, offset, target, size);
        printf(" %s=%lu/%lu", names[i], (unsigned long)got, (unsigned long)(PROF_Microseconds() - start));
        fflush(stdout);
#if defined(ARDUINO_ARCH_ESP32)
        if (target != buffer)
            heap_caps_free(target);
#endif
    }

    // Read the way the game does, through UTIL_fread from an odd offset.
    {
        FILE *fp = (FILE *)UTIL_fopen_without_checking(path, "rb");
        uint32_t start, got;
        if (fp != NULL)
        {
            fseek(fp, 1000003L + (long)PROF_READ_WAYS * 1048576L, SEEK_SET);
            start = PROF_Microseconds();
            got = UTIL_fread(buffer, 1, size, fp);
            printf(" util=%lu/%lu", (unsigned long)got, (unsigned long)(PROF_Microseconds() - start));
            fclose(fp);
        }
    }
    printf("\n");
    fflush(stdout);
    UTIL_free(buffer);
}

#endif
