#ifndef CORE_DIFF_MASTER_TYPES_H
#define CORE_DIFF_MASTER_TYPES_H

// Upstream Windows-style scalar types, usable next to the development headers.

#include <wchar.h>

typedef char CHAR;
typedef wchar_t WCHAR;
typedef short SHORT;
typedef long LONG;
typedef unsigned short WORD, *LPWORD;
typedef unsigned int DWORD, *LPDWORD;
typedef int INT, *LPINT;
typedef int BOOL, *LPBOOL;
typedef unsigned int UINT, *PUINT, UINT32, *PUINT32;
typedef unsigned char BYTE, *LPBYTE;
typedef const BYTE *LPCBYTE;
typedef float FLOAT, *LPFLOAT;
typedef void *LPVOID;
typedef const void *LPCVOID;

#endif
