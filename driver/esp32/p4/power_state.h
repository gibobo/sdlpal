// SDLPAL P4 power-key policy and retained game clock; GPL-3.0-or-later.
#pragma once
#include <stdint.h>
#include <atomic>
namespace p4 {
class PmicPowerButton {
public:
    void arm(uint32_t now) {
        lastEvent=now;
        guarded=true;
    }
    // AXP2101 REG49: short-press bit3, long-press bit2. Call only after
    // acknowledging these W1C bits successfully. GPIO49 is not a key event.
    bool consume(uint8_t irq, uint32_t now) {
        if (guarded && (uint32_t)(now-lastEvent)<400) return false;
        if ((irq&0x0c)!=0x08) return false;
        lastEvent=now;
        guarded=true;
        return true;
    }
private:
    bool guarded=true;
    uint32_t lastEvent=0;
};
class GameClock {
public:
    uint32_t milliseconds(uint32_t raw) const { return raw-excluded.load(std::memory_order_relaxed); }
    void exclude(uint32_t elapsed) { excluded.fetch_add(elapsed,std::memory_order_relaxed); }
private:
    std::atomic<uint32_t> excluded{0};
};
}
