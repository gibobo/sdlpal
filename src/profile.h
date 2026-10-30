#ifndef PROFILE_H
#define PROFILE_H

#include <stdint.h>

// Measurement hooks for performance and memory work. Without PAL_PROFILE every
// hook compiles to nothing. With it, the counters count calls and bytes, which
// are deterministic in the replay tests, and the timers measure real time in
// microseconds. Results are printed as "PALPROF key=value" lines. On the boards
// only the game task is measured: the audio task streams music through the same
// file functions, and the statistics are not safe to update from two cores.

typedef enum
{
    PROF_FSEEK,            // UTIL_fseek calls
    PROF_FREAD,            // UTIL_fread calls
    PROF_FREAD_BYTES,      // bytes requested from UTIL_fread
    PROF_FWRITE,           // UTIL_fwrite calls
    PROF_FWRITE_BYTES,     // bytes passed to UTIL_fwrite
    PROF_CHUNK_READ,       // MKF chunks read
    PROF_CHUNK_CACHE_HIT,  // chunk reads served from the chunk cache
    PROF_DECOMPRESS,       // YJ2 decompressions
    PROF_DECOMPRESS_BYTES, // bytes produced by YJ2 decompression
    PROF_MALLOC,           // UTIL_malloc, UTIL_calloc and UTIL_realloc calls
    PROF_MALLOC_BYTES,     // bytes requested from them
    PROF_LATE_WAIT,        // waits that started after their deadline
    PROF_GLYPH_READ,       // glyphs read from the font file (font cache misses)
    PROF_COUNTER_COUNT
} PROF_COUNTER;

typedef enum
{
    PROF_TIME_DECOMPRESS,   // YJ2_Decompress
    PROF_TIME_SCENE,        // PAL_MakeScene
    PROF_TIME_BATTLE_SCENE, // PAL_BattleMakeScene
    PROF_TIME_WORK,         // work between the end of one wait and the start of the next
    PROF_TIME_LOAD_RES,     // PAL_LoadResources when something is loaded
    PROF_TIME_LOAD_GAME,    // reading a saved game
    PROF_TIME_SAVE_GAME,    // writing a saved game
    PROF_TIME_BATTLE_LOAD,  // loading the battle sprites and background
    PROF_TIME_SOUND,        // AUDIO_PlaySound, including the wait for the audio lock
    PROF_TIME_SOUND_LOCK,   // the part of AUDIO_PlaySound spent waiting for the audio lock
    PROF_TIME_MUSIC,        // AUDIO_PlayMusic
    PROF_TIME_ATTACK,       // a player's normal attack animation (nine battle frames)
    PROF_TIME_READ,         // UTIL_fread
    PROF_TIMER_COUNT
} PROF_TIMER;

#ifdef PAL_PROFILE

void PROF_Init(void);
uint32_t PROF_Microseconds(void);
void PROF_Count(PROF_COUNTER counter, uint32_t n);
void PROF_AddTime(PROF_TIMER timer, uint32_t us);
void PROF_HeapAlloc(void *ptr);
void PROF_HeapFree(void *ptr);
void PROF_WaitBegin(int late);
void PROF_WaitEnd(void);
void PROF_Report(const char *label);
void PROF_Poll(void);
void PROF_ReadBenchmark(void);

#define PROF_INIT() PROF_Init()
#define PROF_COUNT(counter, n) PROF_Count((counter), (n))
#define PROF_BEGIN(name) uint32_t name##_prof_start = PROF_Microseconds()
#define PROF_END(name, timer) PROF_AddTime((timer), PROF_Microseconds() - name##_prof_start)
#define PROF_HEAP_ALLOC(ptr) PROF_HeapAlloc(ptr)
#define PROF_HEAP_FREE(ptr) PROF_HeapFree(ptr)
#define PROF_WAIT_BEGIN(late) PROF_WaitBegin(late)
#define PROF_WAIT_END() PROF_WaitEnd()
#define PROF_REPORT(label) PROF_Report(label)
#define PROF_READ_BENCHMARK() PROF_ReadBenchmark()

#else

#define PROF_INIT() ((void)0)
#define PROF_COUNT(counter, n) ((void)0)
#define PROF_BEGIN(name) ((void)0)
#define PROF_END(name, timer) ((void)0)
#define PROF_HEAP_ALLOC(ptr) ((void)0)
#define PROF_HEAP_FREE(ptr) ((void)0)
#define PROF_WAIT_BEGIN(late) ((void)0)
#define PROF_WAIT_END() ((void)0)
#define PROF_REPORT(label) ((void)0)
#define PROF_READ_BENCHMARK() ((void)0)

#endif

#endif
