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
// Portions based on PALx Project by palxex.
// Copyright (c) 2006-2008, Pal Lockheart <palxex@gmail.com>.
//

#include "text.h"
#include "common.h"
#include "font.h"
#include "global.h"
#include "input.h"
#include "palcfg.h"
#include "palcommon.h"
#include "palette.h"
#include "util.h"
#include "video.h"
#include <errno.h>
#include <wctype.h>

#define   FONT_COLOR_DEFAULT        0x4F
#define   FONT_COLOR_YELLOW         0x2D
#define   FONT_COLOR_RED            0x1A
#define   FONT_COLOR_CYAN           0x8D
#define   FONT_COLOR_CYAN_ALT       0x8C
#define   FONT_COLOR_RED_ALT        0x17

int      g_fUpdatedInBattle      = FALSE;

static wchar_t internal_wbuffer[PAL_GLOBAL_BUFFER_SIZE];

#define   MESSAGE_MAX_BUFFER_SIZE   512

#define INCLUDE_CODEPAGE_H
#include "codepage.h"

#define SDLPAL_EXTRA_WORD_COUNT     1
static wchar_t* gc_rgszSDLPalWords[CP_MAX][SDLPAL_EXTRA_WORD_COUNT] = {
	{ L"\x8FD4\x56DE\x8A2D\x5B9A" },
	{ L"\x8FD4\x56DE\x8BBE\x7F6E" },
};

wchar_t* g_rcCredits[12];

TEXTLIB         g_TextLib;

PAL_FORCE_INLINE int
PAL_ParseLine(
	char     *line,
	char    **value,
	int      *length,
	int       deltrail
	)
{
	//
	// Remove the leading spaces
	//
	while (*line && iswspace(*line)) line++;
	//
	// Skip comments starting with '#'
	//
	if (*line && *line != '#')
	{
		//
		// Split the index and value
		//
		char* val = strchr(line, '=');
		if (val)
		{
			//
			// Remove the trailing spaces
			//
			char* end = line + strlen(line);
			int index;
			if (end > line && end[-1] == '\n') *(--end) = 0;
			if (deltrail) while (end > line && iswspace(end[-1])) *(--end) = 0;

			//
			// Parse the index and pass out value
			//
			if (sscanf(line, "%d", &index) == 1)
			{
				*value = val + 1;
				*length = end - *value;
				return index;
			}
		}
	}
	return 0;
}

PAL_FORCE_INLINE char *
PAL_ReadOneLine(
	char     *temp,
	int      limit,
	FILE     *fp
	)
{
	if (fgets(temp, limit, fp))
	{
		int n = strlen(temp);
		if (n == limit - 1 && temp[n - 1] != '\n' && !feof(fp))
		{
			// Line too long, try to read it as a whole
			int nn = 2;
			char *tmp = strdup(temp);
			while (!feof(fp))
			{
				if (!(tmp = (char *)realloc(tmp, nn * limit)))
				{
					TerminateOnError("PAL_ReadOneLine(): failed to allocate memory for long line!");
				}
				if (fgets(tmp + n, limit + 1, fp))
				{
					n += strlen(tmp + n);
					if (n < limit - 1 || temp[n - 1] == '\n')
						break;
					else
						nn++;
				}
			}
			if (tmp[n - 1] == '\n') tmp[n - 1] = 0;
			return tmp;
		}
		else
		{
			while (n > 0 && (temp[n - 1] == '\n' || temp[n - 1] == '\r')) temp[--n] = 0;
			return temp;
		}
	}
	else
		return NULL;
}

