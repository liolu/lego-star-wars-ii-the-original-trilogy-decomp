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
} V3;

typedef struct {
    u8 pad0[4];
    V3 pos;
} Anchor;

typedef struct {
    u8 pad0[0x34];
    Anchor *anchor;
    u8 pad38[0xb0 - 0x38];
    V3 pos;
    u8 padbc[0xca - 0xbc];
    u16 yaw;
    u8 padcc[0xd0 - 0xcc];
    u8 dirty;
} Obj;

extern s16 func_020985d8(s32, s32);

static inline void vec_copy(V3 *d, const V3 *s) {
    if (d != s) {
        d->x.v = s->x.v;
        d->y.v = s->y.v;
        d->z.v = s->z.v;
    }
}

static inline Angle fx_atan2(Fx x, Fx z) {
    Angle r;
    r.v = func_020985d8(x.v, z.v);
    return r;
}

static inline void set_yaw(Obj *a, Angle yaw) {
    if (a->yaw != yaw.v) {
        a->yaw   = yaw.v;
        a->dirty = 1;
    }
}

/* Turns this object toward `other`: takes the position difference (other minus
 * its anchor when it has one, otherwise this minus other), and if the x/z part
 * is non-zero updates the 16 bit yaw at +0xca (from the fixed point atan2
 * func_020985d8) and sets the dirty byte at +0xd0 when it changed.
 */
void func_02007154(Obj *this, Obj *other) {
    V3 delta;
    V3 anchor_delta;
    V3 obj_delta;
    Fx z_arg;
    Fx x_arg;
    s32 v0;
    s32 v1;
    s32 v2;
    Anchor *anchor;
    int has_anchor;

    delta.x.v  = 0;
    delta.y.v  = 0;
    delta.z.v  = 0;
    anchor     = other->anchor;
    has_anchor = 0; // FAKE: 0/1 flag instead of testing anchor directly reproduces the movne/cmp pair
    if (anchor != 0) {
        has_anchor = 1;
    }
    if (has_anchor) {
        v2               = other->pos.z.v - anchor->pos.z.v;
        v1               = other->pos.y.v - anchor->pos.y.v;
        v0               = other->pos.x.v - anchor->pos.x.v;
        anchor_delta.x.v = v0;
        anchor_delta.y.v = v1;
        anchor_delta.z.v = v2;
        vec_copy(&delta, &anchor_delta);
    } else {
        v2            = this->pos.z.v - other->pos.z.v;
        v1            = this->pos.y.v - other->pos.y.v;
        v0            = this->pos.x.v - other->pos.x.v;
        obj_delta.x.v = v0;
        obj_delta.y.v = v1;
        obj_delta.z.v = v2;
        vec_copy(&delta, &obj_delta);
    }
    if (delta.x.v == 0 && delta.z.v == 0) {
        return;
    }
    z_arg = delta.z;
    x_arg = delta.x;
    set_yaw(this, fx_atan2(x_arg, z_arg));
}
