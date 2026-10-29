# Deterministic replay

`palreplay` runs the game headless on a virtual clock with scripted input and
records one line per presented frame:

```
<present> <virtual ms> <dwFrameNum> <state hash> <screen hash> <palette hash>
```

The state hash covers the game data scripts modify, the session state, the
battle state while a battle runs and the random generator state; pointers,
padding and the unused time-charging floats are excluded. Two runs of the same
scenario produce identical traces, so any change to state, screen or palette
shows up as a divergence from the scenario's baseline.

Time only advances when the game sleeps, plus `PAL_REPLAY_WORK_MS` virtual
milliseconds per present to simulate rendering work. Each scenario runs with 0
and 30 ms of work and has a baseline per setting (`baselines/<scenario>.w<ms>.txt`).

## Scenarios

| Scenario | Covers |
|---|---|
| `intro` | Splash, opening menu, new game, opening dialogs, a scene change, the item menu |
| `battle` | Loading a saved game (`Pal98rqptw/0.rpg` as slot 1), item descriptions, two battles including Repeat on the first turn |

Script statements (one per line, `#` starts a comment):

```
seed <n>
at <present> press|hold|release <key>
at <present> item <object> <count>
at <present> battle <enemy team> <battle field>
end <present>
```

Keys: `up down left right menu search pgup pgdn home end repeat auto defend
useitem throwitem flee force status`. Input is keyed by present index, which
performance changes do not move. `item` and `battle` reach states that would
otherwise take long scripted play; issue them while the party can walk.

## Running

```powershell
cmake -B build -DPAL_BUILD_TESTS=ON
cmake --build build --config Release --target palreplay replaycmp
ctest --test-dir build -C Release -R replay --output-on-failure
```

A failure names the first divergent present and which hashes changed. When a
change is meant to alter the game (a fix, a timing change), re-record the
baselines and explain the expected difference in the commit:

```powershell
$env:PAL_REPLAY_RECORD = "1"; ctest --test-dir build -C Release -R replay; Remove-Item Env:PAL_REPLAY_RECORD
```

## Writing scenarios

Run `palreplay` directly and save frames as PNG to see where the game is:

```powershell
$env:PAL_REPLAY_SCRIPT = "tools/tests/replay/scenarios/intro.txt"
$env:PAL_REPLAY_OUT = "intro.trace"
$env:PAL_REPLAY_DUMP = "300,900,1500"   # writes intro.trace.<present>.png
build/palreplay.exe
```

The replays use a private copy of the game data under the build directory and
reset the save slots before each run, so scenarios never touch `Pal98rqptw/`.

## Measuring

`palreplay` is built with `PAL_PROFILE` (see `src/profile.h`) and prints one
`PALPROF total ...` line when the run ends. The counters (file seeks and reads,
chunk reads, decompressions, allocations, late waits) depend only on the
scenario and the simulated work, so a change that should reduce them can quote
exact before and after numbers. The `_us` timers are real time, given as
count/total/max microseconds, and vary from run to run.

The `.cache0` and `.cache65536` tests rerun scenarios with the chunk cache
turned off and under constant eviction (`PAL_REPLAY_CACHE_BYTES`); they compare
against the same baselines, since the cache may only save reads.