int
PAL_InitText(
   void
)
/*++
  Purpose:

    Initialize the in-game texts.

  Parameters:

    None.

  Return value:

    0 = success.
    -1 = memory allocation error.

--*/
{
	FILE       		*fpMsg;
	FILE           	*fpWord;
	unsigned int    *offsets;
	wchar_t      	*tmp;
	unsigned char  	*temp;
	int         wpos, wlen, i;

	//
	// Open the message and word data files.
	//
	fpMsg = UTIL_OpenRequiredFileForMode("m.msg", "rb");
	fpWord = UTIL_OpenRequiredFileForMode("word.dat", "rb");

	//
	// See how many words we have
	//
	fseek(fpWord, 0, SEEK_END);
	i = ftell(fpWord);

	//
	// Each word has 10 bytes
	//
	g_TextLib.nWords = (i + (10 - 1)) / 10;
	if (g_TextLib.nWords < (MAX_OBJECTS + 13)) g_TextLib.nWords = (MAX_OBJECTS + 13);

	//
	// Read the words
	//
	temp = (unsigned char *)malloc(10 * g_TextLib.nWords);
	if (temp == NULL)
	{
		fclose(fpWord);
		fclose(fpMsg);
		return -1;
	}
	fseek(fpWord, 0, SEEK_SET);
	if (fread(temp, 1, i, fpWord) < i)
	{
		fclose(fpWord);
		fclose(fpMsg);
		return -1;
	}
	memset(temp + i, 0, 10 * g_TextLib.nWords - i);

	//
	// Close the words file
	//
	fclose(fpWord);

	// Split the words and do code page conversion
	for (i = 0, wlen = 0; i < g_TextLib.nWords; i++)
	{
		int base = i * 10;
		int pos = base + 10 - 1;
		while (pos >= base && temp[pos] == ' ') temp[pos--] = 0;
		wlen += PAL_MultiByteToWideChar((const char*)temp + base, 10, NULL, 0) + 1;
	}
	g_TextLib.lpWordBuf = (wchar_t**)malloc(g_TextLib.nWords * sizeof(wchar_t*));
	if (g_TextLib.lpWordBuf == NULL)
	{
		free(temp);
		fclose(fpMsg);
		return -1;
	}
	tmp = (wchar_t*)malloc(wlen * sizeof(wchar_t));
	if (tmp == NULL)
	{
		free(g_TextLib.lpWordBuf);
		free(temp);
		fclose(fpMsg);
		return -1;
	}
	for (i = 0, wpos = 0; i < g_TextLib.nWords; i++)
	{
		int l;
		g_TextLib.lpWordBuf[i] = tmp + wpos;
		l = PAL_MultiByteToWideChar((const char*)temp + i * 10, 10, g_TextLib.lpWordBuf[i], wlen - wpos);
		if (l > 0 && g_TextLib.lpWordBuf[i][l - 1] == '1')
			g_TextLib.lpWordBuf[i][l - 1] = 0;
		g_TextLib.lpWordBuf[i][l] = 0;
		wpos += l + 1;
	}
	free(temp);

	//
	// Read the message offsets. The message offsets are in SSS.MKF #3
	//
	i = PAL_MKFGetChunkSize(3, gpGlobals->f.fpSSS) / sizeof(unsigned int);
	g_TextLib.nMsgs = i - 1;

	offsets = (unsigned int*)malloc(i * sizeof(unsigned int));
	if (offsets == NULL)
	{
		free(g_TextLib.lpWordBuf[0]);
		free(g_TextLib.lpWordBuf);
		fclose(fpMsg);
		return -1;
	}

	PAL_MKFReadChunk((unsigned char *)offsets, i * sizeof(unsigned int), 3, gpGlobals->f.fpSSS);

	//
	// Read the messages.
	//
	fseek(fpMsg, 0, SEEK_END);
	i = ftell(fpMsg);

	temp = (unsigned char *)malloc(i);
	if (temp == NULL)
	{
		free(offsets);
		free(g_TextLib.lpWordBuf[0]);
		free(g_TextLib.lpWordBuf);
		fclose(fpMsg);
		return -1;
	}

	fseek(fpMsg, 0, SEEK_SET);
	if (fread(temp, 1, i, fpMsg) < i)
	{
		free(offsets);
		free(g_TextLib.lpWordBuf[0]);
		free(g_TextLib.lpWordBuf);
		fclose(fpMsg);
		return -1;
	}

	fclose(fpMsg);

	// Split messages and do code page conversion here
	for (i = 0, wlen = 0; i < g_TextLib.nMsgs; i++)
	{
		wlen += PAL_MultiByteToWideChar((const char*)temp + offsets[i], offsets[i + 1] - offsets[i], NULL, 0) + 1;
	}
	g_TextLib.lpMsgBuf = (wchar_t**)malloc(g_TextLib.nMsgs * sizeof(wchar_t*));
	if (g_TextLib.lpMsgBuf == NULL)
	{
		free(g_TextLib.lpWordBuf[0]);
		free(g_TextLib.lpWordBuf);
		free(offsets);
		return -1;
	}
	tmp = (wchar_t*)malloc(wlen * sizeof(wchar_t));
	if (tmp == NULL)
	{
		free(g_TextLib.lpMsgBuf);
		free(g_TextLib.lpWordBuf[0]);
		free(g_TextLib.lpWordBuf);
		free(offsets);
		return -1;
	}
	for (i = 0, wpos = 0; i < g_TextLib.nMsgs; i++)
	{
		int l;
		g_TextLib.lpMsgBuf[i] = tmp + wpos;
		l = PAL_MultiByteToWideChar((const char*)temp + offsets[i], offsets[i + 1] - offsets[i], g_TextLib.lpMsgBuf[i], wlen - wpos);
		g_TextLib.lpMsgBuf[i][l] = 0;
		wpos += l + 1;
	}
	free(temp);
	free(offsets);
	memcpy(g_TextLib.lpWordBuf + SYSMENU_LABEL_LAUNCHSETTING, gc_rgszSDLPalWords[PAL_GetCodePage()], SDLPAL_EXTRA_WORD_COUNT * sizeof(const wchar_t*));

   g_TextLib.bCurrentFontColor = FONT_COLOR_DEFAULT;
   g_TextLib.bIcon = 0;
   g_TextLib.posIcon = 0;
   g_TextLib.nCurrentDialogLine = 0;
   g_TextLib.iDelayTime = 3;
   g_TextLib.posDialogTitle = PAL_XY(12, 8);
   g_TextLib.posDialogText = PAL_XY(44, 26);
   g_TextLib.bDialogPosition = kDialogUpper;
   g_TextLib.fUserSkip = FALSE;

   PAL_MKFReadChunk(g_TextLib.bufDialogIcons, 282, 12, gpGlobals->f.fpDATA);

   return 0;
}

void
PAL_FreeText(
   void
)
/*++
  Purpose:

    Free the memory used by the texts.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   if (g_TextLib.lpMsgBuf != NULL)
   {
      free(g_TextLib.lpMsgBuf[0]);
      free(g_TextLib.lpMsgBuf);
      g_TextLib.lpMsgBuf = NULL;
   }
   if (g_TextLib.lpWordBuf != NULL)
   {
      free(g_TextLib.lpWordBuf[0]);
      free(g_TextLib.lpWordBuf);
      g_TextLib.lpWordBuf = NULL;
   }
}

const wchar_t*
PAL_GetWord(
   int        iNumWord
)
/*++
  Purpose:

    Get the specified word.

  Parameters:

    [IN]  wNumWord - the number of the requested word.

  Return value:

    Pointer to the requested word. NULL if not found.

--*/
{
   return (iNumWord >= g_TextLib.nWords || !g_TextLib.lpWordBuf[iNumWord]) ? L"" : g_TextLib.lpWordBuf[iNumWord];
}

const wchar_t*
PAL_GetMsg(
   int        iNumMsg
)
/*++
  Purpose:

    Get the specified message.

  Parameters:

    [IN]  wNumMsg - the number of the requested message.

  Return value:

    Pointer to the requested message. NULL if not found.

--*/
{
   return (iNumMsg >= g_TextLib.nMsgs || !g_TextLib.lpMsgBuf[iNumMsg]) ? L"" : g_TextLib.lpMsgBuf[iNumMsg];
}

wchar_t*
PAL_UnescapeText(
   const wchar_t *lpszText
)
{
   wchar_t *buf = internal_wbuffer;
   
   if(wcsstr(lpszText, L"\\") == NULL)
      return (wchar_t*)lpszText;
   
   memset(internal_wbuffer, 0, sizeof(internal_wbuffer));

   while (*lpszText != L'\0')
   {
      switch (*lpszText)
      {
         case '-':
         case '\'':
         case '@':
         case '\"':
         case '$':
         case '~':
         case ')':
         case '(':
            lpszText++;
            break;
         case '\\':
            lpszText++;
         default:
            wcsncpy(buf++, lpszText++, 1);
            break;
      }
   }
   return internal_wbuffer;
}

void
PAL_DrawText(
   const wchar_t *lpszText,
   unsigned int      pos,
   unsigned char       bColor,
   int       fShadow,
   int       fUpdate,
   int       fUse8x8Font
)
{
    PAL_DrawTextUnescape(lpszText, pos, bColor, fShadow, fUpdate, fUse8x8Font, TRUE);
}

