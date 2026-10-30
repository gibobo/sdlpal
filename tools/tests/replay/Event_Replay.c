// Replay event driver: feeds scripted key input, keyed by present index, and ends
// the run at a given present. Script lines (blank lines and '#' comments ignored):
//
//   seed <n>                  seed the random generator (default 1)
//   at <present> press <key>  tap a key: down now, up on the next poll
//   at <present> hold <key>   press a key and keep it down
//   at <present> release <key>
//   at <present> item <object> <count>   add items to the inventory
//   at <present> battle <team> <field>   start a battle like script 0x0007
//   at <present> cmd <command>  run a test command (src/testcmd.h)
//   end <present>             shut the game down once this present is reached
//
// Keys: up down left right menu search pgup pgdn home end repeat auto defend
// useitem throwitem flee force status. Environment: PAL_REPLAY_SCRIPT (required).
// The item and battle statements reach states that would otherwise need long
// scripted play; they run from the event poll, so issue them while walking.
#include "../../../src/driver.h"
#include "../../../src/global.h"
#include "../../../src/input.h"
#include "../../../src/testcmd.h"
#include "../../../src/util.h"
#include "../../../driver/DrvIf_internal.h"
#include "replay.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_EVENTS 4096
#define MAX_COMMANDS 64
#define MAX_COMMAND_LENGTH 64

typedef enum ReplayAction
{
    kReplayPress,
    kReplayHold,
    kReplayRelease,
    kReplayItem,
    kReplayBattle,
    kReplayCommand
} ReplayAction;

typedef struct ReplayEvent
{
    uint32_t present;
    ReplayAction action;
    PALKEY key;
    uint16_t rgwArg[2]; // item and battle numbers, or the index of a command
} ReplayEvent;

static ReplayEvent g_rgEvents[MAX_EVENTS];
static int g_nEvents = 0;
static char g_rgszCommands[MAX_COMMANDS][MAX_COMMAND_LENGTH];
static int g_nCommands = 0;
static int g_iNextEvent = 0;
static uint32_t g_dwEndPresent = 0;

// Stop on a malformed script: a silently skipped line would change the run.
static void ScriptError(const char *path, int line, const char *what)
{
    fprintf(stderr, "%s:%d: %s\n", path, line, what);
    exit(2);
}

// Map a key name to its PALKEY bit.
static PALKEY ParseKey(const char *name, const char *path, int line)
{
    PALKEY key = TESTCMD_KeyFromName(name);
    if (key == kKeyNone)
        ScriptError(path, line, "unknown key");
    return key;
}

// Read the scenario script named by PAL_REPLAY_SCRIPT and seed the generator.
int DRIVER_Init_Event(void)
{
    const char *path = getenv("PAL_REPLAY_SCRIPT");
    char text[256];
    uint32_t seed = 1;
    int line = 0;
    FILE *fp;

    if (path == NULL || (fp = fopen(path, "r")) == NULL)
    {
        fprintf(stderr, "PAL_REPLAY_SCRIPT must name a readable scenario script\n");
        exit(2);
    }
    while (fgets(text, sizeof(text), fp) != NULL)
    {
        char verb[16], action[16], key[16];
        unsigned long value;
        line++;
        if (sscanf(text, "%15s", verb) != 1 || verb[0] == '#')
            continue;
        if (strcmp(verb, "seed") == 0 && sscanf(text, "%*s %lu", &value) == 1)
            seed = (uint32_t)value;
        else if (strcmp(verb, "end") == 0 && sscanf(text, "%*s %lu", &value) == 1)
            g_dwEndPresent = (uint32_t)value;
        else if (strcmp(verb, "at") == 0 && sscanf(text, "%*s %lu %15s %15s", &value, action, key) == 3)
        {
            ReplayEvent *e;
            if (g_nEvents == MAX_EVENTS)
                ScriptError(path, line, "too many events");
            if (g_nEvents > 0 && value < g_rgEvents[g_nEvents - 1].present)
                ScriptError(path, line, "events must be in present order");
            e = &g_rgEvents[g_nEvents++];
            e->present = (uint32_t)value;
            if (strcmp(action, "cmd") == 0)
            {
                const char *command = strstr(text, " cmd ") + 5;
                if (g_nCommands == MAX_COMMANDS)
                    ScriptError(path, line, "too many commands");
                strncpy(g_rgszCommands[g_nCommands], command, MAX_COMMAND_LENGTH - 1);
                g_rgszCommands[g_nCommands][strcspn(g_rgszCommands[g_nCommands], "\r\n")] = '\0';
                e->action = kReplayCommand;
                e->rgwArg[0] = (uint16_t)g_nCommands++;
                continue;
            }
            if (strcmp(action, "item") == 0 || strcmp(action, "battle") == 0)
            {
                unsigned long a, b;
                if (sscanf(text, "%*s %*s %*s %lu %lu", &a, &b) != 2)
                    ScriptError(path, line, "expected two numbers");
                e->action = (action[0] == 'i') ? kReplayItem : kReplayBattle;
                e->rgwArg[0] = (uint16_t)a;
                e->rgwArg[1] = (uint16_t)b;
                continue;
            }
            e->key = ParseKey(key, path, line);
            if (strcmp(action, "press") == 0)
                e->action = kReplayPress;
            else if (strcmp(action, "hold") == 0)
                e->action = kReplayHold;
            else if (strcmp(action, "release") == 0)
                e->action = kReplayRelease;
            else
                ScriptError(path, line, "unknown action");
        }
        else
            ScriptError(path, line, "unknown statement");
    }
    fclose(fp);
    if (g_dwEndPresent == 0)
        ScriptError(path, line, "missing 'end <present>'");

    UTIL_SetRandomSeed(seed);
    return 0;
}

void DRIVER_DeInit_Event(void)
{
}

// Apply the events due at the current present; end the run at the end present.
int DRIVER_Process_Events(void)
{
    uint32_t present = REPLAY_GetPresentCount();

    // Release a tapped key one poll after pressing it.
    TESTCMD_Poll();

    // Apply every event scheduled up to this present.
    while (g_iNextEvent < g_nEvents && g_rgEvents[g_iNextEvent].present <= present)
    {
        const ReplayEvent *e = &g_rgEvents[g_iNextEvent++];
        if (e->action == kReplayItem)
        {
            PAL_AddItemToInventory(e->rgwArg[0], e->rgwArg[1]);
            continue;
        }
        if (e->action == kReplayCommand)
        {
            if (!TESTCMD_Execute(g_rgszCommands[e->rgwArg[0]]))
            {
                fprintf(stderr, "unknown test command: %s\n", g_rgszCommands[e->rgwArg[0]]);
                exit(2);
            }
            break; // a battle command runs the whole battle, polling events inside
        }
        if (e->action == kReplayBattle)
        {
            // Runs the whole battle, polling events (and later script lines) inside.
            TESTCMD_StartBattle(e->rgwArg[0], e->rgwArg[1]);
            break;
        }
        if (e->action == kReplayRelease)
            PAL_KeyUp(e->key);
        else if (e->action == kReplayHold)
            PAL_KeyDown(e->key);
        else
        {
            TESTCMD_TapKey(e->key);
            break; // one tap per poll, so consecutive taps stay distinct
        }
    }

    return present >= g_dwEndPresent ? -1 : 0;
}
