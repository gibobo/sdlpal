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

#include "input.h"
#include "common.h"
#include "global.h"
#include "input.h"
#include "main.h"
#include "palcfg.h"
#include "video/video.h"
#include <SDL_events.h>

volatile PALINPUTSTATE g_InputState;
#ifdef PAL_HAS_JOYSTICKS
static SDL_Joystick *g_pJoy = NULL;
static int joystick_axis_X;
static int joystick_axis_Y;
static int joystickNeedUpdate = FALSE;
#endif

static void _default_init_filter() {}
static int _default_input_event_filter(const SDL_Event *event, volatile PALINPUTSTATE *state) { return 0; }
static void _default_input_shutdown_filter() {}

static void (*input_init_filter)() = _default_init_filter;
static int (*input_event_filter)(const SDL_Event *, volatile PALINPUTSTATE *) = _default_input_event_filter;
static void (*input_shutdown_filter)() = _default_input_shutdown_filter;

static const int g_KeyMap[][2] = {
   { SDLK_UP,        kKeyUp },
   { SDLK_KP_8,      kKeyUp },
   { SDLK_DOWN,      kKeyDown },
   { SDLK_KP_2,      kKeyDown },
   { SDLK_LEFT,      kKeyLeft },
   { SDLK_KP_4,      kKeyLeft },
   { SDLK_RIGHT,     kKeyRight },
   { SDLK_KP_6,      kKeyRight },
   { SDLK_ESCAPE,    kKeyMenu },
   { SDLK_INSERT,    kKeyMenu },
   { SDLK_LALT,      kKeyMenu },
   { SDLK_RALT,      kKeyMenu },
   { SDLK_KP_0,      kKeyMenu },
   { SDLK_RETURN,    kKeySearch },
   { SDLK_SPACE,     kKeySearch },
   { SDLK_KP_ENTER,  kKeySearch },
   { SDLK_LCTRL,     kKeySearch },
   { SDLK_PAGEUP,    kKeyPgUp },
   { SDLK_KP_9,      kKeyPgUp },
   { SDLK_PAGEDOWN,  kKeyPgDn },
   { SDLK_KP_3,      kKeyPgDn },
   { SDLK_HOME,      kKeyHome },
   { SDLK_KP_7,      kKeyHome },
   { SDLK_END,       kKeyEnd },
   { SDLK_KP_1,      kKeyEnd },
   { SDLK_r,         kKeyRepeat },
   { SDLK_a,         kKeyAuto },
   { SDLK_d,         kKeyDefend },
   { SDLK_e,         kKeyUseItem },
   { SDLK_w,         kKeyThrowItem },
   { SDLK_q,         kKeyFlee },
   { SDLK_f,         kKeyForce },
   { SDLK_s,         kKeyStatus }
};

static int
PAL_GetCurrDirection(
   void
)
/*++
  Purpose:

    Get the current walking direction.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   int i, iCurrDir = kDirSouth;

   for (i = 1; i < sizeof(g_InputState.dwKeyOrder) / sizeof(g_InputState.dwKeyOrder[0]); i++)
      if (g_InputState.dwKeyOrder[iCurrDir] < g_InputState.dwKeyOrder[i]) iCurrDir = i;

   if (!g_InputState.dwKeyOrder[iCurrDir]) iCurrDir = kDirUnknown;

   return iCurrDir;
}

static void
PAL_KeyDown(
   int         key,
   int        fRepeat
)
/*++
  Purpose:

    Called when user pressed a key.

  Parameters:

    [IN]  key - keycode of the pressed key.

  Return value:

    None.

--*/
{
   int iCurrDir = kDirUnknown;

   if (!fRepeat)
   {
      if (key & kKeyDown)
      {
         iCurrDir = kDirSouth;
      }
      else if (key & kKeyLeft)
      {
         iCurrDir = kDirWest;
      }
      else if (key & kKeyUp)
      {
         iCurrDir = kDirNorth;
      }
      else if (key & kKeyRight)
      {
         iCurrDir = kDirEast;
      }

      if (iCurrDir != kDirUnknown)
      {
         g_InputState.dwKeyMaxCount++;
         g_InputState.dwKeyOrder[iCurrDir] = g_InputState.dwKeyMaxCount;
         g_InputState.dir = PAL_GetCurrDirection();
      }
   }

   g_InputState.dwKeyPress |= key;
}