void
PAL_DrawTextUnescape(
   const wchar_t*    lpszText,
   unsigned int      pos,
   unsigned char       bColor,
   int       fShadow,
   int       fUpdate,
   int       fUse8x8Font,
   int       fUnescape
)
/*++
  Purpose:

    Draw text on the screen.

  Parameters:

    [IN]  lpszText - the text to be drawn.

    [IN]  pos - Position of the text.

    [IN]  bColor - Color of the text.

    [IN]  fShadow - TRUE if the text is shadowed or not.

    [IN]  fUpdate - TRUE if update the screen area.

    [IN]  fUse8x8Font - TRUE if use 8x8 font.

    [IN]  fUnescape - TRUE if unescaping needed.
 
  Return value:

    None.

--*/
{
   SDL_Rect   rect, urect;

   urect.x = rect.x = PAL_X(pos);
   urect.y = rect.y = PAL_Y(pos);
   urect.h = PAL_FontHeight() + (fShadow ? 1 : 0);
   urect.w = 0;

   // Handle text overflow
   if (rect.x >= 320) return;

   if(fUnescape)
      lpszText = PAL_UnescapeText(lpszText);

   while (*lpszText)
   {
      //
      // Draw the character
      //
      int char_width = PAL_CharWidth(*lpszText);

      if (fShadow)
      {
         //
         // Note: In the original PAL DOS version, 
         // the text has triple shadows, while Win95 only has one layer. 
         // It is suspected that there is a bug in the original Win95 version, 
         // so sdlpal chose to use triple shadows for both.
         //
         PAL_DrawCharOnSurface(*lpszText, gpScreen, PAL_XY(rect.x + 1, rect.y), 0);
         PAL_DrawCharOnSurface(*lpszText, gpScreen, PAL_XY(rect.x, rect.y + 1), 0);
         PAL_DrawCharOnSurface(*lpszText, gpScreen, PAL_XY(rect.x + 1, rect.y + 1), 0);
      }
      PAL_DrawCharOnSurface(*lpszText++, gpScreen, PAL_XY(rect.x, rect.y), bColor);
      rect.x += char_width;
      urect.w += char_width;
   }

   //
   // Update the screen area
   //
   if (fUpdate && urect.w > 0)
   {
      if (fShadow) urect.w++,urect.h++;
#define PROT_OFFSET 10 //consider offset in custom font
	  urect.x -= PROT_OFFSET;
	  urect.w += 2 * PROT_OFFSET;
	  urect.y -= PROT_OFFSET;
	  urect.h += 2*PROT_OFFSET;
	  if (urect.x < 0) urect.x = 0;
	  if (urect.y < 0) urect.y = 0;
      if (urect.x + urect.w > 320)
      {
         urect.w = 320 - urect.x;
      }
	  if (urect.y + urect.h > 200)
	  {
		  urect.h = 200 - urect.y;
	  }
      VIDEO_UpdateScreen(&urect);
   }
}

void
PAL_DialogSetDelayTime(
   int          iDelayTime
)
/*++
  Purpose:

    Set the delay time for dialog.

  Parameters:

    [IN]  iDelayTime - the delay time to be set.

  Return value:

    None.

--*/
{
   g_TextLib.iDelayTime = iDelayTime;
}

void
PAL_StartDialog(
   unsigned char         bDialogLocation,
   unsigned char         bFontColor,
   int          iNumCharFace,
   int         fPlayingRNG
)
{
   PAL_StartDialogWithOffset(bDialogLocation, bFontColor, iNumCharFace, fPlayingRNG, 0, 0);
}

void
PAL_StartDialogWithOffset(
   unsigned char         bDialogLocation,
   unsigned char         bFontColor,
   int          iNumCharFace,
   int         fPlayingRNG,
   int          xOff,
   int          yOff
)
/*++
  Purpose:

    Start a new dialog.

  Parameters:

    [IN]  bDialogLocation - the location of the text on the screen.

    [IN]  bFontColor - the font color of the text.

    [IN]  iNumCharFace - number of the character face in RGM.MKF.

    [IN]  fPlayingRNG - whether we are playing a RNG video or not.

  Return value:

    None.

--*/
{
   PAL_LARGE unsigned char buf[320 * 200];
   SDL_Rect       rect;

   if (gpGlobals->fInBattle && !g_fUpdatedInBattle)
   {
      //
      // Update the screen in battle, or the graphics may seem messed up
      //
      VIDEO_UpdateScreen(NULL);
      g_fUpdatedInBattle = TRUE;
   }

   g_TextLib.bIcon = 0;
   g_TextLib.posIcon = 0;
   g_TextLib.nCurrentDialogLine = 0;
   g_TextLib.posDialogTitle = PAL_XY(12, 8);
   g_TextLib.fUserSkip = FALSE;

   if (bFontColor != 0)
   {
      g_TextLib.bCurrentFontColor = bFontColor;
   }

   if (fPlayingRNG && iNumCharFace)
   {
      VIDEO_BackupScreen(gpScreen);
      g_TextLib.fPlayingRNG = TRUE;
   }

   switch (bDialogLocation)
   {
   case kDialogUpper:
      if (iNumCharFace > 0)
      {
         //
         // Display the character face at the upper part of the screen
         //
         if (PAL_MKFReadChunk(buf, 320 * 200, iNumCharFace, gpGlobals->f.fpRGM) > 0)
         {
            rect.w = PAL_RLEGetWidth((const unsigned char*)buf);
            rect.h = PAL_RLEGetHeight((const unsigned char*)buf);
            rect.x = 48 - rect.w / 2 + xOff;
            rect.y = 55 - rect.h / 2 + yOff;

            if (rect.x < 0)
            {
               rect.x = 0;
            }

            if (rect.y < 0)
            {
               rect.y = 0;
            }

            PAL_RLEBlitToSurface((const unsigned char*)buf, gpScreen, PAL_XY(rect.x, rect.y));

            if (rect.x < 0)
            {
               rect.x = 0;
            }
            if (rect.y < 0)
            {
               rect.y = 0;
            }

            VIDEO_UpdateScreen(&rect);
         }
      }
      g_TextLib.posDialogTitle = PAL_XY(iNumCharFace > 0 ? 80 : 12, 8);
      g_TextLib.posDialogText = PAL_XY(iNumCharFace > 0 ? 96 : 44, 26);
      break;

   case kDialogCenter:
      g_TextLib.posDialogText = PAL_XY(80, 40);
      break;

   case kDialogLower:
      if (iNumCharFace > 0)
      {
         //
         // Display the character face at the lower part of the screen
         //
         if (PAL_MKFReadChunk(buf, 320 * 200, iNumCharFace, gpGlobals->f.fpRGM) > 0)
         {
            rect.x = 270 - PAL_RLEGetWidth((const unsigned char*)buf) / 2 + xOff;
            rect.y = 144 - PAL_RLEGetHeight((const unsigned char*)buf) / 2 + yOff;

            PAL_RLEBlitToSurface((const unsigned char*)buf, gpScreen, PAL_XY(rect.x, rect.y));

            VIDEO_UpdateScreen(NULL);
         }
      }
      g_TextLib.posDialogTitle = PAL_XY(iNumCharFace > 0 ? 4 : 12, 108);
      g_TextLib.posDialogText = PAL_XY(iNumCharFace > 0 ? 20 : 44, 126);
      break;

   case kDialogCenterWindow:
      g_TextLib.posDialogText = PAL_XY(160, 40);
      break;
   }
   
   g_TextLib.posDialogTitle = PAL_XY( PAL_X(g_TextLib.posDialogTitle) + xOff, PAL_Y(g_TextLib.posDialogTitle) + yOff);
   g_TextLib.posDialogText = PAL_XY( PAL_X(g_TextLib.posDialogText) + xOff, PAL_Y(g_TextLib.posDialogText) + yOff);

   g_TextLib.bDialogPosition = bDialogLocation;
}

