/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
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
// win32.cpp: WIN32-specific codes.
//   @Author: Lou Yihua <louyihua@21cn.com>, 2016.
//

// #define UNICODE
// #define _UNICODE

// #include <tchar.h>
// #include <windows.h>
// #include <commctrl.h>
// #include <shlobj.h>
// #include <errno.h>
// #include <string>
// #include "resource.h"
// #include "global.h"
#include "util.h"
#include "palcfg.h"
// #include "resampler.h"

extern "C" void UTIL_Platform_Quit(void) {}

extern "C" 
BOOL UTIL_IsAbsolutePath(const char *lpszFileName) 
{
	char szDrive[_MAX_DRIVE];
	char szDir[_MAX_DIR];
	char szFname[_MAX_FNAME];
	char szExt[_MAX_EXT];
	if (lpszFileName == NULL)
		return FALSE;
#if !defined(__MINGW32__) // MinGW Distro's win32 api lacks this...Anyway, winxp lacks this too
	if (_splitpath_s(lpszFileName, szDrive, szDir, szFname, szExt) == 0)
#else
	_splitpath(lpszFileName, szDrive, szDir, szFname, szExt);
	if ( errno !=EINVAL )
#endif
		return (strlen(szDrive) > 0 && (szDir[0] == '\\' || szDir[0] == '/'));
	else
		return FALSE;
}
