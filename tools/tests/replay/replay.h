#ifndef REPLAY_H
#define REPLAY_H

// Shared state of the deterministic replay drivers.

#include <stdint.h>

// Virtual time starts away from zero so code comparing against 0 behaves normally.
#define REPLAY_CLOCK_START 0x00100000u

// Advance the virtual clock (simulated work between presents).
void REPLAY_AdvanceClock(uint32_t ms);

// Number of frames presented so far; scripted input is keyed by it.
uint32_t REPLAY_GetPresentCount(void);

// Canonical hash of the gameplay state (no pointers, no padding).
uint64_t REPLAY_StateHash(void);

// FNV-1a hashing helpers.
#define REPLAY_FNV_OFFSET 0xcbf29ce484222325ull
uint64_t REPLAY_HashBytes(uint64_t hash, const void *data, uint32_t size);

#endif