static void
PAL_DialogWaitForKeyWithMaximumSeconds(
   float fMaxSeconds
)
/*++
  Purpose:

    Wait for player to press a key after showing a dialog.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   PAL_LARGE SDL_Color   palette[256];
   SDL_Color   *pCurrentPalette, t;
   int         i;
   uint32_t    dwBeginningTicks = UTIL_GetTicks();

   //
   // get the current palette
   //
   pCurrentPalette = PAL_GetPalette(gpGlobals->wNumPalette, gpGlobals->fNightPalette);
   memcpy(palette, pCurrentPalette, sizeof(palette));

   if (g_TextLib.bDialogPosition != kDialogCenterWindow &&
      g_TextLib.bDialogPosition != kDialogCenter)
   {
      //
      // show the icon
      //
      const unsigned char* p = PAL_SpriteGetFrame(g_TextLib.bufDialogIcons, g_TextLib.bIcon);
      if (p != NULL)
      {
         SDL_Rect rect;

         rect.x = PAL_X(g_TextLib.posIcon);
         rect.y = PAL_Y(g_TextLib.posIcon);
         rect.w = 16;
         rect.h = 16;

         PAL_RLEBlitToSurface(p, gpScreen, g_TextLib.posIcon);
         VIDEO_UpdateScreen(&rect);
      }
   }

   PAL_ClearKeyState();

   while (TRUE)
   {
      UTIL_Delay(100);

      if (g_TextLib.bDialogPosition != kDialogCenterWindow &&
         g_TextLib.bDialogPosition != kDialogCenter)
      {
         //
         // palette shift
         //
         t = palette[0xF9];
         for (i = 0xF9; i < 0xFE; i++)
         {
            palette[i] = palette[i + 1];
         }
         palette[0xFE] = t;

         VIDEO_SetPalette(palette);
      }

      if (fabs(fMaxSeconds) > FLT_EPSILON && UTIL_GetTicks() - dwBeginningTicks > 1000 * fMaxSeconds)
      {
         break;
      }

      if (g_InputState.dwKeyPress != 0)
      {
         break;
      }
   }

   if (g_TextLib.bDialogPosition != kDialogCenterWindow &&
      g_TextLib.bDialogPosition != kDialogCenter)
   {
      PAL_SetPalette(gpGlobals->wNumPalette, gpGlobals->fNightPalette);
   }

   PAL_ClearKeyState();

   g_TextLib.fUserSkip = FALSE;
}

static void
PAL_DialogWaitForKey(
   void
)
{
   PAL_DialogWaitForKeyWithMaximumSeconds(0);
}

int
TEXT_DisplayText(
   const wchar_t *lpszText,
   int            x,
   int            y,
   int           isDialog
)
{
   //
   // normal texts
   //
   wchar_t text[2];
   unsigned char color;
   unsigned char isNumber=0;
   
   while (lpszText != NULL && *lpszText != '\0')
   {
      switch (*lpszText)
      {
         case '-':
            //
            // Set the font color to Cyan
            //
            if (g_TextLib.bCurrentFontColor == FONT_COLOR_CYAN)
            {
               g_TextLib.bCurrentFontColor = FONT_COLOR_DEFAULT;
            }
            else
            {
               g_TextLib.bCurrentFontColor = FONT_COLOR_CYAN;
            }
            lpszText++;
            break;
         case '\'':
            //
            // Set the font color to Red
            //
            if (g_TextLib.bCurrentFontColor == FONT_COLOR_RED)
            {
               g_TextLib.bCurrentFontColor = FONT_COLOR_DEFAULT;
            }
            else
            {
               g_TextLib.bCurrentFontColor = FONT_COLOR_RED;
            }
            lpszText++;
            break;
         case '@':
            //
            // Set the font color to Red
            //
            if (g_TextLib.bCurrentFontColor == FONT_COLOR_RED_ALT)
            {
               g_TextLib.bCurrentFontColor = FONT_COLOR_DEFAULT;
            }
            else
            {
               g_TextLib.bCurrentFontColor = FONT_COLOR_RED_ALT;
            }
            lpszText++;
            break;
         case '\"':
            //
            // Set the font color to Yellow
            //
            if(!isDialog)
            if (g_TextLib.bCurrentFontColor == FONT_COLOR_YELLOW)
            {
               g_TextLib.bCurrentFontColor = FONT_COLOR_DEFAULT;
            }
            else
            {
               g_TextLib.bCurrentFontColor = FONT_COLOR_YELLOW;
            }
            lpszText++;
            break;
            
         case '$':
            //
            // Set the delay time of text-displaying
            //
            g_TextLib.iDelayTime = wcstol(lpszText + 1, NULL, 10) * 10 / 7;
            lpszText += 3;
            break;
            
         case '~':
            //
            // Delay for a period and quit
            //
            if (g_TextLib.fUserSkip)
            {
               VIDEO_UpdateScreen(NULL);
            }
            if( !isDialog )
               UTIL_Delay(wcstol(lpszText + 1, NULL, 10) * 80 / 7);
            g_TextLib.nCurrentDialogLine = -1;
            g_TextLib.fUserSkip = FALSE;
            return x; // don't go further
            
         case ')':
            //
            // Set the waiting icon
            //
            g_TextLib.bIcon = 1;
            lpszText++;
            break;
            
         case '(':
            //
            // Set the waiting icon
            //
            g_TextLib.bIcon = 2;
            lpszText++;
            break;
            
         case '\\':
            lpszText++;
            
         default:
            text[0] = *lpszText++;
            text[1] = 0;
            
            color = g_TextLib.bCurrentFontColor;
            if(isDialog) {
               if(g_TextLib.bCurrentFontColor == FONT_COLOR_DEFAULT)
                  color = 0;
               if( text[0]>= '0' && text[0] <= '9' ) {
                  isNumber = 1;
               }else{
                  isNumber = 0;
               }
            }

            // Update the screen on each draw operation is time-consuming, so disable it if user want to skip
            if( isNumber )
               PAL_DrawNumber(text[0]-'0', 1, PAL_XY(x, y+4), kNumColorYellow, kNumAlignLeft);
            else
               PAL_DrawTextUnescape(text, PAL_XY(x, y), color, !isDialog, !isDialog && !g_TextLib.fUserSkip, FALSE, FALSE);
            x += PAL_CharWidth(text[0]);
            
            if (!isDialog && !g_TextLib.fUserSkip)
            {
               PAL_ClearKeyState();
               UTIL_Delay(g_TextLib.iDelayTime * 8);
               
               if (g_InputState.dwKeyPress & (kKeySearch | kKeyMenu))
               {
                  //
                  // User pressed a key to skip the dialog
                  //
                  g_TextLib.fUserSkip = TRUE;
               }
            }
      }
   }
   return x;
}

void
PAL_ShowDialogText(
	const wchar_t *lpszText
)
/*++
  Purpose:

    Show one line of the dialog text.

  Parameters:

    [IN]  lpszText - the text to be shown.

  Return value:

    None.

--*/
{
   SDL_Rect        rect;
   int             x, y;

   PAL_ClearKeyState();
   g_TextLib.bIcon = 0;

   if (gpGlobals->fInBattle && !g_fUpdatedInBattle)
   {
      //
      // Update the screen in battle, or the graphics may seem messed up
      //
      VIDEO_UpdateScreen(NULL);
      g_fUpdatedInBattle = TRUE;
   }

   if (g_TextLib.nCurrentDialogLine > 3)
   {
      //
      // The rest dialogs should be shown in the next page.
      //
      PAL_DialogWaitForKey();
      g_TextLib.nCurrentDialogLine = 0;
      VIDEO_RestoreScreen(gpScreen);
      VIDEO_UpdateScreen(NULL);
   }

   x = PAL_X(g_TextLib.posDialogText);
   y = PAL_Y(g_TextLib.posDialogText) + g_TextLib.nCurrentDialogLine * 18;

   if (g_TextLib.bDialogPosition == kDialogCenterWindow)
   {
      //
      // The text should be shown in a small window at the center of the screen
      //
      {
         unsigned int      pos;
         BOX       *lpBox;
		 int        i;
		 int        w = wcslen(lpszText);
		 int        len = 0;

		 for (i = 0; i < w; i++)
            len += PAL_CharWidth(lpszText[i]) >> 3;
         //
         // Create the window box
         //
         pos = PAL_XY(PAL_X(g_TextLib.posDialogText) - len * 4, PAL_Y(g_TextLib.posDialogText));
         // Follow behavior of original version
         lpBox = PAL_CreateSingleLineBoxWithShadow(pos, (len + 1) / 2, FALSE, g_TextLib.iDialogShadow);

         rect.x = PAL_X(pos);
         rect.y = PAL_Y(pos);
         rect.w = 320 - rect.x * 2 + 32;
         rect.h = 64;
         VIDEO_UpdateScreen(&rect);

         //
         // Show the text on the screen
         //
         TEXT_DisplayText(lpszText, PAL_X(pos) + 8 + ((len & 1) << 2), PAL_Y(pos) + 10, TRUE);
         VIDEO_UpdateScreen(&rect);

         PAL_DialogWaitForKeyWithMaximumSeconds(1.4f);

         //
         // Delete the box
         //
         PAL_DeleteBox(lpBox);
         VIDEO_UpdateScreen(&rect);

         PAL_EndDialog();
      }
   }
   else
   {
      int len = wcslen(lpszText);
      if (g_TextLib.nCurrentDialogLine == 0 &&
          g_TextLib.bDialogPosition != kDialogCenter &&
		  (lpszText[len - 1] == 0xff1a ||
		   lpszText[len - 1] == 0x2236 || // Special case for Pal WIN95 Simplified Chinese version
		   lpszText[len - 1] == ':')
		 )
      {
         //
         // name of character
         //
         PAL_DrawText(lpszText, g_TextLib.posDialogTitle, FONT_COLOR_CYAN_ALT, TRUE, TRUE, FALSE);
      }
      else
      {
         if (!g_TextLib.fPlayingRNG && g_TextLib.nCurrentDialogLine == 0)
         {
            //
            // Save the screen before we show the first line of dialog
            //
            VIDEO_BackupScreen(gpScreen);
         }
         
         x = TEXT_DisplayText(lpszText, x, y, FALSE);

		 // and update the full screen at once after all texts are drawn
		 if (g_TextLib.fUserSkip)
		 {
			 VIDEO_UpdateScreen(NULL);
		 }

         g_TextLib.posIcon = PAL_XY(x, y);
         g_TextLib.nCurrentDialogLine++;
      }
   }
}

