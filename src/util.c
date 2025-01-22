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
#include "global.h"
#include "input/input.h"
#include "main.h"
#include <errno.h>
#ifdef _WIN32
#include <io.h>
#include <windows.h>
#define access _access
#else
#include <sys/time.h>
#include <unistd.h>
#endif

long flength(FILE *fp) {
	long old_pos = ftell(fp);
	if (old_pos == -1)
		return -1;
	if (fseek(fp, 0, SEEK_END) == -1)
		return -1;
	long length = ftell(fp);
	fseek(fp, old_pos, SEEK_SET);
	return length;
}

/*
 * RNG code based on RACC by Pierre-Marie Baty.
 * http://racc.bots-united.com
 *
 * Copyright (c) 2004, Pierre-Marie Baty
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in
 * the documentation and/or other materials provided with the
 * distribution.
 *
 * Neither the name of the RACC nor the names of its contributors
 * may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 * CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

// Our random number generator's seed.
static int glSeed = 0;

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

int RandomLong(
	int from,
	int to)
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

float RandomFloat(
	float from,
	float to)
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

void UTIL_Delay(
	unsigned int ms)
{
	PAL_DelayUntil(UTIL_GetTicks() + ms);
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

void *
UTIL_malloc(
	size_t buffer_size)
{
	// handy wrapper for operations we always forget, like checking malloc's returned pointer.

	void *buffer;

	// first off, check if buffer size is valid
	if (buffer_size == 0)
		TerminateOnError("UTIL_malloc() called with invalid buffer size: %d\n", buffer_size);

	buffer = malloc(buffer_size); // allocate real memory space
	memset(buffer, 0, buffer_size);

	// last check, check if malloc call succeeded
	if (buffer == NULL)
		TerminateOnError("UTIL_malloc() failure for %d bytes (out of memory?)\n", buffer_size);

	return buffer; // nothing went wrong, so return buffer pointer
}

void *
UTIL_calloc(
	size_t n,
	size_t size)
{
	// handy wrapper for operations we always forget, like checking calloc's returned pointer.

	void *buffer;

	// first off, check if buffer size is valid
	if (n == 0 || size == 0)
		TerminateOnError("UTIL_calloc() called with invalid parameters\n");

	buffer = calloc(n, size); // allocate real memory space
	memset(buffer, 0, size * n);

	// last check, check if malloc call succeeded
	if (buffer == NULL)
		TerminateOnError("UTIL_calloc() failure for %d bytes (out of memory?)\n", size * n);

	return buffer; // nothing went wrong, so return buffer pointer
}

void UTIL_CloseFile(
	FILE *fp)
/*++
  Purpose:

	Close a file.

  Parameters:

	[IN]  fp - file handle to be closed.

  Return value:

	None.

--*/
{
	if (fp != NULL)
	{
		fclose(fp);
	}
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

void PAL_DelayUntil(unsigned int tm) {
	do 	{
		PAL_ProcessEvent();
	} while (tm > UTIL_GetTicks());
}
