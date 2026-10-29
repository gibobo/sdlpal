# P4 landscape regression checks

The tests compile the actual portable P4 renderer and gesture state machine,
without Arduino or hardware. They compare scaled partial updates and shaking
against SDL source advancement, and exercise overlay restoration, touch
rotation, fixed-center direction taps, six-zone finger ownership, diagonal hysteresis,
dead zones, actual delayed key repetition, toolbar and six-level volume capture,
mutually exclusive R/A, F/D and confirm/AUTO tap/hold actions, hold-only Tools,
invalid reads, and timer
wraparound, PMIC short/long events, unrelated IRQ rejection, wake-event suppression and sleep-excluded game
time.

Changed-scene tests also cover zero transfers for identical frames, separated
dirty bands, moving sprites clearing their old position, palette-only changes,
consecutive ROI frames, shake frames in both directions and a covering-rectangle fallback when the
caller provides fewer output regions than required.

```powershell
cmake -S tools/tests/p4-landscape -B build/p4-landscape-tests
cmake --build build/p4-landscape-tests --config Release
ctest --test-dir build/p4-landscape-tests -C Release --output-on-failure
```

Physical USB orientation, mirrored output and touch calibration must also be
confirmed on the board.
