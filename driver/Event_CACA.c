#include "../src/input.h"
#include "DrvIf_internal.h"
#include "caca.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern caca_display_t *dp;
static unsigned char *rgdwKeyLastTime = NULL;
static unsigned char rgdwKeyCount = 0;
static unsigned int counter = 0; // Event index
static const int g_KeyMap[][2] = {
    {CACA_KEY_UP, kKeyUp},
    {'8', kKeyUp},
    {CACA_KEY_DOWN, kKeyDown},
    {'2', kKeyDown},
    {CACA_KEY_LEFT, kKeyLeft},
    {'4', kKeyLeft},
    {CACA_KEY_RIGHT, kKeyRight},
    {'6', kKeyRight},
    {CACA_KEY_ESCAPE, kKeyMenu},
    {CACA_KEY_INSERT, kKeyMenu},
    {'0', kKeyMenu},
    {13, kKeySearch},
    {CACA_KEY_BACKSPACE, kKeySearch},
    {CACA_KEY_PAGEUP, kKeyPgUp},
    {'9', kKeyPgUp},
    {CACA_KEY_PAGEDOWN, kKeyPgDn},
    {'3', kKeyPgDn},
    {CACA_KEY_HOME, kKeyHome},
    {'7', kKeyHome},
    {CACA_KEY_END, kKeyEnd},
    {'1', kKeyEnd},
    {'r', kKeyRepeat},
    {'a', kKeyAuto},
    {'d', kKeyDefend},
    {'e', kKeyUseItem},
    {'w', kKeyThrowItem},
    {'q', kKeyFlee},
    {'f', kKeyForce},
    {'s', kKeyStatus}};

int DRIVER_Process_Events(void)
{
    caca_event_t ev;
    int res = 0;
    int event, key, i;
    if (dp)
        res = caca_get_event(dp, CACA_EVENT_QUIT | CACA_EVENT_KEY_PRESS | CACA_EVENT_KEY_RELEASE, &ev, 0);
    if (res)
    {
        event = caca_get_event_type(&ev);
        switch (event)
        {
            case CACA_EVENT_QUIT:
                res = -1;
                break;
            case CACA_EVENT_KEY_PRESS:
            case CACA_EVENT_KEY_RELEASE:
                key = caca_get_event_key_ch(&ev);
                if (key == CACA_KEY_CTRL_C)
                {
                    res = -1;
                    break;
                }
                for (i = 0; i < rgdwKeyCount; i++)
                {
                    if (g_KeyMap[i][0] == key)
                    {
                        if (event == CACA_EVENT_KEY_PRESS)
                        {
                            PAL_KeyDown(g_KeyMap[i][1], (rgdwKeyLastTime[i] != 0));
                            rgdwKeyLastTime[i] = 0xFF;
                        }
                        else
                        {
                            PAL_KeyUp(g_KeyMap[i][1]);
                            rgdwKeyLastTime[i] = 0;
                        }
                        break;
                    }
                }
                break;
            default:
                break;
        }
    }

    return res;
}

int DRIVER_Init_Event(void)
{
    rgdwKeyCount = sizeof(g_KeyMap) / sizeof(g_KeyMap[0]);
    rgdwKeyLastTime = (unsigned char *)calloc(rgdwKeyCount, sizeof(unsigned char));
    return 0;
}

void DRIVER_DeInit_Event(void)
{
    free(rgdwKeyLastTime);
    rgdwKeyLastTime = NULL;
}
