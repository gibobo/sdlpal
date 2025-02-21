#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *UTIL_malloc(unsigned int buffer_size) {
   // handy wrapper for operations we always forget, like checking malloc's returned pointer.
   void *buffer = NULL;

   // first off, check if buffer size is valid
   if (buffer_size == 0)
      TerminateOnError("UTIL_malloc() called with invalid buffer size: %d\n", buffer_size);
   else
      buffer = malloc(buffer_size); // allocate real memory space

   // last check, check if malloc call succeeded
   if (buffer == NULL)
      TerminateOnError("UTIL_malloc() failure for %d bytes (out of memory?)\n", buffer_size);
   else
      memset(buffer, 0, buffer_size);

   return buffer; // nothing went wrong, so return buffer pointer
}

void *UTIL_calloc(unsigned int n, unsigned int size) {
   // handy wrapper for operations we always forget, like checking calloc's returned pointer.
   void *buffer = NULL;

   // first off, check if buffer size is valid
   if (n == 0 || size == 0)
      TerminateOnError("UTIL_calloc() called with invalid parameters\n");
   else
      buffer = calloc(n, size); // allocate real memory space

   // last check, check if malloc call succeeded
   if (buffer == NULL)
      TerminateOnError("UTIL_calloc() failure for %d bytes (out of memory?)\n", size * n);
   else
      memset(buffer, 0, size * n);

   return buffer; // nothing went wrong, so return buffer pointer
}

void *UTIL_realloc(void *ptr, unsigned int n, unsigned int size) {
   // handy wrapper for operations we always forget, like checking calloc's returned pointer.
   void *buffer = NULL;

   // first off, check if buffer size is valid
   if (n == 0 || size == 0)
      TerminateOnError("%s() called with invalid parameters\n", __func__);
   else
      buffer = realloc(ptr, n * size); // allocate real memory space

   // last check, check if malloc call succeeded
   if (buffer == NULL)
      TerminateOnError("%s() failure for %d bytes (out of memory?)\n", __func__, size * n);

   return buffer; // nothing went wrong, so return buffer pointer
}

void UTIL_free(void *ptr) {
   if (ptr)
      free(ptr);
}
