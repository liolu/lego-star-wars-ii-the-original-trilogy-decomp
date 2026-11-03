#include "m2c.h"

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct {
    u8 pad0[0x28];
    s32 sx;
    s32 sy;
    s32 sz;
} Scale;

typedef struct {
    s32 v;
} Fx;

typedef struct {
    Fx x;
    Fx y;
    Fx z;
} FxVec3;

// 1.0 and 0.5 in 20.12 fixed point.
enum {
    FX_ONE  = 0x1000,
    FX_HALF = 0x800
};

static inline Fx *FxAbs(Fx *dst, Fx a) {
    if (a.v < 0) {
        dst->v = -a.v;
        return dst;
    }
    dst->v = a.v;
    return dst;
}

static inline s32 AbsV(Fx b) {
    s32 r = b.v;
    if (r < 0) {
        r = -r;
    }
    return r;
}

static inline void Axis(Vec3 *out, FxVec3 *wp) {
    Fx t3;
    Fx t2;
    Fx t1;

    if (FxAbs(&t1, wp->x)->v > AbsV(wp->y)) {
        if (FxAbs(&t2, wp->x)->v > AbsV(wp->z)) {
            if (wp->x.v > 0) {
                out->x = FX_ONE;
                out->y = 0;
                out->z = 0;
            } else {
                out->x = -FX_ONE;
                out->y = 0;
                out->z = 0;
            }
            return;
        }
        if (wp->z.v > 0) {
            out->x = 0;
            out->y = 0;
            out->z = FX_ONE;
        } else {
            out->x = 0;
            out->y = 0;
            out->z = -FX_ONE;
        }
        return;
    }
    if (FxAbs(&t3, wp->y)->v > AbsV(wp->z)) {
        if (wp->y.v > 0) {
            out->x = 0;
            out->y = FX_ONE;
            out->z = 0;
        } else {
            out->x = 0;
            out->y = -FX_ONE;
            out->z = 0;
        }
        return;
    }
    if (wp->z.v > 0) {
        out->x = 0;
        out->y = 0;
        out->z = FX_ONE;
    } else {
        out->x = 0;
        out->y = 0;
        out->z = -FX_ONE;
    }
}

static inline s32 FxMulR(const s32 *a, const s32 *b) {
    return (s32) (((s64) *a * *b + FX_HALF) >> 12);
}

// Listed in the vtable data_020fe4e0 (ctor func_0205039c); the Vec3 result is
// returned through out. Scales v by the cofactors of the scale at 0x28
// (x*sy*sz, y*sx*sz, z*sx*sy) and returns the signed unit axis of its largest
// component.
void func_02050418(Vec3 *out, Scale *s, Vec3 v) {
    FxVec3 w;

    w.x.v = FxMulR(&v.x, &s->sy);
    w.x.v = FxMulR(&w.x.v, &s->sz);
    w.y.v = FxMulR(&v.y, &s->sx);
    w.y.v = FxMulR(&w.y.v, &s->sz);
    w.z.v = FxMulR(&v.z, &s->sx);
    w.z.v = FxMulR(&w.z.v, &s->sy);

    Axis(out, &w);
}
