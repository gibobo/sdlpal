// SDLPAL P4 touch gestures; GPL-3.0-or-later.
#pragma once
#include "landscape.h"
extern "C" {
#include "../../../src/input.h"
}
namespace p4 {
constexpr uint32_t LongPressMs = 600;
struct KeyChanges { uint32_t pressed, released; };
class KeyRepeater {
public:
    KeyChanges update(uint32_t keys, uint32_t now);
private:
    uint32_t held = 0, deadline = 0;
};
struct ControlState {
    bool toolbar = false, joystick = false;
    int volume = DefaultVolume;
    uint32_t keys = 0, commands = 0;
};
inline uint32_t visibleDirection(const ControlState &state) {
    return state.joystick ? state.keys & (kKeyUp|kKeyDown|kKeyLeft|kKeyRight) : 0;
}
inline Rect directionBounds(uint32_t key) {
    if (key==kKeyUp) return {JoyCenterX-7,JoyCenterY-48,15,15};
    if (key==kKeyDown) return {JoyCenterX-7,JoyCenterY+34,15,15};
    if (key==kKeyLeft) return {JoyCenterX-48,JoyCenterY-7,15,15};
    if (key==kKeyRight) return {JoyCenterX+34,JoyCenterY-7,15,15};
    return {0,0,0,0};
}
class Controls {
public:
    ControlState state;
    void update(const Point *points, int count, uint32_t now);
    void cancel(); // Transport fault: release keys without synthesizing a tap.
private:
    enum Kind { Swallow, Joystick, CancelTap, Toggle, OpenTools, Volume, Tool, Shortcut };
    struct Finger {
        int id = -1, x = 0, y = 0, lastX = 0, lastY = 0, tool = -1;
        uint32_t started = 0, direction = 0;
        int zone = -1;
        Rect area{0,0,0,0};
        Kind kind = Swallow;
        bool moved = false, longFired = false;
    } fingers[2];
    bool blocked = false;
    void begin(Finger &f, Point p, uint32_t now);
    void move(Finger &f, Point p);
    void release(Finger &f, uint32_t now);
    void toggle();
};
}
