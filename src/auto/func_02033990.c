#include "m2c.h"

typedef struct {
    s32 x, y, z;
} Vec3;

typedef struct {
    u8 pad0[4];
    u8 *obj;
} Owner;

typedef struct {
    u8 pad0[0x4c];
    u8 *list;
} Mgr;

extern Mgr data_0210930c;

s32 func_02093534(Mgr *, void *);
void func_02007f24(s32 *, double);
typedef struct {
    s32 v;
} Fx;
s32 func_0200f5d0(void *, void *, Fx, s32 *);

static inline s32 fxmul(s32 x, s32 n) {
    return (s32) (((s64) x * (s64) (n << 12) + 0x800) >> 12);
}

void func_02033990(Owner *self) {
    u8 *list;
    s32 idx;
    Vec3 *src;
    Fx rad;
    s32 dist;
    s32 b, a, c;
    s32 q;
    s32 *pa, *pb;

    int ok;
    u8 *o2;

    src  = (Vec3 *) (self->obj + 0xb0);
    list = data_0210930c.list;
    idx  = func_02093534(&data_0210930c, self->obj);
    o2   = self->obj;
    if (o2[0x40] != 0) {
        u8 *l = 0;
        ok    = 0;
        if (o2[0x40] != 0 && o2[0x41] != 0) {
            l = *(u8 **) (o2 + 0x54);
            if (l != 0) {
                l = l - 4;
            }
            if (l != 0) {
                ok = 1;
            }
        }
        if (ok == 0) {
            Vec3 *dst = (Vec3 *) (o2 + 0x6a0);
            if (dst != src) {
                dst->x = src->x;
                dst->y = src->y;
                dst->z = src->z;
            }
        }
    }
    func_02007f24(&a, 12.0);
    pa = &a;
    q  = 0;
    if (pa) {
        q = fxmul(*pa, idx);
    }
    func_02007f24(&b, 4.0);
    pb   = &b;
    dist = 0;
    if (pb) {
        rad.v = *pb + q;
    } else {
        rad.v = q;
    }
    func_0200f5d0(src, list + 0xb0, rad, &dist);
    func_02007f24(&c, 2.0);
}
