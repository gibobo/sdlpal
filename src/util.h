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

#ifndef UTIL_H
#define UTIL_H

#ifndef RESOURCE_PATH
#define RESOURCE_PATH "."
#endif

#ifndef CACHES_PATH
#define CACHES_PATH "."
#endif

#ifndef SEEK_SET
#define SEEK_SET 0
#endif
#ifndef SEEK_CUR
#define SEEK_CUR 1
#endif
#ifndef SEEK_END
#define SEEK_END 2
#endif

#define FRAME_TIME (1000 / 10) // 10 FPS

#ifndef max
#define max(a, b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

// status of characters
typedef enum tagPALRES
{
    Res_ABC = 0,    // enemy sprites in battle
    Res_FBP,        // battlefield background images
    Res_MGO,        // sprites in scenes
    Res_BALL,       // item bitmaps
    Res_DATA,       // misc data
    Res_F,          // player sprites during battle
    Res_FIRE,       // fire effect sprites
    Res_RGM,        // character face bitmaps
    Res_SSS,        // script data
    Res_SOUNDS,     // sound data
    Res_PAT,        // palette data
    Res_MAP,        // map data
    Res_GOP,        // map objects
    Res_MUS,        // music data
    Res_RNG,        // RNG animation data
    Save_1,         // save slot 1
    Save_2,         // save slot 2
    Save_3,         // save slot 3
    Save_4,         // save slot 4
    Save_5,         // save slot 5
    Cache_Word_2B,  // word cache 2 Bytes
    Cache_Word_4B,  // word cache 4 Bytes
    Cache_WordLen,  // word length cache
    Cache_Msg_2B,   // message cache 2 Bytes
    Cache_Msg_4B,   // message cache 4 Bytes
    Cache_MsgLen,   // message length cache
    Cache_Font,     // font cache
    Cache_FontSize, // font size cache
} PALRES;

long flength(void *fp);

int RandomLong(int from, int to);

float RandomFloat(float from, float to);

void TerminateOnError(const char *fmt, ...);

void *UTIL_malloc(unsigned int buffer_size);

void *UTIL_calloc(unsigned int n, unsigned int size);

void *UTIL_realloc(void *ptr, unsigned int n, unsigned int size);

void UTIL_free(void *ptr);

// Platform-specific utilities
unsigned long UTIL_GetTicks(void);

void *UTIL_fopen(const char *_FileName, const char *_Mode);

void *UTIL_fopen_without_checking(const char *_FileName, const char *_Mode);

int UTIL_fseek(void *_Stream, long _Offset, int _Origin);

unsigned int UTIL_fread(void *_Buffer, unsigned int _ElementSize, unsigned int _ElementCount, void *_Stream);

unsigned int UTIL_fwrite(void *_Buffer, unsigned int _ElementSize, unsigned int _ElementCount, void *_Stream);

void UTIL_fclose(void *fp);

void *UTIL_Open(const PALRES res, const char *_Mode);

void *UTIL_Open_without_checking(const PALRES res, const char *_Mode);

void UTIL_Close(const PALRES res);

#ifdef __cplusplus
extern "C" {
#endif

void UTIL_Delay(int ms);

#ifdef __cplusplus
}
#endif

#endif
