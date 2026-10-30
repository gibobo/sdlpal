# Waveshare ESP32-P4-WIFI6-Touch-LCD-3.5

This port is specific to the 3.5-inch ST7796 SPI / FT6336 I2C board.
The connected unit was identified as ESP32-P4 rev1.3, 16 MB flash, on COM3.
The build selects **ChipVariant=prev3**; do not select the rev3 silicon profile.

## Build and flash

Requires Arduino CLI, Espressif Arduino core 3.3.7, and GFX Library for Arduino
1.6.5 in `Documents/Arduino/libraries/GFX_Library_for_Arduino`.
The build script enables PSRAM and stages the correctly named Arduino sketch
under `build/arduino-stage/p4/sdlpal`, allowing this checkout to retain its current name.
The original drivers live in `driver/esp32/{p4,web,hw}` alongside the desktop
drivers. Only the generated sketch places ESP32 drivers inside its `src/`.
`build-esp32.ps1` prepares that sketch for all three backends; `build-p4.ps1`
keeps the existing P4 command unchanged. Desktop CMake selects its drivers
explicitly and excludes the ESP32 directory by construction.

```powershell
./tools/build-p4.ps1
./tools/build-p4.ps1 -Upload -Port COM3
```

Build outputs are in `build/p4`. The UART console runs at **921600 baud**.
No Wi-Fi or Bluetooth coprocessor firmware is required for the local controls.

## SD data

Use a FAT32 microSD card. The firmware never formats it automatically.
Copy all 15 `.mkf` files from `Pal98rqptw`, and these generated files from
`cache_data`, into the card root:

- `word.bin`
- `msg.bin`
- `unicode_font.bin`
- `unicode_font.dat`

`res_*.bin`, `res_*.dat` and `resource.dat` are not used by the current engine
configuration. Existing `*.rpg` saves are preserved; new saves are written to
the card root. The game uses pre-rendered 22050 Hz PCM under `/mus`. On this
board it also accepts the existing `/mus_pcm` folder after validating the
PCM1 header, sample rate and stereo layout. Live RIX is the fallback when
PCM tracks are unavailable.

The same USB-to-UART connector can provision the card without removing it:

```powershell
python -m pip install pyserial
python tools/p4-sd-upload.py --port COM3
```

The uploader resets the board, enters the five-second transfer window,
compares file sizes and CRC32, skips identical files, and uploads missing or
different files through temporary `.part` files. It reads back every upload
and checks CRC32 before restarting the game. It excludes save files.
Interrupted transfers can be retried by rerunning the command. The firmware
also stays in transfer mode when required game data is missing.

To inspect the card without changing its files:

```powershell
python tools/p4-sd-upload.py --check --port COM3
python tools/p4-sd-upload.py --list-only --port COM3 --report build/p4-sd-list.json
python tools/p4-monitor.py --port COM3 --seconds 60
```

The check/upload tools reset the board. Read-only checks are saved in
`build/p4-sd-report.json`. Close serial monitors before flashing or using
another serial tool.

## PWR sleep and resume

A short press/release of PWR requests Light-sleep. The display and backlight
are turned off, the amplifier is disabled, and the audio worker cooperatively
parks before I2S is stopped. Press PWR again to wake and resume the same game
state with the previous volume. A waking event is consumed with a 400 ms guard;
active touchscreen contacts and key repeat are cancelled to prevent stray
commands. Sleep duration, including peripheral restoration, is excluded from
the game clock so transitions and battle deadlines do not skip forward.

PWR now uses the AXP2101's latched short-press event (REG49 bit3), acknowledged
with write-one-to-clear. Long-press events (bit2) are rejected. GPIO49, routed
through a transistor on this board, is diagnostic only: its level cannot
request sleep or restore the display/audio. The PMIC IRQ net is not connected
to the MCU, so while sleeping the CPU checks the latched key event between
500 ms Light-sleep intervals; those internal timer wakes keep audio parked and
the display off. Touch does not wake the game. Flash/PSRAM supplies
remain enabled, and no AXP2101 regulator is turned off. The PMIC's roughly
six-second forced power-off remains unchanged; that is a power cut and does
not preserve live RAM state. This feature does not create an RPG save file.

Source references:
- `build/waveshare-p4-reference/schematic/ESP32-P4-WIFI6-Touch-LCD-3.5-schematic.pdf`
- https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32p4/api-reference/system/sleep_modes.html

