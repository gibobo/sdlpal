// Stand-ins for engine entry points referenced by the linked utility files.
#include "global.h"
#include "input.h"
#include "main.h"
#include <stdio.h>
#include <stdlib.h>

GLOBALVARS *gpGlobals = NULL;

// Abort the test run: the engine only shuts down here on a fatal error.
void PAL_Shutdown(int exit_code)
{
    fprintf(stderr, "PAL_Shutdown(%d) called\n", exit_code);
    exit(exit_code != 0 ? exit_code : 1);
}

// Tests have no event source.
void PAL_ProcessEvent(void)
{
}

// Tests have no key state.
void PAL_ClearKeyState(void)
{
}

// Tests have no key state.
PALKEY PAL_GetKeyInput(void)
{
    return kKeyNone;
}
