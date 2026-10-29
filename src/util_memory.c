#include "util.h"
#include "profile.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ARDUINO_ARCH_ESP32
#include "esp_heap_caps.h"
#include "esp_system.h"
#if __has_include("esp_memory_utils.h")
#include "esp_memory_utils.h"
#else
#include "soc/soc_memory_types.h"
#endif

// Define to enable/disable memory usage reporting
#define ENABLE_MEMORY_REPORT 0

// Function to report memory usage on ESP32
static void report_memory_usage(const char *operation, size_t allocated_size)
{
#if ENABLE_MEMORY_REPORT
    size_t free_internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    size_t free_spiram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t total_free = esp_get_free_heap_size();
    printf("Memory %s %zu bytes. Remaining - Internal: %zu bytes, SPIRAM: %zu bytes, Total: %zu bytes\n",
           operation, allocated_size, free_internal, free_spiram, total_free);
#endif
}
#endif

// Called when an allocation fails: frees memory that can be rebuilt, such as
// caches, and returns the number of bytes freed.
static uint32_t (*g_pfnReleaseMemory)(void) = NULL;

void UTIL_SetMemoryReleaseHandler(uint32_t (*handler)(void))
{
    g_pfnReleaseMemory = handler;
}

// Allocate n * size bytes, zeroed if asked, from PSRAM where there is one and
// otherwise from the general heap. Returns NULL on failure.
static void *TryAllocate(uint32_t n, uint32_t size, int zeroed)
{
    void *buffer = NULL;

#ifdef ARDUINO_ARCH_ESP32
    // buffer = heap_caps_malloc(n * size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    // if (buffer == NULL)
    buffer = zeroed ? heap_caps_calloc(n, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)
                    : heap_caps_malloc(n * size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (buffer != NULL)
    {
        report_memory_usage("allocated", n * size);
    }
#endif

    // if the PSRAM allocation fails, fall back to the general heap
    if (buffer == NULL)
        buffer = zeroed ? calloc(n, size) : malloc(n * size);

    return buffer;
}

// Release rebuildable memory after a failed allocation; true if anything was freed.
static int ReleaseMemory(void)
{
    return g_pfnReleaseMemory != NULL && g_pfnReleaseMemory() > 0;
}

int UTIL_IsPlentiful(const void *ptr)
{
#ifdef ARDUINO_ARCH_ESP32
    return esp_ptr_external_ram(ptr);
#else
    (void)ptr;
    return 1;
#endif
}

void *UTIL_TryMalloc(uint32_t buffer_size)
{
    void *buffer = NULL;

    if (buffer_size == 0)
        return NULL;
#ifdef ARDUINO_ARCH_ESP32
    // What only saves work never takes the scarce internal RAM.
    buffer = heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#else
    buffer = malloc(buffer_size);
#endif

    if (buffer != NULL)
    {
        PROF_COUNT(PROF_MALLOC, 1);
        PROF_COUNT(PROF_MALLOC_BYTES, buffer_size);
        PROF_HEAP_ALLOC(buffer);
    }
    return buffer;
}

void *UTIL_malloc(uint32_t buffer_size)
{
    // handy wrapper for operations we always forget, like checking malloc's returned pointer.
    void *buffer = NULL;

    // first off, check if buffer size is valid
    if (buffer_size == 0)
        TerminateOnError("%s() failed: invalid buffer size (cannot allocate 0 bytes)\n", __func__);

    buffer = TryAllocate(1, buffer_size, 0);
    if (buffer == NULL && ReleaseMemory())
        buffer = TryAllocate(1, buffer_size, 0);

    // last check, check if malloc call succeeded
    if (buffer == NULL)
        TerminateOnError("%s() failed: memory allocation failed for %u bytes (insufficient memory)\n", __func__, buffer_size);

    memset(buffer, 0, buffer_size);
    PROF_COUNT(PROF_MALLOC, 1);
    PROF_COUNT(PROF_MALLOC_BYTES, buffer_size);
    PROF_HEAP_ALLOC(buffer);

    return buffer; // nothing went wrong, so return buffer pointer
}

void *UTIL_calloc(uint32_t n, uint32_t size)
{
    // handy wrapper for operations we always forget, like checking calloc's returned pointer.
    void *buffer = NULL;

    // first off, check if buffer size is valid
    if (n == 0 || size == 0)
        TerminateOnError("%s() failed: invalid parameters (n=%u, size=%u - cannot allocate zero elements or zero-sized elements)\n", __func__, n, size);

    // Check for potential overflow
    if (n > UINT_MAX / size)
        TerminateOnError("%s() failed: arithmetic overflow detected (n=%u, size=%u - total size exceeds maximum value)\n", __func__, n, size);

    buffer = TryAllocate(n, size, 1);
    if (buffer == NULL && ReleaseMemory())
        buffer = TryAllocate(n, size, 1);

    // last check, check if calloc call succeeded
    if (buffer == NULL)
        TerminateOnError("%s() failed: memory allocation failed for %u bytes (insufficient memory)\n", __func__, size * n);

    // calloc and heap_caps_calloc return zeroed memory.
    PROF_COUNT(PROF_MALLOC, 1);
    PROF_COUNT(PROF_MALLOC_BYTES, size * n);
    PROF_HEAP_ALLOC(buffer);

    return buffer; // nothing went wrong, so return buffer pointer
}

// Reallocate from PSRAM where there is one, otherwise from the general heap.
static void *TryReallocate(void *ptr, uint32_t total)
{
    void *buffer = NULL;

#ifdef ARDUINO_ARCH_ESP32
    // buffer = heap_caps_realloc(ptr, total, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    // if (buffer == NULL)
    buffer = heap_caps_realloc(ptr, total, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (buffer != NULL)
    {
        report_memory_usage("reallocated", total);
    }
#endif
    // if realloc fails, the original block is left untouched; so only assign if successful
    if (buffer == NULL)
        buffer = realloc(ptr, total);

    return buffer;
}

void *UTIL_realloc(void *ptr, uint32_t n, uint32_t size)
{
    // handy wrapper for operations we always forget, like checking realloc's returned pointer.
    void *buffer = NULL;

    // first off, check if buffer size is valid
    if (n == 0 || size == 0)
        TerminateOnError("%s() failed: invalid parameters (n=%u, size=%u - cannot reallocate to zero size)\n", __func__, n, size);

    if (ptr != NULL)
        PROF_HEAP_FREE(ptr);

    buffer = TryReallocate(ptr, n * size);
    if (buffer == NULL && ReleaseMemory())
        buffer = TryReallocate(ptr, n * size);

    // last check, check if realloc call succeeded
    if (buffer == NULL)
        TerminateOnError("%s() failed: memory reallocation failed for %u bytes (insufficient memory)\n", __func__, size * n);

    PROF_COUNT(PROF_MALLOC, 1);
    PROF_COUNT(PROF_MALLOC_BYTES, size * n);
    PROF_HEAP_ALLOC(buffer);
    return buffer; // nothing went wrong, so return buffer pointer
}

void UTIL_free(void *ptr)
{
    if (ptr)
    {
        PROF_HEAP_FREE(ptr);
#ifdef ARDUINO_ARCH_ESP32
        heap_caps_free(ptr);
#else
        free(ptr);
#endif
    }
}
