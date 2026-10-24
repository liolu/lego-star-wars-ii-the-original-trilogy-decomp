/* Reconstructed from USA ARM9 at 0x02029068 (120 bytes).
 *
 * Yaw angle from an object to a position: the object position (+0xb0) is
 * subtracted component wise from the target position and the x/z difference
 * is handed to the fixed point arc-tangent (func_020985d8). A null object
 * yields angle 0. The result goes through a halfword scratch slot.
 *
 * The difference vector and its x/z copies live in one stack aggregate of
 * single-field members, so the stores stay as in the original code.
 */
#include "m2c.h"

typedef struct {
    s32 v;
} Fx;

typedef struct {
    Fx qx;
    Fx qz;
    Fx dx;
    Fx dy;
    Fx dz;
} Loc;

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} V3;

extern s16 func_020985d8(s32, s32);

void func_02029068(u16 *out, M2C_UNK a, V3 *b) {
    u16 sp2;
    u16 sp0;
    u16 *p;
    Loc l;
    s32 v0;
    s32 v1;
    s32 v2;

    if (a != 0) {
        v2     = b->z - M2C_FIELD(a, s32 *, 0xb8);
        l.qz.v = v2;
        v1     = b->y - M2C_FIELD(a, s32 *, 0xb4);
        l.dy.v = v1;
        v0     = b->x - M2C_FIELD(a, s32 *, 0xb0);
        l.dx.v = v0;
        l.dy.v = v1;
        l.dz.v = v2;
        l.qz   = l.dz;
        l.qx   = l.dx;
        sp2    = func_020985d8(l.dx.v, l.dz.v);
        p      = &sp2;
    } else {
        sp0 = 0;
        p   = &sp0;
    }
    *out = *p;
}
