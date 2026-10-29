// SDLPAL P4 nearest-neighbor scaling and clean scene; GPL-3.0-or-later.
#if defined(ESP_PLATFORM)
#pragma GCC optimize ("O2")
#endif
#include "landscape.h"
#include <string.h>
namespace p4 {
Rect updateScene(uint16_t *scene, const uint8_t *frame, const uint16_t *palette,
                 unsigned x, unsigned y, unsigned w, unsigned h, bool padding) {
    Rect dirty{};
    updateSceneChanges(scene,frame,palette,x,y,w,h,padding,&dirty,1);
    return dirty;
}
int updateSceneChanges(uint16_t *scene, const uint8_t *frame, const uint16_t *palette,
                       unsigned x, unsigned y, unsigned w, unsigned h, bool padding,
                       Rect *changes, int capacity) {
    if (!scene || !frame || !palette || !changes || capacity<1 || !w || !h ||
        x >= GameW || y >= GameH) return 0;
    unsigned right = x + w < GameW ? x + w : GameW;
    unsigned bottom = y + h < GameH ? y + h : GameH;
    unsigned top = padding ? 0 : y, end = padding ? GameH : bottom;
    Rect bands[MaxSceneChanges]{};
    int count=0;
    unsigned allLeft=GameW,allTop=GameH,allRight=0,allBottom=0;
    for (unsigned band=top;band<end;) {
        unsigned bandEnd=(band/ChangeBandRows+1)*ChangeBandRows;
        if (bandEnd>end) bandEnd=end;
        unsigned left=GameW,upper=GameH,lower=0,last=0;
        for (unsigned row=band;row<bandEnd;++row) {
            for (unsigned col=padding?0:x;col<(padding?GameW:right);++col) {
                uint16_t value=row>=y && row<bottom && col>=x && col<right ?
                    palette[frame[(padding?row-y:row)*GameW+col]] : 0;
                uint16_t &pixel=scene[row*GameW+col];
                if (pixel==value) continue;
                pixel=value;
                if (col<left) left=col;
                if (col+1>last) last=col+1;
                if (row<upper) upper=row;
                lower=row+1;
            }
        }
        if (last>left) {
            if (left<allLeft) allLeft=left;
            if (last>allRight) allRight=last;
            if (upper<allTop) allTop=upper;
            if (lower>allBottom) allBottom=lower;
            bands[count++]={scaledEdge(left),GameTop+scaledEdge(upper),
                            scaledEdge(last)-scaledEdge(left),scaledEdge(lower)-scaledEdge(upper)};
        }
        band=bandEnd;
    }
    if (count>capacity) {
        changes[0]={scaledEdge(allLeft),GameTop+scaledEdge(allTop),
                    scaledEdge(allRight)-scaledEdge(allLeft),scaledEdge(allBottom)-scaledEdge(allTop)};
        return 1;
    }
    memcpy(changes,bands,count*sizeof(Rect));
    return count;
}
void composeBand(uint16_t *out, const uint16_t *scene, const uint16_t *overlay,
                 Rect b, const Rect *areas, int areaCount) {
    // Scale the clean scene first. Every two source pixels become three pixels;
    // repeated destination rows can be copied before any overlay is applied.
    for (int y=b.y; y<b.y+b.h; ++y) {
        uint16_t *row=out+(y-b.y)*b.w;
        if (y<GameTop || y>=GameBottom) { memset(row,0,b.w*sizeof(uint16_t)); continue; }
        int sy=(y-GameTop)*2/3;
        if (y>b.y && y>GameTop && sy==(y-1-GameTop)*2/3) {
            memcpy(row,row-b.w,b.w*sizeof(uint16_t));
            continue;
        }
        const uint16_t *src=scene+sy*GameW;
        int x=b.x, end=b.x+b.w;
        while (x<end && x%3) *row++=src[x++*2/3];
        int sx=x*2/3;
        for (;x+3<=end;x+=3,sx+=2) {
            uint16_t a=src[sx];
            *row++=a; *row++=a; *row++=src[sx+1];
        }
        while (x<end) *row++=src[x++*2/3];
    }
    // Only read PSRAM overlay pixels inside actual control bounds. In normal
    // play this is just the small menu icon, not a full 480x320 overlay scan.
    const Rect full{0,0,ScreenW,ScreenH};
    if (!areas) { areas=&full; areaCount=1; }
    for (int i=0;i<areaCount;++i) {
        int left=areas[i].x>b.x?areas[i].x:b.x;
        int top=areas[i].y>b.y?areas[i].y:b.y;
        int right=areas[i].x+areas[i].w<b.x+b.w?areas[i].x+areas[i].w:b.x+b.w;
        int bottom=areas[i].y+areas[i].h<b.y+b.h?areas[i].y+areas[i].h:b.y+b.h;
        for (int y=top;y<bottom;++y) for (int x=left;x<right;++x) {
            uint16_t ui=overlay[y*ScreenW+x];
            if (ui) out[(y-b.y)*b.w+x-b.x]=(uint16_t)((ui<<8)|(ui>>8));
        }
    }
}
}
