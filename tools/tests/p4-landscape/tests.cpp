#include "../../../driver/esp32/p4/controls.h"
#include "../../../driver/esp32/p4/power_state.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <cmath>
using namespace p4;
static unsigned checks;
static void check(bool yes,const char *message) {
    ++checks;
    if (!yes) { std::fprintf(stderr,"FAIL: %s\n",message); std::exit(1); }
}
static uint16_t scene[GameW*GameH], reference[GameW*GameH];
static uint16_t overlay[ScreenW*ScreenH], screen[ScreenW*ScreenH], band[ScreenW*16];
static uint8_t frame[GameW*GameH];
static uint16_t palette[256];
static void present(Rect r) {
    for(int y=r.y;y<r.y+r.h;y+=16) {
        int n=std::min(16,r.y+r.h-y);
        composeBand(band,scene,overlay,{r.x,y,r.w,n});
        for(int row=0;row<n;++row)
            std::memcpy(screen+(y+row)*ScreenW+r.x,band+row*r.w,r.w*sizeof(uint16_t));
    }
}
static void referenceUpdate(const uint8_t *source,unsigned x,unsigned y,unsigned w,unsigned h,bool pad) {
    unsigned right=std::min(x+w,(unsigned)GameW), bottom=std::min(y+h,(unsigned)GameH);
    if(!w || !h || x>=GameW || y>=GameH) return;
    // Independent SDL source advancement: padded frames start at source row zero.
    const uint8_t *src=source;
    for(unsigned row=0;row<GameH;++row) {
        if(row>=y && row<bottom) {
            for(unsigned col=0;col<GameW;++col) {
                if(col>=x && col<right) reference[row*GameW+col]=palette[src[col]];
                else if(pad) reference[row*GameW+col]=0;
            }
            src+=GameW;
        } else if(pad) std::fill(reference+row*GameW,reference+(row+1)*GameW,0);
        else src+=GameW;
    }
}
static void renderCase(unsigned x,unsigned y,unsigned w,unsigned h,bool pad) {
    std::fill(scene,scene+GameW*GameH,0xbeef);
    std::copy(scene,scene+GameW*GameH,reference);
    present({0,0,ScreenW,ScreenH});
    referenceUpdate(frame,x,y,w,h,pad);
    Rect dirty=updateScene(scene,frame,palette,x,y,w,h,pad);
    check(std::memcmp(scene,reference,sizeof(scene))==0,"ROI/shake must match SDL");
    present(dirty);
    bool same=true;
    for(int dy=0;dy<ScreenH;++dy) for(int dx=0;dx<ScreenW;++dx) {
        uint16_t expected=0;
        if(dy>=10 && dy<310) {
            // Reference uses rational source footprints, independently of scaledEdge.
            int sx=(int)((double)dx/1.5), sy=(int)((double)(dy-10)/1.5);
            expected=reference[sy*GameW+sx];
        }
        if(screen[dy*ScreenW+dx]!=expected) same=false;
    }
    check(same,"scaled dirty rectangle, 10px bars and untouched pixels");
}
static void rendererTests() {
    for(int i=0;i<256;++i) palette[i]=(uint16_t)(i*127+1);
    for(int y=0;y<GameH;++y) for(int x=0;x<GameW;++x) frame[y*GameW+x]=(y*17+x*3)%256;
    unsigned cases=0;
    for(unsigned y=0;y<200;++y) for(int pad=0;pad<2;++pad) for(int part=0;part<2;++part) {
        renderCase(part?13:0,y,part?289:320,y==0&&pad?196:200-y,pad); ++cases;
    }
    const unsigned special[][4]={{1,1,1,1},{319,199,1,1},{0,0,1,1},{2,3,17,19},
        {319,199,65535,65535},{0,0,0,200},{0,0,320,0},{320,0,1,1},{0,200,1,1}};
    for(auto &r:special) for(bool pad:{false,true}) {renderCase(r[0],r[1],r[2],r[3],pad);++cases;}
    overlay[11*ScreenW+13]=0x1234;
    present({0,0,ScreenW,ScreenH});
    check(screen[11*ScreenW+13]==0x3412,"overlay RGB byte order");
    overlay[11*ScreenW+13]=0;
    present({13,11,1,1});
    check(screen[11*ScreenW+13]==scene[0*GameW+8],"hiding overlay restores original scene");
    for(int y=0;y<480;++y) for(int x=0;x<320;++x) {
        auto p=landscapeTouch(x,y,7);
        if(p.x!=y || p.y!=319-x || p.id!=7) check(false,"rotation inverse");
    }
    auto a=landscapeTouch(0,0,0), b=landscapeTouch(319,479,1);
    check(a.x==0&&a.y==319&&b.x==479&&b.y==0,"touch corner mapping");
    check(scaledEdge(320)==480 && scaledEdge(200)==300,"equal 1.5x scale on both axes");
    std::printf("PASS: %u SDL/scaling/ROI cases; overlay restoration and all touch coordinates\n",cases);
}
static void overlayAreaTests() {
    const Rect regions[]={Toolbar,directionBounds(kKeyUp),directionBounds(kKeyDown),directionBounds(kKeyLeft),directionBounds(kKeyRight),CenterMarkerBounds};
    static uint16_t expectedBand[ScreenW*16];
    for(int variant=0;variant<16;++variant) {
        Rect active[6]{}; int count=0;
        if(variant&1) active[count++]=regions[0];
        else active[count++]=regions[5];
        if(variant&2) active[count++]=regions[1];
        if(variant&4) active[count++]=regions[2];
        if(variant&8) { active[count++]=regions[3]; active[count++]=regions[4]; }
        std::fill(overlay,overlay+ScreenW*ScreenH,0);
        for(int i=0;i<count;++i) for(int y=active[i].y;y<active[i].y+active[i].h;++y)
            for(int x=active[i].x;x<active[i].x+active[i].w;++x)
                if((x+y)%3) overlay[y*ScreenW+x]=(uint16_t)(x+y+1);
        for(int y=0;y<ScreenH;y+=16) for(int x:{0,1,13,319,436,479}) {
            Rect r{x,y,ScreenW-x,16};
            composeBand(expectedBand,scene,overlay,r);
            composeBand(band,scene,overlay,r,active,count);
            check(std::memcmp(expectedBand,band,r.w*r.h*sizeof(uint16_t))==0,
                  "optimized overlay bounds, overlaps, clipped partial bands");
        }
    }
    std::printf("PASS: 1920 optimized overlay coverage comparisons\n");
}
static void changedRegionTests() {
    std::fill(overlay,overlay+ScreenW*ScreenH,0);
    std::fill(scene,scene+GameW*GameH,0);
    std::fill(reference,reference+GameW*GameH,0);
    std::fill(frame,frame+GameW*GameH,0);
    palette[0]=0; palette[1]=0x1234;
    present({0,0,ScreenW,ScreenH});
    Rect regions[MaxSceneChanges];
    check(updateSceneChanges(scene,frame,palette,0,0,GameW,GameH,false,regions,MaxSceneChanges)==0,
          "identical scene performs no SPI update");
    frame[0]=1; frame[199*GameW+319]=1;
    int n=updateSceneChanges(scene,frame,palette,0,0,GameW,GameH,false,regions,MaxSceneChanges);
    check(n==2,"distant changed pixels remain separate bands");
    unsigned pixels=0;
    for(int i=0;i<n;++i) { pixels+=regions[i].w*regions[i].h; present(regions[i]); }
    check(pixels==5,"sparse corner update sends five scaled pixels instead of full screen");
    check(screen[GameTop*ScreenW]==0x1234&&screen[309*ScreenW+479]==0x1234,"sparse corners appear correctly");
    palette[1]=0x5678;
    n=updateSceneChanges(scene,frame,palette,0,0,GameW,GameH,false,regions,MaxSceneChanges);
    check(n==2,"palette changes dirty even when indices are unchanged");
    for(int i=0;i<n;++i) present(regions[i]);
    check(screen[GameTop*ScreenW]==0x5678,"palette-only color changes are displayed");
    for(int step=0;step<60;++step) {
        for(int y=0;y<GameH;++y) for(int x=0;x<GameW;++x)
            frame[y*GameW+x]=(x>=step*3&&x<step*3+17&&y>=53&&y<81)?1:0;
        // Alternate normal, clipped ROI and both shake frames: moved down by a
        // padded region, and moved up by a padded region read from a later row.
        bool up=step%4==3;
        unsigned y=step%4==2?4:0, x=step%4==1?13:0;
        unsigned w=GameW-x,h=GameH-(up?4:y);
        bool pad=step%4>=2;
        const uint8_t *source=up?frame+4*GameW:frame;
        referenceUpdate(source,x,y,w,h,pad);
        n=updateSceneChanges(scene,source,palette,x,y,w,h,pad,regions,MaxSceneChanges);
        check(std::memcmp(scene,reference,sizeof(scene))==0,"incremental scene follows independent SDL reference");
        for(int i=0;i<n;++i) present(regions[i]);
        bool same=true;
        for(int dy=GameTop;dy<GameBottom;++dy) for(int dx=0;dx<ScreenW;++dx)
            if(screen[dy*ScreenW+dx]!=reference[(dy-GameTop)*2/3*GameW+dx*2/3]) same=false;
        check(same,"band updates preserve every pixel across motion, ROI and shake");
    }
    std::fill(frame,frame+GameW*GameH,1);
    n=updateSceneChanges(scene,frame,palette,0,0,GameW,GameH,false,regions,1);
    check(n==1&&regions[0].w==ScreenW&&regions[0].h==300,"insufficient capacity covers all changed bands");
    check(updateSceneChanges(scene,frame,palette,0,0,GameW,GameH,false,regions,MaxSceneChanges)==0,
          "unchanged full frame after fallback sends nothing");
    std::printf("PASS: changed bands, palette-only updates, capacity fallback and moving/shaking scenes\n");
}
static void touches(Controls &c,uint32_t time,std::initializer_list<Point> p) {
    c.update(p.begin(),(int)p.size(),time);
}
static void tap(Controls &c,int x,int y,uint32_t now) {
    touches(c,now,{{x,y,1}}); touches(c,now+40,{});
}
static void openTools(Controls &c,uint32_t now) {
    touches(c,now,{{240,80,2}});
    touches(c,now+LongPressMs,{{240,80,2}});
    touches(c,now+LongPressMs+20,{});
}
static void gestureTests() {
    Controls c;
    check(c.state.volume==1,"boot volume level is one");
    const Point edges[]={{0,0,0},{159,159,0},{160,0,0},{319,159,0},
        {320,0,0},{479,159,0},{0,160,0},{159,319,0},{160,160,0},{319,319,0},
        {320,160,0},{479,319,0}};
    for(int i=0;i<12;++i) check(zoneAt(edges[i].x,edges[i].y)==i/2,"six-zone boundaries");
    struct Direction { Point p; uint32_t key; } dirs[]={{{80,196,1},kKeyUp},
        {{80,284,1},kKeyDown},{{36,240,1},kKeyLeft},{{124,240,1},kKeyRight}};
    KeyRepeater repeated;
    for(unsigned i=0;i<4;++i) {
        uint32_t now=100+i*100;
        touches(c,now,{dirs[i].p});
        check(c.state.keys==dirs[i].key,"cardinal press immediately produces a direction");
        check(visibleDirection(c.state)==dirs[i].key,"only the current cardinal direction is visible");
        check(c.state.commands==0&&c.state.joystick,"direction is never cancel");
        auto down=repeated.update(c.state.keys,now);
        check(down.pressed==dirs[i].key&&!down.released,"one keydown for a direction tap");
        touches(c,now+20,{dirs[i].p});
        check(!repeated.update(c.state.keys,now+20).pressed,"no repeat during a short tap");
        touches(c,now+60,{});
        auto up=repeated.update(c.state.keys,now+60);
        check(up.released==dirs[i].key&&!up.pressed,"tap releases direction once");
        check(!c.state.commands&&!c.state.joystick,"lifting a direction never sends cancel");
    }
    touches(c,600,{{80,240,1}}); check(!c.state.keys,"fixed center is neutral");
    touches(c,640,{}); check(!c.state.commands,"neutral tap does not cancel");
    touches(c,700,{{124,240,1}}); touches(c,720,{{120,199,1}});
    check(c.state.keys==kKeyRight,"diagonal jitter does not change the direction");
    touches(c,740,{{120,188,1}}); check(c.state.keys==kKeyUp,"decisive sector change works");
    touches(c,760,{{400,240,1}}); check(c.state.keys==kKeyRight,"joystick retains finger outside its zone");
    touches(c,780,{}); check(!c.state.commands,"drag into confirm zone never confirms");
    touches(c,800,{{80,196,4},{400,240,8}});
    touches(c,820,{{400,240,8},{80,196,4}});
    check(c.state.keys==kKeyUp,"reordered touch reports preserve joystick ownership");
    check(visibleDirection(c.state)==kKeyUp,"confirmation adds no button feedback");
    touches(c,840,{{80,196,4}});
    check(c.state.commands==kKeySearch&&c.state.keys==kKeyUp,"confirm while moving");
    touches(c,860,{}); check(!c.state.commands&&!c.state.keys,"all contacts released");
    tap(c,400,80,900); check(c.state.commands==kKeyMenu,"right upper zone cancels");
    touches(c,960,{}); check(!c.state.commands,"cancel is one shot");
    touches(c,1000,{{80,80,1}}); check(!c.state.keys&&!c.state.commands,"R waits for release");
    touches(c,1040,{}); check(c.state.commands==kKeyRepeat&&!c.state.keys,"upper left tap is R");
    touches(c,1100,{{240,240,1}}); check(!c.state.keys&&!c.state.commands,"F waits for release");
    touches(c,1140,{}); check(c.state.commands==kKeyForce,"lower middle tap is F");
    tap(c,240,80,1200); check(!c.state.toolbar&&!c.state.commands,"upper middle tap does nothing");
    openTools(c,1200); check(c.state.toolbar&&!c.state.commands,"upper middle hold opens tools only");
    tap(c,230,290,1300); check(c.state.commands==kKeyStatus,"status tool retained");
    tap(c,330,290,1400); check(c.state.commands==kKeyAuto,"auto tool retained");
    touches(c,1500,{{72,240,3}}); check(c.state.volume==0,"volume zero");
    touches(c,1520,{{370,240,3}}); check(c.state.volume==5,"volume maximum is five");
    touches(c,1540,{{479,60,3}}); check(c.state.volume==5,"slider capture beyond panel");
    touches(c,1560,{{479,60,3},{130,285,5}});
    check(c.state.keys==kKeyPgDn&&c.state.volume==5,"slider and toolbar key operate independently");
    touches(c,1580,{{10,60,3},{130,285,5}}); check(c.state.volume==0,"slider lower clamp");
    touches(c,1600,{});
    touches(c,1620,{{72,240,3},{370,240,5}});
    check(c.state.volume==0,"second finger cannot take slider ownership"); touches(c,1640,{});
    touches(c,1700,{{72,240,3}});
    for(int x=0;x<ScreenW;++x) {
        touches(c,1701+x,{{x,240,3}});
        double fraction=(std::clamp(x,SliderLeft,SliderRight)-SliderLeft)/(double)(SliderRight-SliderLeft);
        check(c.state.volume==(int)std::lround(fraction*5),"volume snaps to six detents across the full range");
    }
    touches(c,3000,{});
    for(int level=0;level<=5;++level) {
        int x=SliderLeft+(SliderRight-SliderLeft)*level/VolumeMax;
        tap(c,x,240,3100+level*100);
        check(c.state.volume==level,"each displayed detent selects its level");
        int expected=level ? (int)std::lround(2*(-6+20*std::log10(level/5.0))) : -191;
        check(volumeGainHalfDb(level)==expected,"discrete levels preserve the calibrated gain curve");
    }
    check(volumeGainHalfDb(-1)==-191&&volumeGainHalfDb(9)==-12,"gain clamps outside volume range");
    tap(c,400,80,3800); check(!c.state.toolbar&&!c.state.commands,"cancel zone closes toolbar without game cancel");
    touches(c,3900,{{400,240,1}}); touches(c,3920,{{450,240,1}}); touches(c,3940,{});
    check(!c.state.commands,"confirmation drag is rejected");
    touches(c,4000,{{400,240,1}}); touches(c,4600,{});
    check(c.state.commands==kKeyAuto,"confirmation long hold sends AUTO instead of confirmation");
    touches(c,4700,{{80,196,1}}); c.cancel();
    check(!c.state.keys&&!c.state.commands&&!c.state.joystick,"transport fault releases safely");
    touches(c,4720,{{80,196,1}}); check(!c.state.keys,"fault requires fingers lifted");
    touches(c,4740,{}); tap(c,400,240,4800); check(c.state.commands==kKeySearch,"recovers after release");
    touches(c,4900,{{80,196,1},{400,240,1}}); check(!c.state.keys&&!c.state.commands,"duplicate IDs rejected");
    touches(c,4920,{}); touches(c,5000,{{-1,100,1}}); check(!c.state.commands,"invalid coordinates rejected");
    touches(c,5020,{}); touches(c,0xfffffff0u,{{400,240,1}}); touches(c,20,{});
    check(c.state.commands==kKeySearch,"tap timer wraparound");
    touches(c,100,{{400,240,1},{240,80,2}});
    touches(c,700,{{240,80,2}}); touches(c,720,{});
    check(c.state.toolbar&&!c.state.commands,"tool transition discards simultaneous confirmation");
    touches(c,200,{{130,285,1},{240,80,2}}); touches(c,240,{{130,285,1}});
    check(!c.state.toolbar&&!c.state.keys,"closing toolbar releases captured keys");
    touches(c,260,{}); check(!c.state.commands,"old toolbar finger cannot activate a game zone");
    c.update(nullptr,1,300); check(!c.state.commands&&!c.state.keys,"missing sample rejected");
    std::printf("PASS: fixed directional taps, six-zone actions, volume 0-5, ownership and safety\n");
}
static void shortcutTests() {
    for (uint32_t age : {0u,40u,599u,600u,601u,2000u}) {
        Controls tools;
        touches(tools,100,{{240,80,1}});
        touches(tools,100+age,{});
        check(tools.state.toolbar==(age>=LongPressMs),"Tools release fallback respects hold threshold");
        check(!tools.state.commands&&!tools.state.keys,"Tools never sends a game command");
    }
    {
        Controls tools;
        touches(tools,100,{{240,80,1}});
        touches(tools,699,{{240,80,1}}); check(!tools.state.toolbar,"Tools waits for threshold");
        touches(tools,700,{{240,80,1}}); check(tools.state.toolbar,"Tools opens while held");
        touches(tools,2000,{{240,80,1}}); check(tools.state.toolbar,"held Tools does not toggle repeatedly");
        touches(tools,2100,{}); check(tools.state.toolbar,"release does not close newly opened Tools");
        tap(tools,240,80,2200); check(!tools.state.toolbar,"tap closes open Tools");
        touches(tools,3000,{{240,80,1}});
        touches(tools,3600,{{260,80,1}}); touches(tools,3700,{});
        check(!tools.state.toolbar,"drag cancels Tools hold");
        touches(tools,0xfffffff0u,{{240,80,1}});
        touches(tools,584,{{240,80,1}}); check(tools.state.toolbar,"Tools hold survives timer wraparound");
    }
    struct ShortcutCase { int x,y; uint32_t shortKey,longKey; } cases[]={
        {80,80,kKeyRepeat,kKeyAuto},{240,240,kKeyForce,kKeyDefend},
        {400,240,kKeySearch,kKeyAuto}};
    for(auto test:cases) {
        Controls c;
        for(uint32_t age:{0u,1u,599u,600u,601u,1000u}) {
            touches(c,1000,{{test.x,test.y,1}});
            check(!c.state.keys&&!c.state.commands,"shortcut never fires on down");
            check(visibleDirection(c.state)==0,"shortcut does not show a direction arrow");
            touches(c,1000+age,{});
            check(c.state.commands==(age<LongPressMs?test.shortKey:test.longKey),
                  "release threshold, including no final held sample");
            touches(c,2500,{}); check(!c.state.commands,"release command is one shot");
        }
        touches(c,3000,{{test.x,test.y,1}});
        touches(c,3599,{{test.x,test.y,1}}); check(!c.state.commands,"long action waits 600ms");
        touches(c,3600,{{test.x,test.y,1}});
        check(c.state.commands==test.longKey&&!c.state.keys,"long action at threshold is a pulse");
        touches(c,4000,{{test.x,test.y,1}}); check(!c.state.commands,"long hold never repeats");
        touches(c,9000,{{test.x,test.y,1}}); check(!c.state.commands,"extended hold never repeats");
        touches(c,9020,{}); check(!c.state.commands,"long release never adds the short action");
        touches(c,10000,{{test.x,test.y,1}});
        touches(c,10200,{{test.x+20,test.y,1}});
        touches(c,10600,{{test.x+20,test.y,1}}); check(!c.state.commands,"drag cancels long action");
        touches(c,10620,{}); check(!c.state.commands,"drag cancels short action");
        touches(c,11000,{{test.x,test.y,1}}); c.cancel();
        touches(c,12000,{{test.x,test.y,1}}); check(!c.state.commands,"fault cannot trigger long action");
        touches(c,12020,{}); check(!c.state.commands,"fault release cannot trigger short action");
        touches(c,0xfffffff0u,{{test.x,test.y,1}});
        touches(c,583,{{test.x,test.y,1}}); check(!c.state.commands,"long timer wrap before threshold");
        touches(c,584,{{test.x,test.y,1}}); check(c.state.commands==test.longKey,"long timer wrap threshold");
        touches(c,600,{}); check(!c.state.commands,"wraparound long release has no extra short action");
        touches(c,0xfffffff0u,{{test.x,test.y,1}}); touches(c,20,{});
        check(c.state.commands==test.shortKey,"short timer wraps correctly");
        touches(c,100,{{test.x,test.y,1},{240,80,2}});
        touches(c,700,{{test.x,test.y,1},{240,80,2}});
        check(c.state.toolbar&&!c.state.commands,"opening tools suppresses a pending shortcut");
        touches(c,800,{}); check(!c.state.commands,"old shortcut finger cannot activate after tools open");
    }
    Controls both;
    touches(both,0,{{80,80,1},{240,240,2}});
    touches(both,600,{{240,240,2},{80,80,1}});
    check(both.state.commands==(kKeyAuto|kKeyDefend),"two shortcut fingers keep independent timers");
    touches(both,650,{}); check(!both.state.commands,"neither long release sends R or F");
    ControlState neutral;
    check(!visibleDirection(neutral)&&directionBounds(0).w==0,"idle state draws no direction arrow");
    for(uint32_t key:{(uint32_t)kKeyUp,(uint32_t)kKeyDown,(uint32_t)kKeyLeft,(uint32_t)kKeyRight}) {
        neutral.joystick=true; neutral.keys=key;
        auto r=directionBounds(visibleDirection(neutral));
        check(r.w==15&&r.h==15,"only a small active-arrow rectangle is needed");
        check(r.x>=0&&r.y>=0&&r.x+r.w<=ScreenW&&r.y+r.h<=ScreenH,"arrow bounds on screen");
    }
    std::printf("PASS: R/A and F/D tap/hold exclusivity, one-shot long holds, faults and wraparound\n");
}
static void repeatTests() {
    for(uint32_t key:{(uint32_t)kKeyUp,(uint32_t)kKeyDown,(uint32_t)kKeyLeft,
        (uint32_t)kKeyRight,(uint32_t)kKeyPgUp,(uint32_t)kKeyPgDn}) {
        KeyRepeater r;
        check(r.update(key,100).pressed==key,"first press is immediate");
        check(!r.update(key,449).pressed,"repeat waits 350ms");
        check(r.update(key,450).pressed==key,"first held repeat at 350ms");
        check(!r.update(key,549).pressed,"subsequent repeat waits 100ms");
        check(r.update(key,550).pressed==key,"held repeat at 100ms");
        auto up=r.update(0,560); check(up.released==key&&!up.pressed,"release stops repeat");
        check(!r.update(0,900).pressed,"no repeat after release");
        check(r.update(key,920).pressed==key,"a quick second tap is independent");
    }
    KeyRepeater wrap;
    check(wrap.update(kKeyUp,0xfffffff0u).pressed==kKeyUp,"repeat starts before wrap");
    check(!wrap.update(kKeyUp,333).pressed,"wraparound delay not expired");
    check(wrap.update(kKeyUp,334).pressed==kKeyUp,"wraparound repeat deadline");
    auto change=wrap.update(kKeyDown,350);
    check(change.pressed==kKeyDown&&change.released==kKeyUp,"direction change releases old and presses new");
    std::printf("PASS: actual key repeat policy, single taps, held keys and timer wraparound\n");
}
static void powerTests() {
    PmicPowerButton b;
    b.arm(100);
    check(!b.consume(0x08,499),"boot/wake guard consumes a stale short press");
    check(!b.consume(0x04,500)&&!b.consume(0x0c,501),"long press never requests software sleep");
    check(!b.consume(0xf3,502),"unrelated PMIC interrupts never request sleep");
    check(b.consume(0x08,600),"PMIC short press requests sleep");
    check(!b.consume(0x08,650)&&!b.consume(0x08,999),"duplicate short press is suppressed");
    check(!b.consume(0,1000),"absence of a key event never wakes or sleeps");
    check(b.consume(0x88,1001),"short press with unrelated VBUS flag is accepted");
    b.arm(1200);
    check(!b.consume(0x08,1599),"wake press cannot immediately put game back to sleep");
    check(b.consume(0x08,1600),"next independent short press works after guard");
    PmicPowerButton wrap;
    wrap.arm(0xffffff00u);
    check(!wrap.consume(0x08,0x8fu),"PMIC event guard wraps before threshold");
    check(wrap.consume(0x08,0x90u),"PMIC event guard wraps at threshold");
    PmicPowerButton idle;
    idle.arm(0);
    for (uint32_t now=400;now<20000;now+=50)
        check(!idle.consume(0,now),"GPIO changes cannot synthesize a PMIC short-press event");
    GameClock clock;
    check(clock.milliseconds(1000)==1000,"initial game clock follows raw time");
    clock.exclude(2000); check(clock.milliseconds(3000)==1000,"sleep duration excluded from game time");
    check(clock.milliseconds(3500)==1500,"running game time advances normally");
    clock.exclude(3600000); check(clock.milliseconds(3603500)==1500,"multiple long pauses retain exact game position in time");
    GameClock clockWrap;
    clockWrap.exclude(1000); check(clockWrap.milliseconds((uint32_t)(0xfffffff0u+1000u))==0xfffffff0u,"sleep across raw-clock wraparound");
    GameClock offsetWrap;
    offsetWrap.exclude(0xfffffff0u); offsetWrap.exclude(64);
    check(offsetWrap.milliseconds(48)==0,"accumulated pause offset wraps safely");
    std::printf("PASS: PMIC PWR events, stale/wake suppression and retained game clock\n");
}
int main() { rendererTests(); overlayAreaTests(); changedRegionTests(); gestureTests(); shortcutTests(); repeatTests(); powerTests();
    std::printf("PASS: %u assertions\n",checks); }
