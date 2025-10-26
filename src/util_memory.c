#include "util.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ARDUINO_ARCH_ESP32
#include "esp_heap_caps.h"
#include "esp_system.h"

// Define to enable/disable memory usage reporting
#define ENABLE_MEMORY_REPORT 1

// Function to report memory usage on ESP32
static void report_memory_usage(const char* operation, size_t allocated_size)
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

void *UTIL_malloc(unsigned int buffer_size)
{
    // handy wrapper for operations we always forget, like checking malloc's returned pointer.
    void *buffer = NULL;

    // first off, check if buffer size is valid
    if (buffer_size == 0)
        TerminateOnError("%s() failed: invalid buffer size (cannot allocate 0 bytes)\n", __func__);

#ifdef ARDUINO_ARCH_ESP32
    // buffer = heap_caps_malloc(buffer_size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    // if (buffer == NULL)
    buffer = heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    
    if (buffer != NULL) {
        report_memory_usage("allocated", buffer_size);
    }
#endif

    // if malloc fails, the original block is left untouched; so only assign if successful
    if (buffer == NULL)
        buffer = malloc(buffer_size);

    // last check, check if malloc call succeeded
    if (buffer == NULL)
        TerminateOnError("%s() failed: memory allocation failed for %u bytes (insufficient memory)\n", __func__, buffer_size);

    memset(buffer, 0, buffer_size);

    return buffer; // nothing went wrong, so return buffer pointer
}

void *UTIL_calloc(unsigned int n, unsigned int size)
{
    // handy wrapper for operations we always forget, like checking calloc's returned pointer.
    void *buffer = NULL;

    // first off, check if buffer size is valid
    if (n == 0 || size == 0)
        TerminateOnError("%s() failed: invalid parameters (n=%u, size=%u - cannot allocate zero elements or zero-sized elements)\n", __func__, n, size);

    // Check for potential overflow
    if (n > UINT_MAX / size)
        TerminateOnError("%s() failed: arithmetic overflow detected (n=%u, size=%u - total size exceeds maximum value)\n", __func__, n, size);

#ifdef ARDUINO_ARCH_ESP32
    // buffer = heap_caps_calloc(n, size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    // if (buffer == NULL)
    buffer = heap_caps_calloc(n, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    
    if (buffer != NULL) {
        size_t allocated_size = n * size;
        report_memory_usage("allocated", allocated_size);
    }
#endif

    // if calloc fails, the original block is left untouched; so only assign if successful
    if (buffer == NULL)
        buffer = calloc(n, size);

    // last check, check if calloc call succeeded
    if (buffer == NULL)
        TerminateOnError("%s() failed: memory allocation failed for %u bytes (insufficient memory)\n", __func__, size * n);

    memset(buffer, 0, size * n);

    return buffer; // nothing went wrong, so return buffer pointer
}

void *UTIL_realloc(void *ptr, unsigned int n, unsigned int size)
{
    // handy wrapper for operations we always forget, like checking realloc's returned pointer.
    void *buffer = NULL;

    // first off, check if buffer size is valid
    if (n == 0 || size == 0)
        TerminateOnError("%s() failed: invalid parameters (n=%u, size=%u - cannot reallocate to zero size)\n", __func__, n, size);

#ifdef ARDUINO_ARCH_ESP32
    // buffer = heap_caps_realloc(ptr, n * size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    // if (buffer == NULL)
    buffer = heap_caps_realloc(ptr, n * size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    
    if (buffer != NULL) {
        size_t allocated_size = n * size;
        report_memory_usage("reallocated", allocated_size);
    }
#endif
    // if realloc fails, the original block is left untouched; so only assign if successful
    if (buffer == NULL)
        buffer = realloc(ptr, n * size);

    // last check, check if realloc call succeeded
    if (buffer == NULL)
        TerminateOnError("%s() failed: memory reallocation failed for %u bytes (insufficient memory)\n", __func__, size * n);

    return buffer; // nothing went wrong, so return buffer pointer
}

void UTIL_free(void *ptr)
{
    if (ptr)
    {
#ifdef ARDUINO_ARCH_ESP32
        heap_caps_free(ptr);
#else
        free(ptr);
#endif
    }
}