void
PAL_ClearDialog(
   int       fWaitForKey
)
/*++
  Purpose:

    Clear the state of the dialog.

  Parameters:

    [IN]  fWaitForKey - whether wait for any key or not.

  Return value:

    None.

--*/
{
   if (g_TextLib.nCurrentDialogLine > 0 && fWaitForKey)
   {
      PAL_DialogWaitForKey();
   }

   g_TextLib.nCurrentDialogLine = 0;

   if (g_TextLib.bDialogPosition == kDialogCenter)
   {
      g_TextLib.posDialogTitle = PAL_XY(12, 8);
      g_TextLib.posDialogText = PAL_XY(44, 26);
      g_TextLib.bCurrentFontColor = FONT_COLOR_DEFAULT;
      g_TextLib.bDialogPosition = kDialogUpper;
   }
}

void
PAL_EndDialog(
   void
)
/*++
  Purpose:

    Ends a dialog.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   PAL_ClearDialog(TRUE);

   //
   // Set some default parameters, as there are some parts of script
   // which doesn't have a "start dialog" instruction before showing the dialog.
   //
   g_TextLib.posDialogTitle = PAL_XY(12, 8);
   g_TextLib.posDialogText = PAL_XY(44, 26);
   g_TextLib.bCurrentFontColor = FONT_COLOR_DEFAULT;
   g_TextLib.bDialogPosition = kDialogUpper;
   g_TextLib.fUserSkip = FALSE;
   g_TextLib.fPlayingRNG = FALSE;
}

int
PAL_IsInDialog(
   void
)
/*++
  Purpose:

    Check if there are dialog texts on the screen.

  Parameters:

    None.

  Return value:

    TRUE if there are dialog texts on the screen, FALSE if not.

--*/
{
   return (g_TextLib.nCurrentDialogLine != 0);
}