static void
PAL_KeyUp(
   int         key
)
/*++
  Purpose:

    Called when user released a key.

  Parameters:

    [IN]  key - keycode of the released key.

  Return value:

    None.

--*/
{
   int iCurrDir = kDirUnknown;

   if (key & kKeyDown)
   {
      iCurrDir = kDirSouth;
   }
   else if (key & kKeyLeft)
   {
      iCurrDir = kDirWest;
   }
   else if (key & kKeyUp)
   {
      iCurrDir = kDirNorth;
   }
   else if (key & kKeyRight)
   {
      iCurrDir = kDirEast;
   }

   if (iCurrDir != kDirUnknown)
   {
      g_InputState.dwKeyOrder[iCurrDir] = 0;
      iCurrDir = PAL_GetCurrDirection();
      g_InputState.dwKeyMaxCount = (iCurrDir == kDirUnknown) ? 0 : g_InputState.dwKeyOrder[iCurrDir];
      g_InputState.dir = iCurrDir;
   }
}

static void
PAL_UpdateKeyboardState(
   void
)
/*++
  Purpose:

    Poll & update keyboard state.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   static unsigned int   rgdwKeyLastTime[sizeof(g_KeyMap) / sizeof(g_KeyMap[0])] = {0};
   const unsigned char*        keyState = (const unsigned char*)SDL_GetKeyboardState(NULL);
   int            i;
   unsigned int          dwCurrentTime = UTIL_GetTicks();

   for (i = 0; i < sizeof(g_KeyMap) / sizeof(g_KeyMap[0]); i++)
   {
      if (keyState[SDL_GetScancodeFromKey(g_KeyMap[i][0])])
      {
         if (dwCurrentTime > rgdwKeyLastTime[i])
         {
            PAL_KeyDown(g_KeyMap[i][1], (rgdwKeyLastTime[i] != 0));
            if (gConfig.fEnableKeyRepeat)
            {
               rgdwKeyLastTime[i] = dwCurrentTime + (rgdwKeyLastTime[i] == 0 ? 200 : 75);
            }
            else
            {
               rgdwKeyLastTime[i] = 0xFFFFFFFF;
            }
         }
      }
      else
      {
         if (rgdwKeyLastTime[i] != 0)
         {
            PAL_KeyUp(g_KeyMap[i][1]);
            rgdwKeyLastTime[i] = 0;
         }
      }
   }
}

static void
PAL_KeyboardEventFilter(
   const SDL_Event       *lpEvent
)
/*++
  Purpose:

    Handle keyboard events.

  Parameters:

    [IN]  lpEvent - pointer to the event.

  Return value:

    None.

--*/
{
   if (lpEvent->type == SDL_KEYDOWN)
   {
      //
      // Pressed a key
      //
      if (lpEvent->key.keysym.mod & KMOD_ALT)
      {
         if (lpEvent->key.keysym.sym == SDLK_F4)
         {
            //
            // Pressed Alt+F4 (Exit program)...
            //
            PAL_Shutdown(0);
         }
      }
      else if (lpEvent->key.keysym.sym == SDLK_z)
      {
         Filter_StepParamSlot(1);
      }
      else if (lpEvent->key.keysym.sym == SDLK_x)
      {
         Filter_StepParamSlot(-1);
      }
      else if (lpEvent->key.keysym.sym == SDLK_COMMA)
      {
         Filter_StepCurrentParam(1);
      }
      else if (lpEvent->key.keysym.sym == SDLK_PERIOD)
      {
         Filter_StepCurrentParam(-1);
      }
   }
}

