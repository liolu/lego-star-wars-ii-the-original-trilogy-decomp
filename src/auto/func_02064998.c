#include "m2c.h"

typedef struct {
    u16 x;
    s16 y;
} Point;

typedef struct {
    u16 x;
    u16 y;
    u16 w;
    u16 h;
} Rect;

s32 func_02064998(s32 unused, Point pt, Rect *rects, s32 count, s32 *out) {
    s32 i;

    for (i = 0; count != 0; count--, i++, rects++) {
        u16 px = pt.x;
        u16 py = pt.y;
        u16 rh = rects->h;
        u16 rw = rects->w;
        u16 rx = rects->x;
        u16 ry = rects->y;
        if (px >= rx && px < rx + rw && py >= ry && py < ry + rh) {
            *out = i;
            return 1;
        }
    }
    *out = -1;
    return 0;
}
