#include "m2c.h"

/* Reconstructed from USA ARM9 at 0x02032a78 (128 bytes).
 *
 * Yaw angle from the controller's entity (+0x04) to its target (+0x10): the
 * entity positions (+0xb0/+0xb4/+0xb8) are subtracted component wise and the
 * x/z difference is handed to the fixed point arc-tangent (func_020985d8).
 * The angle and the halfword at +0x14 are then passed by value to func_02018210
 * together with the entity.
 *
 * The difference vector and its x/z copies live in one stack aggregate of
 * single-field members, so the stores stay as in the original code.
 */
typedef struct {
    u16 v;
} HalfVal;

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
    u8 pad0[0xb0];
    s32 x;
    s32 y;
    s32 z;
} Entity;

typedef struct {
    u8 pad0[4];
    Entity *ent;    /* +0x04 */
    u8 pad8[8];
    Entity *target; /* +0x10 */
    HalfVal h14;    /* +0x14 */
} Obj;

extern s16 func_020985d8(s32, s32);
extern s32 func_02018210(Entity *, HalfVal, HalfVal);

void func_02032a78(Obj *o) {
    Loc l;
    HalfVal yaw;
    HalfVal prev;
    s32 v0;
    s32 v1;
    s32 v2;

    v2     = o->target->z - o->ent->z;
    l.qz.v = v2;
    v1     = o->target->y - o->ent->y;
    l.dy.v = v1;
    v0     = o->target->x - o->ent->x;
    l.dx.v = v0;
    l.dy.v = v1;
    l.dz.v = v2;
    l.qz   = l.dz;
    l.qx   = l.dx;
    yaw.v  = func_020985d8(l.dx.v, l.dz.v);
    prev   = o->h14;
    func_02018210(o->ent, yaw, prev);
}
