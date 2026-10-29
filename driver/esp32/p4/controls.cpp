// SDLPAL P4 touch gestures; GPL-3.0-or-later.
#include "controls.h"
#include <stdlib.h>
namespace p4 {
static const uint32_t toolKeys[] = {kKeyPgUp, kKeyPgDn, kKeyStatus, kKeyAuto, 0};
static uint32_t shortcutKey(int zone, bool held) {
    if (zone==LeftTop) return held ? kKeyAuto : kKeyRepeat;
    if (zone==RightBottom) return held ? kKeyAuto : kKeySearch;
    return held ? kKeyDefend : kKeyForce;
}
KeyChanges KeyRepeater::update(uint32_t keys, uint32_t now) {
    KeyChanges events{keys & ~held, held & ~keys};
    if (keys!=held) deadline=now+350;
    else if (keys && (int32_t)(now-deadline)>=0) {
        events.pressed |= keys;
        deadline=now+100;
    }
    held=keys;
    return events;
}
void Controls::cancel() {
    for (auto &f : fingers) f.id = -1;
    state.keys = state.commands = 0;
    state.joystick = false;
    blocked = true;
}
void Controls::toggle() {
    state.toolbar = !state.toolbar;
    state.commands = 0;
    state.keys = 0;
    state.joystick = false;
    // Contacts already down must be lifted before the new layout can use them.
    for (auto &f : fingers) f.kind = Swallow;
}
void Controls::begin(Finger &f, Point p, uint32_t now) {
    f = Finger{};
    f.id=p.id; f.x=f.lastX=p.x; f.y=f.lastY=p.y; f.started=now;
    f.zone=zoneAt(p.x,p.y); f.area=Zones[f.zone];
    if (state.toolbar) {
        // Either the tools zone or the cancel zone closes the open panel.
        if (f.zone==MiddleTop || f.zone==RightTop) f.kind=Toggle;
        else if (Slider.contains(p.x,p.y)) {
            bool occupied=false;
            for (const auto &other : fingers)
                if (&other!=&f && other.id>=0 && other.kind==Volume) occupied=true;
            if (!occupied) f.kind=Volume;
        } else for (int i=0;i<5;++i) if (Tools[i].contains(p.x,p.y)) {
            f.kind=i==4 ? Toggle : Tool; f.tool=i; f.area=Tools[i]; break;
        }
    } else if (f.zone==MiddleTop) f.kind=OpenTools;
    else if (f.zone==RightTop) f.kind=CancelTap;
    else if (f.zone==RightBottom) f.kind=Shortcut;
    else if (f.zone==LeftTop || f.zone==MiddleBottom) {
        f.kind=Shortcut;
    } else if (f.zone==LeftBottom) {
        bool occupied=false;
        for (const auto &other : fingers)
            if (&other!=&f && other.id>=0 && other.kind==Joystick) occupied=true;
        if (!occupied) f.kind=Joystick;
    }
}
void Controls::move(Finger &f, Point p) {
    f.lastX=p.x; f.lastY=p.y;
    int dx=p.x-f.x, dy=p.y-f.y;
    if (dx*dx+dy*dy > JoyDeadzone*JoyDeadzone) f.moved=true;
    if (f.kind==Volume) {
        int x=p.x<SliderLeft?SliderLeft:(p.x>SliderRight?SliderRight:p.x);
        state.volume=((x-SliderLeft)*VolumeMax+(SliderRight-SliderLeft)/2)/(SliderRight-SliderLeft);
    }
}
void Controls::release(Finger &f, uint32_t now) {
    if (f.kind==OpenTools) {
        if (!f.moved && !f.longFired && f.area.contains(f.lastX,f.lastY) &&
            (uint32_t)(now-f.started)>=LongPressMs) toggle();
        f.id=-1;
        return;
    }
    if (f.kind==Shortcut) {
        // Also classify a release arriving after the threshold without a final
        // held sample. Never emit the short action after the long action fired.
        if (!f.moved && !f.longFired && f.area.contains(f.lastX,f.lastY))
            state.commands |= shortcutKey(f.zone,(uint32_t)(now-f.started)>=LongPressMs);
        f.id=-1;
        return;
    }
    bool tap=!f.moved && (uint32_t)(now-f.started)<=500;
    if (tap) {
        if (f.kind==CancelTap && f.area.contains(f.lastX,f.lastY)) state.commands |= kKeyMenu;
        else if (f.kind==Toggle && f.area.contains(f.lastX,f.lastY)) toggle();
        else if (f.kind==Tool && f.tool>=2 && Tools[f.tool].contains(f.lastX,f.lastY))
            state.commands |= toolKeys[f.tool];
    }
    f.id=-1;
}
void Controls::update(const Point *points, int count, uint32_t now) {
    state.commands=state.keys=0;
    state.joystick=false;
    if (count<0 || count>2 || (count>0 && !points) ||
        (count==2 && points[0].id==points[1].id)) { cancel(); return; }
    for (int i=0;i<count;++i)
        if (points[i].x<0 || points[i].x>=ScreenW || points[i].y<0 || points[i].y>=ScreenH ||
            points[i].id<0 || points[i].id>15) { cancel(); return; }
    if (blocked) { if (!count) blocked=false; return; }
    for (auto &f : fingers) if (f.id>=0) {
        bool seen=false;
        for (int i=0;i<count;++i) if (points[i].id==f.id) { move(f,points[i]); seen=true; break; }
        if (!seen) release(f,now);
    }
    for (int i=0;i<count;++i) {
        bool known=false;
        for (const auto &f : fingers) if (f.id==points[i].id) known=true;
        if (!known) for (auto &f : fingers) if (f.id<0) {
            begin(f,points[i],now); move(f,points[i]); break;
        }
    }
    for (auto &f : fingers) if (f.id>=0) {
        if (f.kind==Joystick) {
            state.joystick=true;
            int dx=f.lastX-JoyCenterX, dy=f.lastY-JoyCenterY;
            int distance2=dx*dx+dy*dy;
            uint32_t direction=0;
            if (distance2>JoyDeadzone*JoyDeadzone) {
                if (abs(dx)>=abs(dy)) direction=dx>0?kKeyRight:kKeyLeft;
                else direction=dy>0?kKeyDown:kKeyUp;
                // Hold the previous sector near a diagonal boundary to avoid chatter.
                bool sameSign=(f.direction==kKeyRight && dx>0) || (f.direction==kKeyLeft && dx<0) ||
                    (f.direction==kKeyDown && dy>0) || (f.direction==kKeyUp && dy<0);
                if (sameSign && abs(abs(dx)-abs(dy))<8) direction=f.direction;
            }
            f.direction=direction;
            state.keys |= direction;
        } else if (f.kind==OpenTools && !f.moved &&
                   f.area.contains(f.lastX,f.lastY) && (uint32_t)(now-f.started)>=LongPressMs) {
            toggle();
        } else if (f.kind==Shortcut && !f.moved && !f.longFired &&
                   f.area.contains(f.lastX,f.lastY) && (uint32_t)(now-f.started)>=LongPressMs) {
            state.commands |= shortcutKey(f.zone,true);
            f.longFired=true;
        }
        else if (f.kind==Tool && f.tool<2 && Tools[f.tool].contains(f.lastX,f.lastY))
            state.keys |= toolKeys[f.tool];
    }
}
}
