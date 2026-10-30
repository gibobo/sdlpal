#ifndef TESTCMD_H
#define TESTCMD_H

#include "input.h"
#include <stdint.h>

// Test commands that drive the game from lines of text: the boards read them
// from the serial port in profiling builds, and the replay tests share the key
// and battle helpers. Results are printed as "PALCMD", "PALSTATE" and
// "PALSAVES" lines for the controlling host.

#ifdef __cplusplus
extern "C" {
#endif

// The key with the given name ("search", "left", ...), or kKeyNone.
PALKEY TESTCMD_KeyFromName(const char *name);

// Press a key now and release it at the next TESTCMD_Poll, like one tap.
void TESTCMD_TapKey(PALKEY key);

// Release the key tapped since the previous poll; call once per event poll.
void TESTCMD_Poll(void);

// Start a battle against an enemy team on a battle field, like script 0x0007,
// and return its result when it ends.
int TESTCMD_StartBattle(uint16_t wEnemyTeam, uint16_t wBattleField);

/*++
  Run one command line; returns 0 if its verb is not a test command. Commands:

    key press|hold|release <name>   tap, hold down or release a key
    bkey press|hold|release <name>  the same during a battle; afterwards it is
                                    dropped and answered "PALCMD skip"
    state                           print scene, position, battle and party HP
    saves                           print the saved times of slots 1 to 5
    item <object> <count>           add items to the inventory
    battle <team> <field>           run a whole battle (from the event poll), then
                                    drop keys still pressed
    scene <number>                  enter another scene, like script 0x0059
    load <slot>                     reload a saved game (slot 1-9) on the next frame
    save <slot>                     save to an empty slot 1-5 or to test slot 6-9;
                                    never overwrites a saved game in slots 1-5
--*/
int TESTCMD_Execute(const char *line);

#ifdef __cplusplus
}
#endif

#endif