#ifdef PAL_HAS_JOYSTICKS
static void
PAL_JoystickEventFilter(
   const SDL_Event       *lpEvent
)
/*++
  Purpose:

    Handle joystick events.

  Parameters:

    [IN]  lpEvent - pointer to the event.

  Return value:

    None.

--*/
{
   switch (lpEvent->type)
   {
   case SDL_JOYAXISMOTION:
      joystickNeedUpdate = TRUE;
      //
      // Moved an axis on joystick
      //
      switch (lpEvent->jaxis.axis)
      {
      case 0:
         //
         // X axis
         //
         if (lpEvent->jaxis.value > 3200)
         {
            joystick_axis_X = 1;
         }
         else if (lpEvent->jaxis.value < -3200)
         {
            joystick_axis_X = -1;
         }
         else
         {
            joystick_axis_X = 0;
         }
         break;

      case 1:
         //
         // Y axis
         //
         if (lpEvent->jaxis.value > 3200)
         {
            joystick_axis_Y = 1;
         }
         else if (lpEvent->jaxis.value < -3200)
         {
            joystick_axis_Y = -1;
         }
         else
         {
            joystick_axis_Y = 0;
         }
         break;
      }
      break;

   case SDL_JOYHATMOTION:
      //
      // Pressed the joystick hat button
      //
      switch (lpEvent->jhat.value)
      {
         case SDL_HAT_LEFT:
         case SDL_HAT_LEFTUP:
            g_InputState.dir = kDirWest;
            g_InputState.dwKeyPress = kKeyLeft;
            break;

         case SDL_HAT_RIGHT:
         case SDL_HAT_RIGHTDOWN:
            g_InputState.dir = kDirEast;
            g_InputState.dwKeyPress = kKeyRight;
            break;

         case SDL_HAT_UP:
         case SDL_HAT_RIGHTUP:
            g_InputState.dir = kDirNorth;
            g_InputState.dwKeyPress = kKeyUp;
            break;

         case SDL_HAT_DOWN:
         case SDL_HAT_LEFTDOWN:
            g_InputState.dir = kDirSouth;
            g_InputState.dwKeyPress = kKeyDown;
            break;

         case SDL_HAT_CENTERED:
            g_InputState.dir = kDirUnknown;
            g_InputState.dwKeyPress = kKeyNone;
            break;
      }
      break;

   case SDL_JOYBUTTONDOWN:
      //
      // Pressed the joystick button
      //
      switch (lpEvent->jbutton.button & 1)
      {
      case 0:
         g_InputState.dwKeyPress |= kKeyMenu;
         break;

      case 1:
         g_InputState.dwKeyPress |= kKeySearch;
         break;
      }
      break;
   }
}


static void
PAL_UpdateJoyStickState(
void
)
/*++
 Purpose:
 
 Poll & update joystick state.
 
 Parameters:
 
 None.
 
 Return value:
 
 None.
 
 --*/
{
   if( joystick_axis_X == 1 && joystick_axis_Y >= 0 )
   {
      g_InputState.dir = kDirEast;
      g_InputState.dwKeyPress |= kKeyRight;
   }
   else if( joystick_axis_X == -1 && joystick_axis_Y <= 0 )
   {
      g_InputState.dir = kDirWest;
      g_InputState.dwKeyPress |= kKeyLeft;
   }
   else if( joystick_axis_Y == 1 && joystick_axis_X <= 0 )
   {
      g_InputState.dir = kDirSouth;
      g_InputState.dwKeyPress |= kKeyDown;
   }
   else if( joystick_axis_Y == -1 && joystick_axis_X >= 0 )
   {
      g_InputState.dir = kDirNorth;
      g_InputState.dwKeyPress |= kKeyUp;
   }
   else
   {
      g_InputState.dir = kDirUnknown;
      if(!input_event_filter)
         g_InputState.dwKeyPress = kKeyNone;
   }
}

