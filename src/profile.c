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
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
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
    "decompress", "decompress_bytes", "malloc", "malloc_bytes", "late_wait",
};
static const char *const g_rgTimerNames[PROF_TIMER_COUNT] = {
    "decompress_us", "scene_us", "battle_scene_us", "work_us", "load_res_us", "load_game_us",
    "save_game_us", "battle_load_us", "sound_us", "sound_lock_us", "music_us", "attack_us",
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

#endif