The exact UART maintenance command `P4SLEEPTEST [milliseconds]` runs real
Light-sleep with timer wake (100..30000 ms, default 2000). Normal PWR sleep
resumes only on a confirmed PMIC short press, or on a PMIC communication error
(three failed polls) to avoid becoming unresponsive. Each resume reports the SDK result, wake cause, excluded
time, game-clock delta and a CRC check of the retained PSRAM scene. Automated
checks can run without pressing the physical button:

```powershell
python tools/p4-sleep-test.py --port COM3 --reset --cycles 3
python tools/p4-sleep-test.py --port COM3 --cycles 1 --sleep-ms 8000
```

Close other serial monitors first. `--reset` restarts the game; omit it to test
while retaining the current game context. The log defaults to
`build/p4-sleep-test.log`. The user confirmed physical PWR sleep/wake restores the game, touch and sound.
Current consumption has not been measured; host key-event/clock tests alone do
not prove hardware retention or wake behavior.

Battery-mode diagnosis: bounded append-only `/p4-power.log` on SD records boot
reset reasons, sleep/resume, GPIO49 level, PMIC status/protection IRQ flags,
battery percentage and battery voltage. Appending stops at 128 KiB without
truncating earlier evidence. Send `P4POWERLOG` at 921600 baud after game startup
to retrieve the file without resetting the game; `P4POWER` records an immediate
status sample. Reconnecting USB changes the supply state, but earlier SD log
entries remain available. USB tests alone cannot establish the cause of the
reported battery-only cold-start issue.

```powershell
python tools/p4-power-log.py --port COM3 --output build/p4-power-log.txt
```

The 2026-10-28 PMIC-key revision passed 4,741 native assertions and was flashed
with verified hashes. Three 2-second tests returned ESP_OK, timer wake cause 4,
PSRAM=OK and game_delta=0; PCM/audio initialized normally. SD diagnostic log
retrieval succeeded without reset. USB telemetry was 89%, charging, about
4151 mV. On 2026-10-28 the user confirmed the battery-only physical test was
normal: PWR sleep/resume, no touchscreen wake, and no recurring black screen
after the requested power-off / approximately 10-minute wait / restart test.
This confirms the observed behavior is resolved; the original electrical
cause was not measured. The earlier GPIO-only validation
below documents the previous implementation, not this revised wake policy.

### Power optimization stages (2026-10-28)

1. Retain the running game in Light-sleep. Park the audio worker, mute and disable
   the amplifier, suspend ES8311 analog blocks and clocks, and stop I2S. Verify
   the codec power/clock registers after suspend. On resume initialize the codec
   fully and reapply the saved volume before enabling the amplifier. The LCD's
   existing `displayOff()` sends SLPIN, in addition to turning off its backlight.
   PMIC key polling now uses 500 ms intervals (2 rather than 5 wakes per second);
   key detection can take up to 0.5 seconds plus peripheral restoration.
   This does not reduce CPU clocks while the game is running.
2. Measure battery-side current in normal play and sleep, then evaluate touch
   hibernation/reset recovery and independently controllable unused peripherals.
   Keep PSRAM, Flash and SD state until their retention/wake requirements have
   been established. Touch and further peripheral shutdown are not enabled yet.
3. Evaluate PMIC monitoring by the low-power core, subject to the actual I2C
   pin/controller routing and bus ownership. Do not restore unvalidated GPIO49
   wake or assume that the unconnected PMIC IRQ net is available to the CPU.

The green power LED follows the board's hardware enable circuit, not a spare
software LED GPIO. It remains on during state-retaining sleep. Current savings
and battery endurance are unmeasured; register readback confirms a programmed
state, not whole-board current. Deep-sleep would require a separate game-state
save/restore design to preserve the live game.

