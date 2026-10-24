#include "m2c.h"

typedef struct {
    s32 v;
} Fx;

typedef struct {
    u16 v;
} Angle;

typedef struct {
    Fx x;
    Fx y;
    Fx z;
} VecFx;

typedef struct {
    u16 f0;
    u8 f2;
    u8 f3;
    u8 f4;
    u8 f5;
} OutVec;

typedef struct {
    u8 pad0[0xb0];
    VecFx pos;
    u8 padbc[0xca - 0xbc];
    Angle yaw;
} Entity;

extern void func_0202732c(void *);
extern s16 func_020985d8(s32, s32);
extern s32 func_0200f5d0(VecFx *, VecFx *, Fx, s32 *);

static inline void vec_set(s32 *v, s32 x, s32 y, s32 z) {
    v[0] = x;
    v[1] = y;
    v[2] = z;
}

static inline u16 fx_atan2(Fx x, Fx z) {
    return func_020985d8(x.v, z.v);
}

void func_02027590(OutVec *out, Entity *ent, VecFx *pos) {
    Angle h;
    Fx md;
    s32 dist;
    Fx qz;
    Fx qx;
    VecFx d;
    u16 yaw;

    md.v = 0x14000;
    func_0202732c(out);
    vec_set((s32 *) &d, pos->x.v - ent->pos.x.v, pos->y.v - ent->pos.y.v, pos->z.v - ent->pos.z.v);
    qz      = d.z;
    qx      = d.x;
    yaw     = fx_atan2(qx, qz);
    h       = ent->yaw;
    dist    = 0;
    out->f0 = yaw - h.v;
    out->f5 = 100;
    if (func_0200f5d0(&ent->pos, pos, md, &dist) != 0) {
        if (dist < 0xE1000) {
            out->f2 = 0;
        } else {
            out->f5 = 0;
        }
    } else {
        out->f2 = 1;
    }
    out->f4 = 100;
}
