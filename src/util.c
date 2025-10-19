//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2024, SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 3
// as published by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "util.h"
#include "input.h"
#include "main.h"
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#if defined(_WIN32)
#include <windows.h>
#elif defined(ARDUINO_ARCH_ESP32)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

static int glSeed = 0; // Our random number generator's seed.

static void lsrand(unsigned int iInitialSeed)
/*++
  Purpose:

   This function initializes the random seed based on the initial seed value passed in the
   iInitialSeed parameter.

  Parameters:

   [IN]  iInitialSeed - The initial random seed.

  Return value:

   None.

--*/
{
    //
    // fill in the initial seed of the random number generator
    //
    glSeed = 1664525L * iInitialSeed + 1013904223L;
}

static int lrand(void)
/*++
  Purpose:

   This function is the equivalent of the rand() standard C library function, except that
   whereas rand() works only with short integers (i.e. not above 32767), this function is
   able to generate 32-bit random numbers.

  Parameters:

   None.

  Return value:

   The generated random number.

--*/
{
    if (glSeed == 0)                          // if the random seed isn't initialized...
        lsrand((unsigned int)time(NULL));     // initialize it first
    glSeed = 1664525L * glSeed + 1013904223L; // do some twisted math (infinite suite)
    return ((glSeed >> 1) + 1073741824L);     // and return the result.
}

int RandomLong(int from, int to)
/*++
  Purpose:

   This function returns a random integer number between (and including) the starting and
   ending values passed by parameters from and to.

  Parameters:

   from - the starting value.

   to - the ending value.

  Return value:

   The generated random number.

--*/
{
    if (to <= from)
        return from;

    return from + lrand() / (INT_MAX / (to - from + 1));
}

float RandomFloat(float from, float to)
/*++
  Purpose:

   This function returns a random floating-point number between (and including) the starting
   and ending values passed by parameters from and to.

  Parameters:

   from - the starting value.

   to - the ending value.

  Return value:

   The generated random number.

--*/
{
    if (to <= from)
        return from;

    return from + (float)lrand() / (INT_MAX / (to - from));
}

void TerminateOnError(
    const char *fmt,
    ...)
// This function is called when a fatal error or abnormal condition is detected.
// It formats and prints the error message to stderr, then immediately
// terminates the application with an error exit code.
{
    va_list argptr;
    char string[256];
    // concatenate all the arguments in one string
    va_start(argptr, fmt);
    vsnprintf(string, sizeof(string), fmt, argptr);
    va_end(argptr);
    fprintf(stderr, "\nFATAL ERROR: %s\n", string);
    PAL_Shutdown(255);
}

#ifdef _WIN32
int gettimeofday(struct timeval *tp, void *tzp)
{
    time_t clock;
    struct tm tm;
    SYSTEMTIME wtm;
    GetLocalTime(&wtm);
    tm.tm_year = wtm.wYear - 1900;
    tm.tm_mon = wtm.wMonth - 1;
    tm.tm_mday = wtm.wDay;
    tm.tm_hour = wtm.wHour;
    tm.tm_min = wtm.wMinute;
    tm.tm_sec = wtm.wSecond;
    tm.tm_isdst = -1;
    clock = mktime(&tm);
    tp->tv_sec = (long)clock;
    tp->tv_usec = wtm.wMilliseconds * 1000;
    return 0;
}
#endif

unsigned long UTIL_GetMilliseconds(void)
{
#ifdef ARDUINO_ARCH_ESP32
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (tv.tv_sec * 1000LL + (tv.tv_usec / 1000LL));
#endif
}

void UTIL_Sleep(unsigned int ms)
{
#if defined(ARDUINO_ARCH_ESP32)
    vTaskDelay(pdMS_TO_TICKS(ms));
#elif defined(_WIN32)
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

unsigned int UTIL_Delay(unsigned int ms)
{
    // Clear previous key states
    PAL_ClearKeyState();

    unsigned long end_time = UTIL_GetMilliseconds() + ms;
    while (UTIL_GetMilliseconds() < end_time)
    {
        UTIL_Sleep(1);
        PAL_ProcessEvent();
    }

    return PAL_GetKeyInput();
}

unsigned int UTIL_WaitKeys(unsigned int ms, unsigned int wait_keys)
{
    // If no specific keys are specified, wait for any key.
    if (wait_keys == 0)
        wait_keys = 0xFFFFFFFF;

    // Clear previous key states
    PAL_ClearKeyState();

    PALKEY pressed_key = kKeyNone;
    unsigned long end_time = UTIL_GetMilliseconds() + ms;
    while ((ms == 0 || UTIL_GetMilliseconds() < end_time) && (pressed_key == kKeyNone))
    {
        UTIL_Sleep(1);
        PAL_ProcessEvent();
        pressed_key |= (PAL_GetKeyInput() & wait_keys);
    }

    return pressed_key;
}