The codec suspend sequence follows
[Espressif's ES8311 driver](https://github.com/espressif/esp-adf/blob/release/v2.x/components/esp_codec_dev/device/es8311/es8311.c).

The first-stage image passed 4,869 native assertions and was flashed with hash
verification. Three 2-second sleep cycles read back `codec suspend=ESP_OK`,
resumed without an audio error, retained PSRAM and reported game_delta=0. This
validates the suspend/resume path; whole-board current still needs measurement.

### Landscape battle transfer budget

Full landscape transfer measured about 44 ms, exceeding the 40 ms battle-frame
budget even before scene drawing. `updateSceneChanges()` now compares final
RGB565 colors against the clean scene cache and sends only changed rectangles
grouped in eight-source-row bands. Identical frames send no scene pixels.
Palette fades and padded shake compare actual output colors, preserving their
semantics. Every game state and animation step still executes; only unchanged
LCD transfers are omitted. Touch overlays are composed over each changed region
and keep their independent restoration updates. No desktop/core gameplay code
was changed for this optimization.

Native checks cover sparse changes, palette-only changes, moving sprite trails,
consecutive shifted/padded scenes and small-output-capacity fallback. UART
`P4 render` summaries report average/max update time and transferred pixels out
of 144000; `P4 frame` reports work/total/target. Initial non-battle update times
were about 6–21 ms. During the user's battle test, render averages were about
5–8 ms (sample-window maxima around 14–18 ms after scene entry); sampled battle
frame work was 10–17 ms and total frame time held at the intended 40 ms. The
enemy-death battle fade measured 1159 ms against its 1152 ms target. Scene-entry
full transfers still peaked around 47 ms. On 2026-10-28 the user confirmed
normal physical attack speed, correct rendering without trails/missing graphics,
and normal music/touch after PWR sleep/resume. Whole-board sleep current remains
unmeasured; functional verification does not quantify battery endurance.

## Controls and wiring

Hold the device in landscape with the USB connectors on the right. The game
keeps its 320x200 logical resolution, displayed at 480x300 with nearest-neighbor
1.5x scaling on both axes. Black bars are 10 pixels at the top and bottom.
Nothing is cropped. The LCD uses rotation 1 and touch coordinates transform
from portrait `(x,y)` to landscape `(y,319-x)`.

Controls are hidden overlays in six equal 160x160 zones. Screen coordinates
include the 10-pixel top/bottom bars:

| | Left (x=0..159) | Middle (x=160..319) | Right (x=320..479) |
| --- | --- | --- | --- |
| Upper (y=0..159) | Tap R / hold A | Hold to open Tools (no icon) | Cancel / game menu |
| Lower (y=160..319) | Fixed D-pad | Tap F / hold D | Tap confirm / hold AUTO |

There are no press labels, button popups, or persistent tools icon. Tapping
the hidden middle-upper zone does nothing; holding it for 600 ms opens Tools.
When Tools is open, tapping that zone closes it. The fixed D-pad center is
`(80,240)` with a 14-pixel neutral zone: a cardinal sector press sends its
direction immediately. A fixed center circle with a 14-pixel radius appears only while a touch owns
the lower-left joystick zone. It is hidden when no joystick finger is down.
Only the currently active direction arrow is shown around it; there is no
outer range circle, movable knob, or inactive arrow. Neutral and release
remove the arrow. Neutral keeps the center circle while that finger is down;
release hides both, restoring the current game pixels. Other touch zones
do not display the center marker.

Direction keys repeat after 350 ms, then every 100 ms. Cancel
sends one command on a stationary release within 500 ms. Hidden shortcut zones
use a separate **600 ms** hold threshold:

- Upper left: release before 600 ms sends R (`kKeyRepeat`, previous action).
  Reaching 600 ms sends A (`kKeyAuto`, auto-attack) once.
- Lower middle: release before 600 ms sends F (`kKeyForce`, the original game's
  automatic magic selection shortcut). Reaching 600 ms sends D (`kKeyDefend`)
  once. This preserves the game's original F behavior, including its fallback.
- Lower right: release before 600 ms confirms (`kKeySearch`); reaching 600 ms
  sends AUTO (`kKeyAuto`) once, without confirming when released.
- Long holds do not repeat and never add the short action on release.
  Release at/after the threshold is classified as long even if the controller
  did not provide a final held sample. Significant drags cancel the action.
- Contacts retain their original zone, so crossing between zones does not
  activate a different shortcut. Tools transitions discard pending shortcuts.

Two fingers can operate movement and confirmation independently. The tools
panel captures touches, with page up/down, status, auto-attack, CLOSE, and a
six-detent volume slider. Its status row shows remaining battery percentage,
USB/battery supply and charging/full status. Missing batteries, unavailable
percentages and I2C read failures are shown explicitly. AXP2101 registers 00,
01 and A4 are read without changing the PMIC configuration, at opening and
every two seconds while the panel is open. Register meanings follow the
[AXP2101 datasheet](https://files.waveshare.com/wiki/common/X-power-AXP2101_SWcharge_V1.0.pdf).
The middle-upper tools zone or right-upper cancel
zone closes it without sending an underlying game command. A layout change
releases contacts already held and requires them to be lifted before reuse.
A sustained I2C read failure similarly releases held keys without making a tap.

The hold-only Tools / lower-right AUTO revision passed 4,350 native assertions
and was flashed to COM3 with verified hashes. Boot telemetry reported
`BAT 88% | USB power | Charging`, then audio/PCM and `PAL GAME READY` initialized
successfully. Physical long-press interaction and status-row layout await user
verification.

Volume has six levels, **0 through 5**, and defaults to **1** after reboot.
Level 0 mutes the codec; levels 1..5 retain the previously calibrated amplitude
curve at 20/40/60/80/100 percent, capped at -6 dB to avoid clipping:

| Level | Codec gain |
| --- | --- |
| 0 | Muted |
| 1 (default) | -20 dB |
| 2 | -14 dB |
| 3 | -10.5 dB |
| 4 | -8 dB |
| 5 | -6 dB |

A slider touch snaps to the nearest level and captures its finger through
release, even outside the panel. A second finger cannot take over the same
slider, but may operate another toolbar control.

The toolbar covers the lower part of the game while open; it does not pause
simulation. The clean RGB565 scene is separate from the transparent control
layer, so hiding controls restores current pixels without trails. Only affected
UI regions are redrawn. The ES8311 gain register scale is documented at:
https://files.waveshare.com/wiki/common/ES8311.DS.pdf

| Peripheral | GPIO |
| --- | --- |
| LCD MOSI / CLK / CS / DC / RESET / backlight | 20 / 21 / 23 / 26 / 27 / 28 |
| Touch + ES8311 I2C SDA / SCL | 7 / 8 |
| FT6336 reset / interrupt | 29 / 50 |
| microSD CLK / CMD / D0 / D1 / D2 / D3 | 43 / 44 / 39 / 40 / 41 / 42 |
| ES8311 MCLK / BCLK / WS / MCU DOUT | 13 / 12 / 10 / 9 |
| Speaker amplifier enable | 53 |

Audio uses 22050 Hz, 16-bit stereo I2S, with an adjustable codec volume
(level 1 of 5 at boot).
The display uses unmirrored landscape rotation and 80 MHz SPI mode 3.
SPI transactions must be enabled: Arduino_HWSPI applies the clock and mode
in `beginTransaction()`. With transactions disabled it remains at 1 MHz and
takes about one second per 320x200 frame. Pixel colors are pre-swapped to
wire byte order and sent with the bulk-byte bitmap API.
Audio runs on core 0; the Arduino/game task runs on core 1. SDMMC uses the
P4's internal LDO4 IO supply and a 32-file limit.
The Arduino loop calls `PAL_StartFrame()` directly because the engine already
handles frame timing and input polling. On P4, logic and display time are
subtracted from the 100 ms frame budget instead of adding a fixed 100 ms
after rendering. Every 2 s `P4 frame` reports the work, total and target time
of one paced frame; work exceeding the budget is reported rather than hidden by
extra delay. Frames that waited inside, such as one that ran a menu or a whole
battle, are not paced frames and are not reported.

Battle frames use the same budget rule at their original 40 ms (25 FPS)
timing. Ordinary attacks, hit reactions, the main battle loop, and magic
animations subtract their rendering time from their original frame duration.
Magic-specific speeds remain unchanged. In profiling builds PALPROF `attack_us`
times a player's whole normal attack animation (nine battle frames, nominally
360 ms, plus resource/audio work).

Screen shaking uses the same source/destination rules as the SDL driver:
for padded updates, source row zero moves to destination row `roi_y`.
The uncovered strip is filled black while the image itself shifts. Ordinary
partial updates continue to use their absolute source coordinates.

Fades use an accumulated transition deadline on P4. Rendering and calculation
time count toward the original duration. Overdue intermediate presentations
are omitted for fast 8-16 ms effects, while all palette/blending/scene state
updates still run and the final state is presented. This covers palette and
color fades, scene fades, screen blending, battle entry, title completion,
and ending picture fades. The shared core paces transitions this way on every
platform; on the desktop presenting is fast, so steps are rarely skipped.
`P4 transition` logs actual/target duration and the numbers of shown/skipped
intermediate steps.

During battle scene fades, the auto-attack indicator is redrawn at its normal
color on every presented frame. Its drawing helper only renders the label;
it does not process input, select targets, or commit battle actions.

## UART test control

Profiling builds (`tools/build-esp32.ps1 -Backend p4 -Profiling`) also take
the test commands of `src/testcmd.h` on the UART, one per line, after
`PAL GAME READY`:

| Command | Effect |
| --- | --- |
| `key press\|hold\|release <name>` | Tap, hold or release a key (`search`, `menu`, `left`, `force`, ...) |
| `bkey press\|hold\|release <name>` | The same during a battle; after it, answered `PALCMD skip` and dropped |
| `state` | Print `PALSTATE` with scene, map position, battle flag, frame, cash and party HP |
| `saves` | Print `PALSAVES` with the saved times of slots 1-5 (0 = empty) |
| `item <object> <count>` | Add items to the inventory |
| `battle <team> <field>` | Run a whole battle, then drop keys still pressed; prints `PALCMD done battle result=<n>` |
| `scene <number>` | Enter another scene, like script 0x0059 |
| `load <slot>` | Reload a saved game on the next frame |
| `save <slot>` | Save to an empty slot 1-5 or to test slot 6-9 |

Each command answers `PALCMD ok <command>` or `PALCMD error <reason>: <command>`.
Saving never overwrites a saved game in slots 1-5; the menus never show slots
6-9, so test saves leave the player's games alone.

`tools/p4-drive.py` plays a fixed route with these commands and logs every UART
line with host timestamps: reset, load the slot with the most saves, walk,
scroll the item list, save to test slot 6, three battles (attacks, Repeat,
magic and an item), then load test slot 6 and check that it restored the
scene, position, cash and HP it was saved with. Battle keys go through `bkey`,
so a key sent as a battle ends cannot open a menu on the map, and the route
waits for frames to advance between steps. The PALPROF windows in the log then
compare directly between builds.

```powershell
python tools/p4-drive.py --port COM3 --log build/p4-drive.log
```

## Performance measurements (2026-10-30)

Measured with profiling builds and `tools/p4-drive.py` on the same route and
save (slot 1, scene 280); "before" is the core with buffered stream I/O, "after"
adds the sector transfers and the glyph preload below.

=== SD card transfers — read before changing src/util_io.c ===
The P4's SD card driver moves many sectors per command only when a transfer
starts on a sector boundary of the file *and* its buffer is 128-byte aligned
(PSRAM included); everything else goes one sector per command. The boot
`PALPROF readbench` (512 KB of gop.mkf) measured:

| Way | Speed |
| --- | --- |
| stream, default 128-byte buffer | 1.12 MB/s |
| stream, 32 KB buffer (PSRAM or DMA memory) | 1.23 MB/s |
| stream, unbuffered (newlib reads it byte by byte) | 0.07 MB/s |
| `read()` from an odd offset into PSRAM | 1.23 MB/s |
| `read()` from a sector boundary through a 32 KB DMA stage | 4.1 MB/s |
| `read()` from a sector boundary into aligned PSRAM | 4.4 MB/s |
| `UTIL_fread` (now) from an odd offset | 3.9 MB/s |

`UTIL_fread`/`UTIL_fwrite` therefore pass transfers of 4 KB or more to the file
descriptor in pieces that end on sector boundaries, through an aligned PSRAM
stage. Never make the game's streams unbuffered. The hw and web boards keep
plain stream transfers and read glyphs on first use until they are measured.
================================================================

| Item | Before | After |
| --- | --- | --- |
| First load of a save into the game | 807 ms | 351 ms |
| Reading the 176 KB save file | 153-180 ms | 44-48 ms |
| Loading a save again (chunks cached) | 187 ms | 55 ms |
| Saving | 1104 ms | 520 ms |
| Sound effect, first use (max / average) | 225 / 27 ms | 59 / 9 ms |
| Normal attack, 4 samples (target 360 ms) | 366-406 ms, average 376 | 362-378 ms, average 369 |
| First item-menu frame | 195 ms | 51 ms |
| Glyphs read from the SD card | 235 | 0 |
| Text tables at boot | 256 ms | 84 ms |
| Font at boot | 12 ms | 415 ms (reads all 1.7 MB of glyphs) |
| Boot to `PAL GAME READY` (profiling build) | 10.1 s | 10.6 s |

Unchanged and not limiting: map scene drawing averages 2.6 ms (max 3.1 ms) and
the battle scene 1.0 ms (max 2.6 ms); full-screen display transfer (42-46 ms)
dominates frame time, so internal-RAM placement of the frame buffers was not
pursued. Waiting for the audio lock in `AUDIO_PlaySound` stays under 2 ms. YJ2
decompression averages 5.5 ms (max 30 ms) and runs only while loading.
Fades remain display-bound at 600-660 ms for a 600 ms target (12-23 of 60 steps
shown), screen switches take 364 ms (360 ms) and battle fades 1146-1160 ms
(1152 ms). Internal RAM stays at about 355 KB free; free PSRAM bottoms out at
27.8 MB (largest block 27.3 MB) with the chunk cache and the glyphs loaded.

## Verification on the connected board

The integrated `driver/esp32/` layout passed the P4 Arduino build and desktop
`palgame_dummy` build. Original and staged relative header paths were checked.

- Firmware flash completed with hash verification.
- 32 MB PSRAM, FT6336 at 0x38, ES8311, and the 968 MB card initialized.
- All 19 required files (48,836,432 bytes) matched the host CRC32.
- The card contains five saves and 86 pre-rendered 22050 Hz music files.
  Existing saves and data were retained.
- The PCM player reported that pre-rendered music is available.
- A full-frame SPI benchmark improved from 1,033,218 us to 17,635 us.
  This measures display transfer, not overall gameplay frame rate.
- Game initialization reached `PAL GAME READY`. The user confirmed normal
  display orientation, speed, and touch operation on the physical board.
- Live UART monitoring received UP/DOWN/LEFT/RIGHT virtual-button events.
- The earlier continuous volume slider was flashed and tested: UART recorded 0%
  followed by nonzero volume updates without codec errors, and the user
  confirmed both mute and volume adjustment work normally.
- After calibrating the gain curve, the user confirmed the low end is audible
  and the high end no longer clips.
- Battle playback measured 26-27 ms of work and 40 ms total per animation
  frame. Repeated normal attacks measured 381-401 ms including sound/resource
  work, versus the nine-frame animation budget of 360 ms.
  The user confirmed normal attack speed is now correct.
- The desktop `palgame_dummy` target also built successfully after the shared
  battle timing change; its original delay behavior is retained.
- The P4 framebuffer passed 800 comparisons against SDL's source-advancement
  and padding rules, covering vertical shake offsets and partial updates.
- The user confirmed vertical battle shaking is restored.
- The actual fade functions passed 64 P4 and 64 desktop regression cases for
  final palette state, zero delay, slow rendering, and timer wraparound.
- On-board fades took 625-630 ms against their original 600 ms target;
  battle entry took 1,175 ms against 1,152 ms, and screen switching took
  381 ms against 360 ms.
- The auto-attack fade overlay passed eight P4 and eight desktop cases,
  verifying constant label color on every visible frame and a single normal
  battle UI update at the end of the fade.
  The firmware was flashed with hash verification, and the user confirmed the
  label remains visible while defeated enemies fade out.
- With the frame budget fix, an observed walking frame took 23 ms of work
  and 100 ms total. The user confirmed walking speed is now close to PC.
  Scene transitions and scripts can take longer than a normal walking frame.

Official board documentation:
https://docs.waveshare.net/ESP32-P4-WIFI6-Touch-LCD-3.5/

The original flash backup from this session is `build/p4-original-flash.bin`.
It includes device configuration and should be kept locally.

## Platform source boundaries

`driver/esp32/p4/timing.c` owns frame budgets, transition deadlines,
late-presentation skipping and timing diagnostics. `platform.c` owns stopping
the audio worker before shared cleanup, the Arduino restart path, and validated
`mus_pcm` fallback files. `board.cpp` continues to own the hardware and controls.

`src/util.c` dispatches to these modules under the P4 build guard. Core game
loops use portable utility calls; desktop/web/hw retain the existing post-work
delays. Transition blending and final-state updates remain in the core, as does
the shared auto-attack overlay fix. The Arduino staging script adjusts relative
header paths only in generated copies, never in the source tree.

Source refactor validation (2026-10-26): desktop `palgame_dummy` and P4
firmware builds passed. The P4 image uses 675,340 bytes of flash and 47,972 bytes
of static RAM. Regression checks passed: 128 actual fade cases, 16 auto-attack
overlay cases, 28 frame-budget/wraparound cases, and 11 PCM fallback/header cases.
These are host regression checks; hardware gameplay confirmation remains the
previous user-verified baseline. The refactored firmware was subsequently flashed
to COM3 with hash verification. Boot logs confirmed display, touch, SD, PCM music
and audio initialization, followed by PAL GAME READY; a fade-out took 624 ms
against its 600 ms target. Full gameplay validation is pending user confirmation.

## Landscape regression tests

`tools/tests/p4-landscape` compiles the actual portable renderer and gesture
state machine on the host. Run the CMake/CTest commands in that folder's README.
The tests include 818 SDL/scaling/ROI cases and 1,920 optimized-overlay coverage
comparisons (4,291 assertions total, including power-key/clock checks). They cover SDL-equivalent shake source
advancement, scaled partial-update boundaries, 10-pixel letterboxing, overlay restoration, all touch coordinates,
fixed-center single direction taps, delayed key repeat, six-zone ownership,
simultaneous movement/confirm, tool and six-level slider capture,
transport failures and timer wraparound. Physical USB orientation and the final
touch alignment must additionally be checked on the board.

Landscape implementation validation (2026-10-26): desktop `palgame_dummy` and
P4 firmware builds passed, as did the host checks above. The first landscape
image was flashed with hash verification and reached PAL GAME READY with PCM
music. Its full-screen benchmark took 53,404 us. The renderer was then optimized
to expand source pairs into three output pixels, reuse repeated rows before
compositing, and scan overlay pixels only inside active control bounds.
The optimized image was subsequently flashed on COM3 with hash verification
and reached PAL GAME READY with touch and PCM music initialized. A 480x320
full-screen benchmark took 44,100 us (2,111 us composition and 41,754 us SPI);
a fade-out took 689 ms against its 600 ms target. Display transfer still
exceeds the 40 ms battle-frame budget, so some battle slowdown remains possible.
Physical USB orientation, touch alignment and gameplay are awaiting user testing.

Six-zone controls validation (2026-10-26): the final P4 build and 4,183 host
assertions passed, including actual key-repeat timing, immediate cardinal
presses, neutral-center release without cancel, six-zone ownership and all
volume detents. The image was flashed on COM3 with hash verification and booted
to PAL GAME READY. The codec reported volume=1/5, gain=-20.0 dB; PCM music was
available. The full-screen benchmark was 44,091 us (2,117 us compose, 41,740 us
SPI). Physical single-option selection and the revised layout await user testing.

Hidden shortcut revision validation (2026-10-26): 4,273 host assertions and the
P4 build passed. The revised image was flashed on COM3 with hash verification
and reached PAL GAME READY; PCM music and volume=1/5 (-20.0 dB) were confirmed
in the boot log. The full-screen benchmark was 43,990 us. The six-zone press
labels, persistent Tools icon, joystick circles/knob and inactive arrows were
removed. Only the active cardinal arrow and an explicitly opened toolbar are
composited. Physical R/A, F/D and the revised visuals await user confirmation.

Center-reference follow-up: the fixed 14-pixel center circle is visible while
Tools is closed, including neutral/no-touch state. The outer range circle and
movable knob remain hidden, and only the active direction arrow is added. The
existing overlay-comparison tests now include the center marker's bounds and
its replacement by the Tools panel. Tests and the P4 build passed; the image
was flashed on COM3 with hash verification.

Center visibility revision: the reference circle now follows joystick contact
ownership. A neutral-center press shows it immediately; releasing, opening
Tools, or cancelling a failed touch read removes it even when no direction
arrow was ever active. The range outer circle remains hidden.

PWR sleep validation (2026-10-26): desktop and P4 builds passed, along with
4,291 host assertions. After flash hash verification, three consecutive real
two-second Light-sleep cycles and one eight-second cycle returned ESP_OK with
timer wake cause 4, retained PSRAM CRC and game_delta=0 ms. Excluded pause
durations were approximately 2,347-2,356 ms and 8,353 ms, including peripheral
restoration. Volume remained 1/5. The user then tested the physical PWR key and
confirmed that the original game screen, touch and sound resumed normally.
UART additionally recorded three physical-key GPIO wakes (cause 7), all with
ESP_OK, game_delta=0 ms and PSRAM=OK; pauses included 1,584, 8,786 and 4,454 ms.
