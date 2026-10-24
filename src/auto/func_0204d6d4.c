#include "m2c.h"

typedef struct {
    s32 pad0;
    s32 x;
    s32 y;
    s32 z;
    u8 pad10[0x18];
    s32 halfHeight;
    s32 radius;
} Shape;

static inline s32 FxMul(s32 x, s32 y) {
    return (s32) (((s64) x * y + 0x800) >> 12);
}

/* Cylinder overlap test: vertical extents first, then XZ distance vs. radius sum. */
s32 func_0204d6d4(Shape *a, Shape *b) {
    s32 h    = a->halfHeight;
    s32 aMax = a->y;
    s32 aMin = aMax;
    s32 bMin;
    s32 bMax;
    s32 dx;
    s32 dz;
    s32 rr;

    aMax += h;
    aMin -= h;
    h    = b->halfHeight;
    bMin = b->y;
    bMax = bMin;
    bMin -= h;
    bMax += h;
    if (aMax < bMin || aMin > bMax) {
        return 0;
    }

    dx = a->x - b->x;
    dz = a->z - b->z;
    rr = a->radius + b->radius;
    dx = FxMul(dx, dx);
    dz = FxMul(dz, dz);
    rr = FxMul(rr, rr);
    if (dx + dz <= rr) {
        return 1;
    }
    return 0;
}