#endif

static int SDLCALL
PAL_EventFilter(
   const SDL_Event       *lpEvent
)

/*++
  Purpose:

    SDL event filter function. A filter to process all events.

  Parameters:

    [IN]  lpEvent - pointer to the event.

  Return value:

    1 = the event will be added to the internal queue.
    0 = the event will be dropped from the queue.

--*/
{
   switch (lpEvent->type)
   {
   case SDL_WINDOWEVENT:
      if (lpEvent->window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
      {
         //
         // resized the window
         //
         VIDEO_Resize(
             lpEvent->window.data1,
             lpEvent->window.data2);
      }
      break;

   case SDL_APP_WILLENTERBACKGROUND:
      VIDEO_RenderPaused(TRUE);
      break;

   case SDL_APP_DIDENTERFOREGROUND:
      VIDEO_RenderPaused(FALSE);
      VIDEO_UpdateScreen(NULL);
      break;

   case SDL_QUIT:
      //
      // clicked on the close button of the window. Quit immediately.
      //
      PAL_Shutdown(0);
   }

   PAL_KeyboardEventFilter(lpEvent);
#ifdef PAL_HAS_JOYSTICKS
   PAL_JoystickEventFilter(lpEvent);
#endif

   //
   // All events are handled here; don't put anything to the internal queue
   //
   return 0;
}

void
PAL_ClearKeyState(
   void
)
/*++
  Purpose:

    Clear the record of pressed keys.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   g_InputState.dwKeyPress = kKeyNone;
}

void
PAL_InitInput(
   void
)
/*++
  Purpose:

    Initialize the input subsystem.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   memset((void *)&g_InputState, 0, sizeof(g_InputState));
   g_InputState.dir = kDirUnknown;

   //
   // Check for joystick
   //
#ifdef PAL_HAS_JOYSTICKS
   if (SDL_NumJoysticks() > 0)
   {
      int i;
      for (i = 0; i < SDL_NumJoysticks(); i++)
      {            
         if (SDL_JoystickOpen(i) != NULL)
         {
            SDL_JoystickEventState(SDL_ENABLE);
            break;
         }
      }

   }
#endif

   input_init_filter();
}

void
PAL_ShutdownInput(
   void
)
/*++
  Purpose:

    Shutdown the input subsystem.

  Parameters:

    None.

  Return value:

    None.

--*/
{
#ifdef PAL_HAS_JOYSTICKS
   if (g_pJoy != NULL)
   {
      SDL_JoystickClose(g_pJoy);
      g_pJoy = NULL;
   }
#endif
   input_shutdown_filter();
}

static int
PAL_PollEvent(
   SDL_Event *event
)
/*++
  Purpose:

    Poll and process one event.

  Parameters:

    [OUT] event - Events polled from SDL.

  Return value:

    Return value of PAL_PollEvent.

--*/
{
   SDL_Event evt;

   int ret = SDL_PollEvent(&evt);
   if (ret != 0 && !input_event_filter(&evt, &g_InputState))
   {
      PAL_EventFilter(&evt);
   }

   if (event != NULL)
   {
      *event = evt;
   }

   return ret;
}

void
PAL_ProcessEvent(
   void
)
/*++
  Purpose:

    Process all events.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   while (PAL_PollEvent(NULL));

   PAL_UpdateKeyboardState();
#ifdef PAL_HAS_JOYSTICKS
   if (joystickNeedUpdate)
     PAL_UpdateJoyStickState();
   joystickNeedUpdate = FALSE;
#endif
}

void PAL_SetKeyInput(unsigned int key) {
  g_InputState.dwKeyPress = key;
}

unsigned int PAL_GetKeyInput(void) {
  return g_InputState.dwKeyPress;
}

void PAL_SetDirInput(unsigned char dir) {
  g_InputState.dir = dir;
}

unsigned char PAL_GetDirInput(void) {
  return g_InputState.dir;
}
