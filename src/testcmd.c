#include "testcmd.h"
#include "battle.h"
#include "global.h"
#include "palcommon.h"
#include "res.h"
#include "uigame.h"
#include "util.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

// The game's menus offer save slots 1 to 5; slots above them, up to this one,
// are test slots that the menus never show.
#define TESTCMD_SAVE_SLOTS 5
#define TESTCMD_LAST_SLOT 9

static const struct
{
    const char *name;
    PALKEY key;
} g_rgKeyNames[] = {
    {"up", kKeyUp},           {"down", kKeyDown},       {"left", kKeyLeft},   {"right", kKeyRight},
    {"menu", kKeyMenu},       {"search", kKeySearch},   {"pgup", kKeyPgUp},   {"pgdn", kKeyPgDn},
    {"home", kKeyHome},       {"end", kKeyEnd},         {"repeat", kKeyRepeat}, {"auto", kKeyAuto},
    {"defend", kKeyDefend},   {"useitem", kKeyUseItem}, {"throwitem", kKeyThrowItem},
    {"flee", kKeyFlee},       {"force", kKeyForce},     {"status", kKeyStatus},
};

static PALKEY g_TapToRelease = kKeyNone;

PALKEY TESTCMD_KeyFromName(const char *name)
{
    size_t i;
    for (i = 0; i < sizeof(g_rgKeyNames) / sizeof(g_rgKeyNames[0]); i++)
    {
        if (strcmp(name, g_rgKeyNames[i].name) == 0)
            return g_rgKeyNames[i].key;
    }
    return kKeyNone;
}

void TESTCMD_TapKey(PALKEY key)
{
    // Release an earlier tap first, so two taps stay two presses.
    TESTCMD_Poll();
    PAL_KeyDown(key);
    g_TapToRelease = key;
}

void TESTCMD_Poll(void)
{
    if (g_TapToRelease != kKeyNone)
    {
        PAL_KeyUp(g_TapToRelease);
        g_TapToRelease = kKeyNone;
    }
}

int TESTCMD_StartBattle(uint16_t wEnemyTeam, uint16_t wBattleField)
{
    int result;

    gpGlobals->wNumBattleField = wBattleField;
    result = PAL_StartBattle(wEnemyTeam, false);
    gpGlobals->fAutoBattle = false;
    return result;
}

// Print one result line for the host and flush it out at once.
static void Reply(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
    fflush(stdout);
}

// Print the scene, the party's map position, the battle flag and the party HP.
static void PrintState(void)
{
    int i;

    printf("PALSTATE scene=%u x=%d y=%d entering=%u battle=%u frame=%lu cash=%lu hp=",
           (unsigned)gpGlobals->wNumScene, PAL_X(gpGlobals->viewport) + PAL_X(gpGlobals->partyoffset),
           PAL_Y(gpGlobals->viewport) + PAL_Y(gpGlobals->partyoffset), (unsigned)gpGlobals->fEnteringScene,
           (unsigned)gpGlobals->fInBattle, (unsigned long)gpGlobals->dwFrameNum, (unsigned long)gpGlobals->dwCash);
    for (i = 0; gpGlobals->g.PlayerRoles != NULL && i <= gpGlobals->wMaxPartyMemberIndex; i++)
        printf("%s%u", i > 0 ? "," : "", (unsigned)gpGlobals->g.PlayerRoles->rgwHP[gpGlobals->rgParty[i].wPlayerRole]);
    Reply("");
}

// Print the saved times of each slot; 0 marks an empty slot.
static void PrintSaves(void)
{
    int i;

    printf("PALSAVES");
    for (i = 1; i <= TESTCMD_SAVE_SLOTS; i++)
        printf(" %d=%u", i, (unsigned)PAL_GetSavedTimes(i));
    Reply("");
}

int TESTCMD_Execute(const char *line)
{
    char verb[16], action[16], name[16];
    unsigned long a, b;
    PALKEY key;

    if (sscanf(line, "%15s", verb) != 1)
        return 0;

    if (strcmp(verb, "key") == 0 || strcmp(verb, "bkey") == 0)
    {
        // Tap, hold or release one key.
        if (sscanf(line, "%*s %15s %15s", action, name) != 2 || (key = TESTCMD_KeyFromName(name)) == kKeyNone)
        {
            Reply("PALCMD error expected '%s press|hold|release <name>': %s", verb, line);
            return 1;
        }
        // Drop a battle key once the battle is over, so it cannot reach the map.
        if (verb[0] == 'b' && !gpGlobals->fInBattle)
        {
            Reply("PALCMD skip %s", line);
            return 1;
        }
        if (strcmp(action, "press") == 0)
            TESTCMD_TapKey(key);
        else if (strcmp(action, "hold") == 0)
            PAL_KeyDown(key);
        else if (strcmp(action, "release") == 0)
            PAL_KeyUp(key);
        else
        {
            Reply("PALCMD error unknown key action: %s", line);
            return 1;
        }
        Reply("PALCMD ok %s", line);
    }
    else if (strcmp(verb, "state") == 0)
        PrintState();
    else if (strcmp(verb, "saves") == 0)
        PrintSaves();
    else if (strcmp(verb, "item") == 0)
    {
        if (sscanf(line, "%*s %lu %lu", &a, &b) != 2)
            Reply("PALCMD error expected 'item <object> <count>': %s", line);
        else
        {
            PAL_AddItemToInventory((uint16_t)a, (int)b);
            Reply("PALCMD ok %s", line);
        }
    }
    else if (strcmp(verb, "battle") == 0)
    {
        // Acknowledge first: the battle runs inside this call and polls events itself.
        if (sscanf(line, "%*s %lu %lu", &a, &b) != 2)
            Reply("PALCMD error expected 'battle <team> <field>': %s", line);
        else
        {
            int result;
            Reply("PALCMD ok %s", line);
            result = TESTCMD_StartBattle((uint16_t)a, (uint16_t)b);
            // Drop keys tapped as the battle ended, so the map does not act on them.
            PAL_ClearKeyState();
            Reply("PALCMD done battle result=%d", result);
        }
    }
    else if (strcmp(verb, "scene") == 0)
    {
        if (sscanf(line, "%*s %lu", &a) != 1 || a == 0 || a > MAX_SCENES)
            Reply("PALCMD error expected 'scene <1-%d>': %s", MAX_SCENES, line);
        else
        {
            gpGlobals->wNumScene = (uint16_t)a;
            gpGlobals->fEnteringScene = true;
            gpGlobals->wLayer = 0;
            PAL_SetLoadFlags(kLoadScene);
            Reply("PALCMD ok %s", line);
        }
    }
    else if (strcmp(verb, "load") == 0 || strcmp(verb, "save") == 0)
    {
        // Saving goes to a test slot or an empty slot, so the player's saved games stay intact.
        if (sscanf(line, "%*s %lu", &a) != 1 || a == 0 || a > TESTCMD_LAST_SLOT)
            Reply("PALCMD error expected '%s <1-%d>': %s", verb, TESTCMD_LAST_SLOT, line);
        else if (verb[0] == 'l')
        {
            PAL_ReloadInNextTick((uint8_t)a);
            Reply("PALCMD ok %s", line);
        }
        else if (a <= TESTCMD_SAVE_SLOTS && PAL_GetSavedTimes((int)a) != 0)
            Reply("PALCMD error slot %lu holds a saved game: %s", a, line);
        else
        {
            PAL_SaveGame((int32_t)a, 1);
            Reply("PALCMD ok %s", line);
        }
    }
    else
        return 0;
    return 1;
}
