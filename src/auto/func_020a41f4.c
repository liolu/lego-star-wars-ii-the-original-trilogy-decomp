#include "m2c.h"

/* Pad-driven free movement of an object (mode 2 of the func_020a0630 update switch).
 *
 * Two Q12 step vectors, forward (0, 0, 3.0) and side (3.0, 0, 0), are rotated by the
 * object's angles at +0x40 (matrix from func_01ffa9e8, applied by func_01ffac98). The
 * pad bits polled through func_0205d060 on the pad state block data_021027f8 (record 4 =
 * the current one) follow the NDS key layout, which fits their pairing here:
 *   L / R           move back / forward along the rotated forward vector
 *   Left / Right    turn ry by +/-0.04 rad, or strafe along the side vector while Y is held
 *   Down / Up       turn rx by -/+0.03 rad, or move -/+3.0 on world y while Y is held
 * Every position or angle write that changes the value sets posDirty / rotDirty.
 * Radians come from func_02007f24 (double -> Q12) and are converted to 16-bit angles.
 */
enum {
    PAD_RIGHT   = 0x10,
    PAD_LEFT    = 0x20,
    PAD_UP      = 0x40,
    PAD_DOWN    = 0x80,
    PAD_R       = 0x100,
    PAD_L       = 0x200,
    PAD_Y       = 0x800,
    PAD_CURRENT = 4,      /* func_0205d060 record index meaning "current" (read from +0x2a) */
    MOVE_STEP   = 0x3000, /* 3.0 in Q12 */
};

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct {
    u16 v;
} Angle;

typedef struct {
    s32 m[4][4];
} Mtx44;

typedef struct {
    u8 pad00[0x28];
    Vec3 pos;
    u8 pad34[0x40 - 0x34];
    Angle rx;
    Angle ry;
    u8 pad44[0x48 - 0x44];
    u8 rotDirty;
    u8 posDirty;
} Obj;

void MTX_Identity44_(Mtx44 *m);
void func_01ffa9e8(Mtx44 *m, Angle *rot);
void func_01ffac98(Mtx44 *m, Vec3 *in, Vec3 *out);
void func_02007f24(s32 *out, f64 v);
u32 func_0205d060(void *pad, s32 mask, s32 mode);

extern u8 data_021027f8[];

static inline void vec_sub(Vec3 *d, const Vec3 *a, const Vec3 *b) {
    s32 z = a->z - b->z;
    s32 y = a->y - b->y;
    s32 x = a->x - b->x;
    d->x  = x;
    d->y  = y;
    d->z  = z;
}

static inline void vec_add(Vec3 *d, const Vec3 *a, const Vec3 *b) {
    s32 z = a->z + b->z;
    s32 y = a->y + b->y;
    s32 x = a->x + b->x;
    d->x  = x;
    d->y  = y;
    d->z  = z;
}

/* Nonzero when any component differs. */
static inline s32 vec_ne(const Vec3 *a, const Vec3 *b) {
    if (a->x != b->x || a->y != b->y || a->z != b->z) {
        return 1;
    }
    return 0;
}

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

static inline void cam_set_pos(Obj *c, const Vec3 *p) {
    if (vec_ne(&c->pos, p)) {
        vec_set(&c->pos, p);
        c->posDirty = 1;
    }
}

/* Q12 radians -> 16-bit angle: rad * 65536 / (2 * pi * 4096), rounded (0x28BE60DB9391 / 2^44). */
static inline Angle rad_to_angle(s32 rad, Angle *a) {
    a->v = (s32) (((s64) rad * 0x28BE60DB9391LL + 0x80000000000LL) >> 44);
    return *a;
}

static inline Angle angle_add(Angle *a, Angle b, Angle *r) {
    r->v = a->v + b.v;
    return *r;
}

static inline Angle angle_sub(Angle *a, Angle b, Angle *r) {
    r->v = a->v - b.v;
    return *r;
}

static inline void cam_set_rx(Obj *c, Angle a) {
    if (c->rx.v != a.v) {
        c->rx.v     = a.v;
        c->rotDirty = 1;
    }
}

static inline void cam_set_ry(Obj *c, Angle a) {
    if (c->ry.v != a.v) {
        c->ry.v     = a.v;
        c->rotDirty = 1;
    }
}

void func_020a41f4(Obj *c) {
    Vec3 fwd;
    Vec3 side;
    Mtx44 m;
    Vec3 up;

    fwd.x  = 0;
    fwd.y  = 0;
    fwd.z  = MOVE_STEP;
    side.x = MOVE_STEP;
    side.y = 0;
    side.z = 0;
    MTX_Identity44_(&m);
    func_01ffa9e8(&m, &c->rx);
    func_01ffac98(&m, &fwd, &fwd);
    func_01ffac98(&m, &side, &side);

    if (func_0205d060(data_021027f8, PAD_L, PAD_CURRENT)) {
        Vec3 p;
        vec_sub(&p, &c->pos, &fwd);
        cam_set_pos(c, &p);
    } else if (func_0205d060(data_021027f8, PAD_R, PAD_CURRENT)) {
        Vec3 p;
        vec_add(&p, &c->pos, &fwd);
        cam_set_pos(c, &p);
    }

    if (func_0205d060(data_021027f8, PAD_LEFT, PAD_CURRENT)) {
        if (func_0205d060(data_021027f8, PAD_Y, PAD_CURRENT)) {
            Vec3 p;
            vec_sub(&p, &c->pos, &side);
            cam_set_pos(c, &p);
        } else {
            s32 rad;
            Angle sum;
            Angle delta;
            func_02007f24(&rad, 0.04); /* turn step in radians */
            cam_set_ry(c, angle_add(&c->ry, rad_to_angle(rad, &delta), &sum));
        }
    } else if (func_0205d060(data_021027f8, PAD_RIGHT, PAD_CURRENT)) {
        if (func_0205d060(data_021027f8, PAD_Y, PAD_CURRENT)) {
            Vec3 p;
            vec_add(&p, &c->pos, &side);
            cam_set_pos(c, &p);
        } else {
            s32 rad;
            Angle sum;
            Angle delta;
            func_02007f24(&rad, 0.04); /* turn step in radians */
            cam_set_ry(c, angle_sub(&c->ry, rad_to_angle(rad, &delta), &sum));
        }
    }

    up.x = 0;
    up.y = MOVE_STEP;
    up.z = 0;
    if (func_0205d060(data_021027f8, PAD_DOWN, PAD_CURRENT)) {
        if (func_0205d060(data_021027f8, PAD_Y, PAD_CURRENT)) {
            Vec3 p;
            vec_sub(&p, &c->pos, &up);
            cam_set_pos(c, &p);
        } else {
            s32 rad;
            Angle sum;
            Angle delta;
            func_02007f24(&rad, 0.03); /* pitch step in radians */
            cam_set_rx(c, angle_sub(&c->rx, rad_to_angle(rad, &delta), &sum));
        }
    } else if (func_0205d060(data_021027f8, PAD_UP, PAD_CURRENT)) {
        if (func_0205d060(data_021027f8, PAD_Y, PAD_CURRENT)) {
            Vec3 p;
            vec_add(&p, &c->pos, &up);
            cam_set_pos(c, &p);
        } else {
            s32 rad;
            Angle sum;
            Angle delta;
            func_02007f24(&rad, 0.03); /* pitch step in radians */
            cam_set_rx(c, angle_add(&c->rx, rad_to_angle(rad, &delta), &sum));
        }
    }
}
