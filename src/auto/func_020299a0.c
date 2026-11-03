/* Offsets the position of owner->shape by a yaw angle and passes the resulting box to func_0204c8a8.
 *
 * Shape kind 3 uses horiz = f2c, drop = f28; kind 5 uses horiz = (f28 + f30) / 2, drop = f2c; any
 * other kind (or no shape) returns 0. The point is pos + (horiz * sin, -drop, horiz * cos) in fx32
 * (20.12, rounded FX_Mul), with sin/cos taken from the s16 sine/cosine pair table at index
 * angle >> 4. func_0204c5fc expands that point by shape->extent into the box {x0, x1, y0, y1, z0, z1};
 * the function returns 1 when func_0204c8a8 (on the object read via data_0210930c +0x68, +0x384)
 * returns 0 for that box, else 0. What that test means is not established here. */
#include "m2c.h"

extern const s16 data_020f88c4[];
extern u8 data_0210930c[];

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} V3;

typedef struct {
    u16 v;
} Ang;

typedef struct {
    u8 pad_0[4];
    s32 px;
    s32 py;
    s32 pz;
    u8 pad_10[4];
    V3 extent;
    u8 pad_20[5];
    s8 kind;
    u8 pad_26[2];
    s32 f28;
    s32 f2c;
    s32 f30;
} Shape;

typedef struct {
    u8 pad_0[0x20];
    Shape *shape;
} Owner;

extern s32 *func_0204c5fc(s32 *, V3 *, V3 *);
extern s32 func_0204c8a8(s32, s32 *);

enum {
    FX_SHIFT = 12,
    FX_HALF  = 0x800
};
enum {
    ANGLE_INDEX_SHIFT = 4
};

/* fx32 multiply with rounding (FX_Mul). */
static inline s32 fx_mul(s32 a, s32 b) {
    return (s32) (((s64) a * b + FX_HALF) >> FX_SHIFT);
}

s32 func_020299a0(void *arg0, Owner *owner, Ang ang) {
    Shape *sh;
    s32 drop;
    s32 horiz;
    int x, y, z;
    s32 sin_a;
    s32 cos_a;
    s32 box[6];
    V3 v;

    sh = owner->shape;
    if (sh != NULL) {
        if (sh->kind == 3) {
            horiz = sh->f2c;
            drop  = sh->f28;
        } else if (sh->kind == 5) {
            horiz = (sh->f28 + sh->f30) >> 1;
            drop  = sh->f2c;
        } else {
            return 0;
        }
        sin_a = data_020f88c4[(ang.v >> ANGLE_INDEX_SHIFT) * 2];
        cos_a = data_020f88c4[(ang.v >> ANGLE_INDEX_SHIFT) * 2 + 1];
        x     = fx_mul(horiz, sin_a);
        z     = fx_mul(horiz, cos_a);
        /* Order matters for the register assignment: py is read before the x sum and the
         * negated drop is added last. */
        z    = sh->pz + z;
        y    = sh->py;
        x    = sh->px + x;
        drop = -drop;
        y += drop;
        v.x = x;
        v.y = y;
        v.z = z;
        func_0204c5fc(box, &v, &sh->extent);
        if (func_0204c8a8(*(s32 *) (*(u8 **) (data_0210930c + 0x68) + 0x384), box) == 0) {
            return 1;
        }
    }
    return 0;
}