int
PAL_DialogIsPlayingRNG(
   void
)
/*++
  Purpose:

    Check if the script used the RNG playing parameter when displaying texts.

  Parameters:

    None.

  Return value:

    TRUE if the script used the RNG playing parameter, FALSE if not.

--*/
{
   return g_TextLib.fPlayingRNG;
}

static CODEPAGE g_codepage = CP_UTF_8;

CODEPAGE
PAL_GetCodePage(
	void
)
{
	return g_codepage;
}

void
PAL_SetCodePage(
	CODEPAGE    uCodePage
)
{
	g_codepage = uCodePage;
}

CODEPAGE
PAL_DetectCodePageForString(
	const char *   text,
	int            text_len,
	CODEPAGE       default_cp,
	int *          probability
)
{
	// Try to convert the content of word.dat with different codepages,
	// and use the codepage with minimal inconvertible characters
	// Works fine currently for detecting Simplified Chinese & Traditional Chinese.
	// Since we're using language files to support additional languages, this detection
	// should be fine for us now.
	int min_invalids = INT_MAX;

	if (text && text_len > 0)
	{
		// The file to be detected should not contain characters outside these ranges
		const static int valid_ranges[][2] = {
			{ 0x4E00, 0x9FFF }, // CJK Unified Ideographs
			{ 0x3400, 0x4DBF }, // CJK Unified Ideographs Extension A
			{ 0xF900, 0xFAFF }, // CJK Compatibility Ideographs
			{ 0x0020, 0x007E }, // Basic ASCII
			{ 0x3000, 0x301E }, // CJK Symbols
			{ 0xFF01, 0xFF5E }, // Fullwidth Forms
		};

		for (CODEPAGE i = CP_BIG5; i <= CP_GBK; i++)
		{
			int invalids, length = PAL_MultiByteToWideCharCP(i, text, text_len, NULL, 0);
			wchar_t *wbuf = (wchar_t *)malloc(length * sizeof(wchar_t));
			PAL_MultiByteToWideCharCP(i, text, text_len, wbuf, length);
			for (int j = invalids = 0; j < length; j++)
			{
				int score = 1;
				for (int k = 0; k < sizeof(valid_ranges) / sizeof(valid_ranges[0]); k++)
				{
					if (wbuf[j] >= valid_ranges[k][0] &&
						wbuf[j] <= valid_ranges[k][1])
					{
						score = 0;
						break;
					}
				}
				invalids += score;
			}
			// code page with less invalid chars wins
			if (invalids < min_invalids)
			{
				min_invalids = invalids;
				default_cp = i;
			}
			free(wbuf);
		}
	}
	if (probability)
	{
		if (min_invalids < text_len / 2)
			*probability = (text_len / 2 - min_invalids) * 200 / text_len;
		else
			*probability = 0;
	}

	return default_cp;
}

