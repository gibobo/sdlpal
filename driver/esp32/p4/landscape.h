// SDLPAL P4 landscape geometry; GPL-3.0-or-later.
#pragma once
#include <stdint.h>
namespace p4 {
constexpr int ScreenW = 480, ScreenH = 320;
constexpr int GameW = 320, GameH = 200, GameTop = 10, GameBottom = 310;
constexpr int Rotation = 1; // Unmirrored landscape, USB connectors on the right.
constexpr int SliderLeft = 72, SliderRight = 370, SliderY = 240;
constexpr int JoyDeadzone = 14, JoyCenterX = 80, JoyCenterY = 240;
constexpr int VolumeMax = 5, DefaultVolume = 1;
inline int volumeGainHalfDb(int level) {
    // Same safe amplitude curve as 20/40/60/80/100 percent, in codec 0.5 dB steps.
    constexpr int gains[] = {-191, -40, -28, -21, -16, -12};
    return gains[level < 0 ? 0 : (level > VolumeMax ? VolumeMax : level)];
}
struct Point { int x, y, id; };
struct Rect {
    int x, y, w, h;
    bool contains(int px, int py) const {
        return px >= x && py >= y && px < x + w && py < y + h;
    }
};
enum Zone { LeftTop, MiddleTop, RightTop, LeftBottom, MiddleBottom, RightBottom };
constexpr Rect Zones[] = {{0,0,160,160}, {160,0,160,160}, {320,0,160,160},
                         {0,160,160,160}, {160,160,160,160}, {320,160,160,160}};
inline int zoneAt(int x,int y) { return x/160 + (y/160)*3; }
constexpr Rect Toolbar{0, 196, 480, 124};
constexpr Rect CenterMarkerBounds{JoyCenterX-JoyDeadzone-1,JoyCenterY-JoyDeadzone-1,
                                 JoyDeadzone*2+3,JoyDeadzone*2+3};
constexpr Rect Slider{8, 218, 424, 48};
constexpr Rect Tools[] = {{8,270,86,42}, {104,270,86,42},
    {200,270,86,42}, {296,270,86,42}, {392,270,80,42}};
inline Point landscapeTouch(int x, int y, int id) { return {y, 319-x, id}; }
// Boundaries use ceil so partial updates cover exactly the affected samples.
inline int scaledEdge(int source) { return (source * 3 + 1) / 2; }
Rect updateScene(uint16_t *scene, const uint8_t *frame, const uint16_t *palette,
                 unsigned x, unsigned y, unsigned w, unsigned h, bool padding);
constexpr int ChangeBandRows=8, MaxSceneChanges=(GameH+ChangeBandRows-1)/ChangeBandRows;
// Return only RGB565 pixels that actually changed, grouped into source bands.
// Insufficient output capacity falls back to one covering rectangle.
int updateSceneChanges(uint16_t *scene, const uint8_t *frame, const uint16_t *palette,
                       unsigned x, unsigned y, unsigned w, unsigned h, bool padding,
                       Rect *changes, int capacity);
// Output and scene use big-endian RGB565; zero in overlay means transparent.
void composeBand(uint16_t *out, const uint16_t *scene, const uint16_t *overlay,
                 Rect band, const Rect *overlayAreas = nullptr, int areaCount = 0);
}
