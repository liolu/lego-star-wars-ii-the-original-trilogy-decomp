#include "m2c.h"

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct {
    s32 f0;
    Vec3 pos;
    u8 pad[0x28 - 0x10];
    Vec3 half;
} BoxObj;

typedef struct {
    s32 f0;
    Vec3 pos;
    u8 pad[0x30 - 0x10];
    s32 radius;
} SphereObj;

enum {
    FX_SHIFT = 12,    /* 20.12 fixed point */
    FX_HALF  = 0x800, /* rounding term: 0.5 in 20.12 */
};

static inline s32 FxMul(s32 a, s32 b) {
    return (s32) (((s64) a * b + FX_HALF) >> FX_SHIFT);
}

/*
 * Box/sphere overlap test. Each axis of the sphere centre is clamped into
 * [box centre - half extent, box centre + half extent] to get the closest
 * point of the box; returns 1 when the squared distance from that point to
 * the sphere centre (rounded fixed-point products) is <= radius squared.
 * cx/cy/cz hold the clamped point, then the per-axis offset from the centre.
 */
s32 func_01ff9e38(BoxObj *box, SphereObj *s) {
    s32 px, py, pz, cx, cy, cz, rr, sum;
    px = s->pos.x;
    cx = box->pos.x - box->half.x;
    if (px >= cx) {
        cx = box->pos.x + box->half.x;
        if (px <= cx) {
            cx = px;
        }
    }
    py = s->pos.y;
    cy = box->pos.y - box->half.y;
    if (py >= cy) {
        cy = box->pos.y + box->half.y;
        if (py <= cy) {
            cy = py;
        }
    }
    pz = s->pos.z;
    cz = box->pos.z - box->half.z;
    if (pz >= cz) {
        cz = box->pos.z + box->half.z;
        if (pz <= cz) {
            cz = pz;
        }
    }
    cx -= px;
    cy -= py;
    cz -= pz;
    rr  = FxMul(s->radius, s->radius);
    sum = FxMul(cx, cx);
    sum += FxMul(cy, cy);
    sum += FxMul(cz, cz);
    return sum <= rr;
}