int
PAL_MultiByteToWideCharCP(
   CODEPAGE      cp,
   const char   *mbs,
   int           mbslength,
   wchar_t*      wcs,
   int           wcslength
)
/*++
  Purpose:

    Convert multi-byte string into the corresponding unicode string.

  Parameters:

    [IN]  cp - Code page for conversion.
    [IN]  mbs - Pointer to the multi-byte string.
	[IN]  mbslength - Length of the multi-byte string, or -1 for auto-detect.
	[IN]  wcs - Pointer to the wide string buffer.
	[IN]  wcslength - Length of the wide string buffer.

  Return value:

    The length of converted wide string. If mbslength is set to -1, the returned
	value includes the terminal null-char; otherwise, the null-char is not included.
	If wcslength is set to 0, wcs can be set to NULL and the return value is the
	required length of the wide string buffer.

--*/
{
	int i = 0, state = 0, wlen = 0, null = 0;

	if (mbslength == -1)
	{
		mbslength = strlen(mbs);
		null = 1;
	}

	if (!wcs)
	{
		switch (cp)
		{
		//case CP_SHIFTJIS:
		//	for (i = 0; i < mbslength && mbs[i]; i++)
		//	{
		//		if (state == 0)
		//		{
		//			if ((unsigned char)mbs[i] <= 0x80 || (unsigned char)mbs[i] >= 0xfd || ((unsigned char)mbs[i] >= 0xa0 && (unsigned char)mbs[i] <= 0xdf))
		//				wlen++;
		//			else
		//				state = 1;
		//		}
		//		else
		//		{
		//			wlen++;
		//			state = 0;
		//		}
		//	}
		//	break;
		case CP_GBK:
		case CP_BIG5:
			for (i = 0; i < mbslength && mbs[i]; i++)
			{
				if (state == 0)
				{
					if ((unsigned char)mbs[i] <= 0x80 || (unsigned char)mbs[i] == 0xff)
						wlen++;
					else
						state = 1;
				}
				else
				{
					wlen++;
					state = 0;
				}
			}
			break;
		case CP_UTF_8:
			for (i = 0; i < mbslength && mbs[i]; i++)
			{
				if (state == 0)
				{
					if ((unsigned char)mbs[i] >= 0x80)
					{
						unsigned char s = (unsigned char)mbs[i] << 1;
						while (s >= 0x80) { state++; s <<= 1; }
						if (state < 1 || state > 3)
						{
							state = 0;
							wlen++;
						}
					}
					else
						wlen++;
				}
				else
				{
					if ((unsigned char)mbs[i] >= 0x80 && (unsigned char)mbs[i] < 0xc0)
					{
						if (--state == 0) wlen++;
					}
					else
					{
						state = 0; wlen++;
					}
				}
			}
			break;
        case CP_UCS:
            i = mbslength;
            wlen = mbslength/2;
            break;
		default:
			return -1;
		}
		if (i < mbslength && !mbs[i]) null = 1;
		return wlen + null + (state != 0);
	}
	else
	{
		wchar_t invalid_char;
		switch (cp)
		{
		//case CP_SHIFTJIS:
		//	invalid_char = 0x30fb;
		//	for (i = 0; i < mbslength && wlen < wcslength && mbs[i]; i++)
		//	{
		//		if (state == 0)
		//		{
		//			if ((unsigned char)mbs[i] <= 0x80)
		//				wcs[wlen++] = mbs[i];
		//			else if ((unsigned char)mbs[i] >= 0xa0 && (unsigned char)mbs[i] <= 0xdf)
		//				wcs[wlen++] = cptbl_jis_half[(unsigned char)mbs[i] - 0xa0];
		//			else if ((unsigned char)mbs[i] == 0xfd)
		//				wcs[wlen++] = 0xf8f1;
		//			else if ((unsigned char)mbs[i] == 0xfe)
		//				wcs[wlen++] = 0xf8f2;
		//			else if ((unsigned char)mbs[i] == 0xff)
		//				wcs[wlen++] = 0xf8f3;
		//			else
		//				state = 1;
		//		}
		//		else
		//		{
		//			if ((unsigned char)mbs[i] < 0x40)
		//				wcs[wlen++] = 0x30fb;
		//			else if ((unsigned char)mbs[i - 1] < 0xa0)
		//				wcs[wlen++] = cptbl_jis[(unsigned char)mbs[i - 1] - 0x81][(unsigned char)mbs[i] - 0x40];
		//			else
		//				wcs[wlen++] = cptbl_jis[(unsigned char)mbs[i - 1] - 0xc1][(unsigned char)mbs[i] - 0x40];
		//			state = 0;
		//		}
		//	}
		//	break;
		case CP_GBK:
			invalid_char = 0x3f;
			for (i = 0; i < mbslength && wlen < wcslength && mbs[i]; i++)
			{
				if (state == 0)
				{
					if ((unsigned char)mbs[i] < 0x80)
						wcs[wlen++] = mbs[i];
					else if ((unsigned char)mbs[i] == 0x80)
						wcs[wlen++] = 0x20ac;
					else if ((unsigned char)mbs[i] == 0xff)
						wcs[wlen++] = 0xf8f5;
					else
						state = 1;
				}
				else
				{
					if ((unsigned char)mbs[i] < 0x40)
						wcs[wlen++] = invalid_char;
					else
						wcs[wlen++] = cptbl_gbk[(unsigned char)mbs[i - 1] - 0x81][(unsigned char)mbs[i] - 0x40];
					state = 0;
				}
			}
			break;
		case CP_BIG5:
			invalid_char = 0x3f;
			for (i = 0; i < mbslength && wlen < wcslength && mbs[i]; i++)
			{
				if (state == 0)
				{
					if ((unsigned char)mbs[i] <= 0x80)
						wcs[wlen++] = mbs[i];
					else if ((unsigned char)mbs[i] == 0xff)
						wcs[wlen++] = 0xf8f8;
					else
						state = 1;
				}
				else
				{
					if ((unsigned char)mbs[i] < 0x40 || ((unsigned char)mbs[i] >= 0x7f && (unsigned char)mbs[i] <= 0xa0))
						wcs[wlen++] = invalid_char;
					else if ((unsigned char)mbs[i] <= 0x7e)
						wcs[wlen++] = cptbl_big5[(unsigned char)mbs[i - 1] - 0x81][(unsigned char)mbs[i] - 0x40];
					else
						wcs[wlen++] = cptbl_big5[(unsigned char)mbs[i - 1] - 0x81][(unsigned char)mbs[i] - 0x60];
					state = 0;
				}
			}
			break;
		case CP_UTF_8:
			invalid_char = 0x3f;
			for (i = 0; i < mbslength && wlen < wcslength && mbs[i]; i++)
			{
				if (state == 0)
				{
					if ((unsigned char)mbs[i] >= 0x80)
					{
						unsigned char s = (unsigned char)mbs[i] << 1;
						while (s >= 0x80) { state++; s <<= 1; }
						if (state < 1 || state > 3)
						{
							state = 0;
							wcs[wlen++] = invalid_char;
						}
						else
						{
							wcs[wlen] = s >> (state + 1);
						}
					}
					else
						wcs[wlen++] = mbs[i];
				}
				else
				{
					if ((unsigned char)mbs[i] >= 0x80 && (unsigned char)mbs[i] < 0xc0)
					{
						wcs[wlen] <<= 6;
						wcs[wlen] |= (unsigned char)mbs[i] & 0x3f;
						if (--state == 0) wlen++;
					}
					else
					{
						state = 0;
						wcs[wlen++] = invalid_char;
					}
				}
			}
			break;
        case CP_UCS:
            for (i = 0; i < mbslength && wlen < wcslength; i+=2){
                uint8_t *ptr = (uint8_t*)&wcs[wlen++];
                *(ptr+1)=mbs[i];
                *ptr    =mbs[i+1];
            }
            break;
		default:
			return -1;
		}
		if (state != 0 && wlen < wcslength)
		{
			wcs[wlen++] = invalid_char;
		}
		if (null || (i < mbslength && !mbs[i]))
		{
			if (wlen < wcslength)
				wcs[wlen++] = 0;
			else
				wcs[wlen - 1] = 0;
		}
		return wlen;
	}
}

int
PAL_MultiByteToWideChar(
   const char    *mbs,
   int           mbslength,
   wchar_t       *wcs,
   int           wcslength
)
/*++
  Purpose:

    Convert multi-byte string into the corresponding unicode string.

  Parameters:

    [IN]  mbs - Pointer to the multi-byte string.
	[IN]  mbslength - Length of the multi-byte string, or -1 for auto-detect.
	[IN]  wcs - Pointer to the wide string buffer.
	[IN]  wcslength - Length of the wide string buffer.

  Return value:

    The length of converted wide string. If mbslength is set to -1, the returned
	value includes the terminal null-char; otherwise, the null-char is not included.
	If wcslength is set to 0, wcs can be set to NULL and the return value is the
	required length of the wide string buffer.

--*/
{
	return PAL_MultiByteToWideCharCP(g_codepage, mbs, mbslength, wcs, wcslength);
}

