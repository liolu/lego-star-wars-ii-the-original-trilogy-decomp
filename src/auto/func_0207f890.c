#include "m2c.h"

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct {
    s32 v;
} Fx;

typedef struct {
    u16 v;
} Angle;

typedef struct {
    u8 pad0[8];
    u32 flags;
    u16 flags2;
    u8 pad1[0xB0 - 0xE];
    Vec3 pos;
    u8 pad2[0xCA - 0xBC];
    Angle yaw;
    u8 pad3[0xD0 - 0xCC];
    u8 yaw_dirty;
    u8 pad4[0x6B4 - 0xD1];
    Angle yaw2;
} Ent;

typedef struct {
    u8 pad0[4];
    Ent *ent;
    s32 path_id;
    u8 pad1[4];
    Fx speed;
    u32 point_idx;
    s32 arg0;
    s32 arg1;
    u16 timer;
    u8 face;
    u8 flat;
    u8 no_path;
    u8 pad4[3];
    Vec3 target;
    Vec3 step;
} PathFollower;

typedef struct {
    u8 pad0[0x68];
    void *game;
} Globals;

extern Globals data_0210930c;
extern Vec3 data_020ff9b4;

extern void func_020906a8(Vec3 *out, void *game, s32 path_id, u32 point_idx);
extern s16 func_020985d8(Fx a, Fx b);
extern s32 FX_Div(s32, s32);

enum {
    REG_DIVCNT      = 0x04000280,
    REG_DIV_NUMER   = 0x04000290,
    REG_DIV_DENOM   = 0x04000298,
    REG_DIV_RESULT  = 0x040002A0,
    REG_SQRTCNT     = 0x040002B0,
    REG_SQRT_RESULT = 0x040002B4,
    REG_SQRT_PARAM  = 0x040002B8,
    DIV_MODE_64_64  = 2,
    SQRT_MODE_64    = 1,
    HW_BUSY         = 0x8000,
    FLAT_GATE_FLAG  = 0x20, /* entity flag that must be set for the flat (y = 0) step; meaning unknown */
    YAW2_COPY_MASK  = 3,    /* entity flags2 bits that make the yaw also go to +0x6B4; meaning unknown */
    FX_SHIFT        = 12,
    FX_ROUND_UP     = 0xFFF,
};

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

static inline void vec_sub(Vec3 *d, const Vec3 *a, const Vec3 *b) {
    s32 z = a->z - b->z;
    s32 y = a->y - b->y;
    s32 x = a->x - b->x;
    d->x  = x;
    d->y  = y;
    d->z  = z;
}

static inline Fx fx(s32 v) {
    Fx r;
    r.v = v;
    return r;
}

static inline s32 fx_mul(s32 a, Fx b) {
    return (s32) (((s64) a * b.v + 0x800) >> 12);
}

static inline void vec_normalize(const Vec3 *src, Vec3 *dst) {
    s64 t;
    s32 sqrt;
    s64 div;

    t = (s64) src->x * src->x;
    t += (s64) src->y * src->y;
    t += (s64) src->z * src->z;
    *(volatile u16 *) REG_DIVCNT    = DIV_MODE_64_64;
    *(volatile u64 *) REG_DIV_NUMER = 1LL << 56;
    *(volatile u64 *) REG_DIV_DENOM = t;
    *(volatile u16 *) REG_SQRTCNT   = SQRT_MODE_64;
    *(u64 *) REG_SQRT_PARAM         = t << 2;
    while (*(volatile u16 *) REG_SQRTCNT & HW_BUSY) {
    }
    sqrt = *(volatile s32 *) REG_SQRT_RESULT;
    while (*(volatile u16 *) REG_DIVCNT & HW_BUSY) {
    }
    div = *(s64 *) REG_DIV_RESULT;
    div *= sqrt;
    dst->x = (s32) ((div * src->x + (1LL << 44)) >> 45);
    dst->y = (s32) ((div * src->y + (1LL << 44)) >> 45);
    dst->z = (s32) ((div * src->z + (1LL << 44)) >> 45);
}

static inline void vec_scale(Vec3 *v, const Fx *len) {
    Fx x, y, z;

    x    = *len;
    v->x = fx_mul(v->x, x);
    y    = *len;
    v->y = fx_mul(v->y, y);
    z    = *len;
    v->z = fx_mul(v->z, z);
}

static inline s32 vec_mag(const Vec3 *v) {
    s64 t;

    t = (s64) v->x * v->x;
    t += (s64) v->y * v->y;
    t += (s64) v->z * v->z;
    *(volatile u16 *) REG_SQRTCNT = SQRT_MODE_64;
    *(u64 *) REG_SQRT_PARAM       = t << 2;
    while (*(volatile u16 *) REG_SQRTCNT & HW_BUSY) {
    }
    return (*(volatile s32 *) REG_SQRT_RESULT + 1) >> 1;
}

static inline Angle angle_of(Fx x, Fx z) {
    Angle a;
    a.v = func_020985d8(x, z);
    return a;
}

static inline void set_yaw(Ent *e, Angle a) {
    if (e->yaw.v != a.v) {
        e->yaw       = a;
        e->yaw_dirty = 1;
    }
}

static inline Angle get_yaw(Ent *e) {
    Angle a;
    a.v = e->yaw.v;
    return a;
}

/*
 * Aim a path follower at its current waypoint (called by func_0207f6b8 at setup and by func_0207f70c
 * after each waypoint is reached).
 * 1. Unless no_path: fetch waypoint point_idx of path path_id (func_020906a8) into target.
 * 2. delta = target - entity position; with the flat flag and entity flag 0x20 set, delta.y = 0.
 * 3. step = normalized delta (the zero-vector global if delta is zero) scaled by speed; func_0207f70c
 *    copies step to the entity each call.
 * 4. timer = ceil(|delta| / speed) (fx 20.12): the calls left until arrival; func_0207f70c counts it down.
 * 5. If face: entity yaw = func_020985d8(delta.x, delta.z), dirty flag set on change; with any flags2 & 3
 *    bit set the yaw is also copied to +0x6B4.
 */
void func_0207f890(PathFollower *s) {
    Vec3 delta;
    Fx dz, dx;
    Ent *ent;

    if (s->no_path == 0) {
        Vec3 waypoint;
        func_020906a8(&waypoint, data_0210930c.game, s->path_id, s->point_idx);
        vec_set(&s->target, &waypoint);
    }
    vec_sub(&delta, &s->target, &s->ent->pos);
    if (s->flat != 0 && (s->ent->flags & FLAT_GATE_FLAG) == FLAT_GATE_FLAG) {
        delta.y = 0;
    }
    vec_set(&s->step, &delta);
    if (s->step.x == 0 && s->step.y == 0 && s->step.z == 0) {
        vec_set(&s->step, &data_020ff9b4);
    } else {
        vec_normalize(&s->step, &s->step);
    }
    vec_scale(&s->step, &s->speed);
    s->timer = (FX_Div(vec_mag(&delta), s->speed.v) + FX_ROUND_UP) >> FX_SHIFT;
    if (s->face == 0) {
        return;
    }
    dz = fx(delta.z);
    dx = fx(delta.x);
    set_yaw(s->ent, angle_of(dx, dz));
    ent = s->ent;
    if ((ent->flags2 & YAW2_COPY_MASK) == 0) {
        return;
    }
    ent->yaw2 = get_yaw(ent);
}
