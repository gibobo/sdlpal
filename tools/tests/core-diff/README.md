# Core differential tests

`core_diff` runs development's core routines and the upstream implementation
(`origin/master`, pinned by `PAL_MASTER_REF`) on the complete game data and
requires identical results. The upstream sources are read from git at configure
time and compiled side by side under a `master_` prefix; nothing upstream is
vendored. File-static development functions are sliced from the working tree, so
the tests always exercise the checked-out code.

| Test | Compares |
|---|---|
| `yj2_chunks` | YJ2 decompression of every compressed MKF chunk and RNG frame (bytes, bounds, `PAL_MKFDecompressChunk` return value) |
| `rle_blits` | Every sprite, tile, UI sprite, item image and face, blitted plain, shadowed, color-shifted and mono-colored at interior and clipped positions; every FBP background. Each case compares the rows the bitmap covers; a full-canvas check after each bitmap catches writes outside them unless a later case overwrote the same rows |
| `xy_offset` | `PAL_XY_OFFSET` against per-coordinate addition, as upstream scripts compute it |
| `rng_stream` | `RandomLong` / `RandomFloat` sequences and generator state from identical seeds |
| `damage` | Base, physical and magic damage formulas over the game's value ranges |
| `rng_blit` | Every frame of every RNG movie applied in order |

The game data defaults to `Pal98rqptw/` (complete WIN95 set, gitignored);
override it with `PAL_TEST_RESOURCE_DIR`. File names are opened in lower case,
so a case-sensitive file system needs lower-case data files.

```powershell
cmake -B build -DPAL_BUILD_TESTS=ON
cmake --build build --config Release --target core_diff
ctest --test-dir build -C Release --output-on-failure
```

The project also builds on its own:

```powershell
cmake -S tools/tests/core-diff -B build/core-diff
cmake --build build/core-diff --config Release
ctest --test-dir build/core-diff -C Release --output-on-failure
```

To compare against a newer upstream, fetch it and set `PAL_MASTER_REF` to the new
commit; the configure output names the commit each run compares against.