int
PAL_swprintf(
	wchar_t* buffer,
	int count,
	const wchar_t *format,
	...
)
/*++
  Purpose:

    Formatted wide-character output conversion that output Chinese characters correctly.
	This function supported a subset of format strings that are commonly supported by
	various C libraries, which can be formalized as following:

	%[flags] [width] [.precision] [{h | l | ll}] type

	When handling '%c' and '%s', this function follows the Linux's library convention,
	which means '%c' and '%s' always output multi-byte strings, and '%lc' and '%ls'
	always output wide-char strings.

  Parameters:

    [IN]  buffer - Storage location for output.
	[IN]  count - Length of the output buffer in characters including the termination null one.
	[IN]  format - Format-control string.
	[IN]  ... - Optional arguments.

  Return value:

    The length of outputed wide string, not including the termination null character.

--*/
{
	va_list ap;
	const wchar_t * const format_end = format + wcslen(format);
	const wchar_t * const buffer_end = buffer + count - 1;
	wchar_t chr_buf[2] = { 0, 0 };
	const wchar_t* fmt_start = NULL;
	wchar_t* cur_fmt = NULL;
	int fmt_len = 0;
	int state, precision = 0, width = 0;
	int left_aligned = 0, wide = 0, narrow = 0;
	int width_var = 0, precision_var = 0, precision_defined = 0;

	// Buffer & length check
	if (buffer == NULL || format == NULL)
	{
		errno = EINVAL;
		return -1;
	}

	if (buffer_end <= buffer)
		return 0;

	va_start(ap, format);

	count = 0; state = 0;
	while (buffer < buffer_end && format < format_end)
	{
		switch (state)
		{
		case 0: // Outside format spec
			if (*format != L'%')
			{
				*buffer++ = *format++;
				count++;
			}
			else
			{
				fmt_start = format++;
				left_aligned = wide = narrow = 0;
				precision_var = width_var = 0;
				precision_defined = 0;
				state = 1;
			}
			continue;
		case 1: // [flags]
			switch (*format)
			{
			case L'-':
				left_aligned = 1;
			case L'+':
			case L' ':
			case L'#':
			case L'0':
				format++;
				continue;
			default:
				state = 2;
				width = width_var = 0;
			}
		case 2: // [width]
			switch (*format)
			{
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				if (width >= 0)
					width = width * 10 + (*format - L'0');
				format++;
				continue;
			case '*':
				if (width == 0)
					width_var = 1;
				format++;
				continue;
			case '.':
				format++;
				precision = precision_var = 0;
				precision_defined = 1;
				state = 3;
				continue;
			default:
				state = 4;
				continue;
			}
		case 3: // [.precision]
			switch (*format)
			{
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				if (precision >= 0)
					precision = precision * 10 + (*format - L'0');
				format++;
				continue;
			case '*':
				if (precision == 0)
					precision_var = 1;
				format++;
				continue;
			default:
				state = 4;
			}
		case 4: // [{h | l | ll}]
			switch (*format)
			{
			case 'l': if (narrow == 0) wide++; format++; continue;
			case 'h': if (wide == 0) narrow++; format++; continue;
			default: state = 5;
			}
		case 5: // type
			if (*format == 'c' || *format == 's')
			{
				// We handle char & str specially
				wchar_t* buf;
				int len;
				int i;

				// Check width
				if (width_var)
				{
					width = va_arg(ap, int);
					left_aligned = (width < 0);
					width = left_aligned ? -width : width;
				}
				// Although precision has no meaning to '%c' output, however
				// the argument still needs to be read if '.*' is provided
				if (precision_var)
					precision = va_arg(ap, int);
				else if (!precision_defined)
					precision = INT_MAX;

				if (*format == 's')
				{
					// For ANSI string, convert it through PAL_MultiByteToWideChar
					// To improve effciency, here just test the length and left
					// actual conversion later directly into the output buffer
					if (wide)
					{
						buf = va_arg(ap, wchar_t*);
						len = wcslen(buf);
					}
					else
					{
						buf = (wchar_t*)va_arg(ap, char*);
						len = PAL_MultiByteToWideChar((const char*)buf, -1, NULL, 0) - 1;
					}
				}
				else
				{
					// For ANSI character, put it into the internal buffer
					if (wide)
						chr_buf[0] = va_arg(ap, wchar_t);
					else
						chr_buf[0] = va_arg(ap, int);
					buf = chr_buf; len = 1;
				}

				// Limit output length no longer then precision
				if (precision > (int)len)
					precision = len;

				// Left-side padding
				for (i = 0; !left_aligned && i < width - precision && buffer < buffer_end; i++)
					*buffer++ = L' ', count++;

				// Do not overflow the output buffer
				if (buffer + precision > buffer_end)
					precision = buffer_end - buffer;

				// Convert or copy string (char) into output buffer
				if (*format == 's' && !wide)
					PAL_MultiByteToWideChar((const char*)buf, -1, buffer, precision);
				else
					wcsncpy(buffer, buf, precision);
				buffer += precision; count += precision;

				// Right-side padding
				for (i = 0; left_aligned && i < width - precision && buffer < buffer_end; i++)
					*buffer++ = L' ', count++;
			}
			else
			{
				// For other types, pass them directly into vswprintf
				int cur_cnt = 0;
				va_list apd;

				// We copy this argument's format string into internal buffer
				if (fmt_len < (int)(format - fmt_start + 1))
					cur_fmt = realloc(cur_fmt, ((fmt_len = format - fmt_start + 1) + 1) * sizeof(wchar_t));
				wcsncpy(cur_fmt, fmt_start, fmt_len);
				cur_fmt[fmt_len] = L'\0';
				// And pass it into vswprintf to get the output
				va_copy(apd, ap);
				cur_cnt = vswprintf(buffer, buffer_end - buffer, cur_fmt, apd);
				va_end(apd);
				buffer += cur_cnt; count += cur_cnt;

				// Then we need to move the argument pointer into next one
				// Check if width/precision should be read from arguments
				if (width_var) va_arg(ap, int);
				if (precision_var) va_arg(ap, int);

				// Move pointer to pass the actual value argument
				switch (*format)
				{
				case 'd':
				case 'i':
				case 'o':
				case 'u':
				case 'x':
				case 'X':
					if (wide == 1)
						va_arg(ap, long);
					else if (wide >= 2)
						va_arg(ap, long long);
					else
						va_arg(ap, int);
					break;
				case 'e':
				case 'E':
				case 'f':
				case 'g':
				case 'G':
				case 'a':
				case 'A':
					va_arg(ap, double);
					break;
				case 'p':
				case 'n':
					va_arg(ap, void*);
					break;
				}
			}
			state = 0;
			format++;
			break;
		}
	}

	// If the format string is malformed, try to copy it into the dest buffer
	if (state && buffer < buffer_end)
	{
		int fmt_len = format - fmt_start;
		int buf_len = buffer_end - buffer;
		if (fmt_len <= buf_len)
		{
			wcsncpy(buffer, fmt_start, buf_len);
			buffer += fmt_len;
		}
		else
		{
			wcsncpy(buffer, fmt_start, buf_len);
			buffer += buf_len;
		}
	}

	// NULL-terminate the string
	*buffer = L'\0';

	va_end(ap);
	return count;
}
