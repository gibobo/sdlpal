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
#include "common.h"
#include "input.h"
#include "main.h"
#include <errno.h>
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#include <unistd.h>
#endif

static int glSeed = 0;	// Our random number generator's seed.

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
	if (glSeed == 0)						  // if the random seed isn't initialized...
		lsrand((unsigned int)time(NULL));	  // initialize it first
	glSeed = 1664525L * glSeed + 1013904223L; // do some twisted math (infinite suite)
	return ((glSeed >> 1) + 1073741824L);	  // and return the result.
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
// This function terminates the game because of an error and
// prints the message string pointed to by fmt both in the
// console and in a messagebox.
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

unsigned int UTIL_GetTicks(void) {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000LL + (tv.tv_usec / 1000LL));
}

void UTIL_Sleep(unsigned int tm) {
#ifdef _WIN32
	Sleep(tm);
#else
	usleep(tm * 1000);
#endif
}

void UTIL_Delay(unsigned int ms) {
	unsigned int tm = UTIL_GetTicks() + ms;
	while (tm > UTIL_GetTicks()) {
		PAL_ProcessEvent();
	}
}
