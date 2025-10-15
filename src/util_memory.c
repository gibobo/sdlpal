#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ARDUINO_ARCH_ESP32
#include "esp_heap_caps.h"
#endif

void *UTIL_malloc(unsigned int buffer_size)
{
    // handy wrapper for operations we always forget, like checking malloc's returned pointer.
    void *buffer = NULL;

    // first off, check if buffer size is valid
    if (buffer_size == 0)
        TerminateOnError("%s() called with invalid buffer size: %d\n", __func__, buffer_size);

#ifdef ARDUINO_ARCH_ESP32
    buffer = heap_caps_malloc(buffer_size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (buffer == NULL)
        buffer = heap_caps_malloc(buffer_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#endif

    // if malloc fails, the original block is left untouched; so only assign if successful
    if (buffer == NULL)
        buffer = malloc(buffer_size);

    // last check, check if malloc call succeeded
    if (buffer == NULL)
        TerminateOnError("%s() failure for %d bytes (out of memory?)\n", __func__, buffer_size);

    memset(buffer, 0, buffer_size);

    return buffer; // nothing went wrong, so return buffer pointer
}

void *UTIL_calloc(unsigned int n, unsigned int size)
{
    // handy wrapper for operations we always forget, like checking calloc's returned pointer.
    void *buffer = NULL;

    // first off, check if buffer size is valid
    if (n == 0 || size == 0)
        TerminateOnError("%s() called with invalid parameters\n", __func__);

#ifdef ARDUINO_ARCH_ESP32
    buffer = heap_caps_calloc(n, size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (buffer == NULL)
        buffer = heap_caps_calloc(n, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#endif

    // if calloc fails, the original block is left untouched; so only assign if successful
    if (buffer == NULL)
        buffer = calloc(n, size);

    // last check, check if calloc call succeeded
    if (buffer == NULL)
        TerminateOnError("%s() failure for %d bytes (out of memory?)\n", __func__, size * n);

    memset(buffer, 0, size * n);

    return buffer; // nothing went wrong, so return buffer pointer
}

void *UTIL_realloc(void *ptr, unsigned int n, unsigned int size)
{
    // handy wrapper for operations we always forget, like checking realloc's returned pointer.
    void *buffer = NULL;

    // first off, check if buffer size is valid
    if (n == 0 || size == 0)
        TerminateOnError("%s() called with invalid parameters\n", __func__);

#ifdef ARDUINO_ARCH_ESP32
    buffer = heap_caps_realloc(ptr, n * size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (buffer == NULL)
        buffer = heap_caps_realloc(ptr, n * size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#endif
    // if realloc fails, the original block is left untouched; so only assign if successful
    if (buffer == NULL)
        buffer = realloc(ptr, n * size);

    // last check, check if realloc call succeeded
    if (buffer == NULL)
        TerminateOnError("%s() failure for %d bytes (out of memory?)\n", __func__, size * n);

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
