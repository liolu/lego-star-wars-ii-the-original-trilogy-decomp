#include "m2c.h"

typedef volatile u64 vu64;
typedef volatile u32 vu32;
typedef volatile u16 vu16;

enum {
    REG_DIVCNT      = 0x04000280,
    REG_DIV_NUMER   = 0x04000290,
    REG_DIV_DENOM   = 0x04000298,
    REG_DIV_RESULT  = 0x040002a0,
    REG_SQRTCNT     = 0x040002b0,
    REG_SQRT_RESULT = 0x040002b4,
    REG_SQRT_PARAM  = 0x040002b8
};

enum {
    DIV_MODE_64_64 = 2,
    SQRT_MODE_64   = 1,
    MATH_BUSY      = 0x8000
};

enum {
    FX_SHIFT = 12,
    FX_HALF  = 0x800
};

enum {
    NEAR_TARGET_DIST = 0x6000, /* 6.0 in fx32 */
    FLAG_NEAR_TARGET = 0x200
};

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct {
    s32 v;
} Fx;

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} VecFx32;

extern Vec3 data_020ff9b4;

void func_02007f24(s32 *out, f64 x);
void func_02007eb4(u32 *out, s32 *a1, s32 *a2);

static inline void vec_sub(Vec3 *d, const Vec3 *a, const Vec3 *b) {
    s32 z = a->z - b->z;
    s32 y = a->y - b->y;
    s32 x = a->x - b->x;
    d->x  = x;
    d->y  = y;
    d->z  = z;
}

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

static inline void VEC_Normalize(const VecFx32 *s, VecFx32 *d) {
    s64 t;
    s32 sqrt;
    s64 div;

    t = (s64) s->x * s->x;
    t += (s64) s->y * s->y;
    t += (s64) s->z * s->z;

    (*(vu16 *) REG_DIVCNT)    = DIV_MODE_64_64;
    (*(vu64 *) REG_DIV_NUMER) = 1LL << 56;
    (*(vu64 *) REG_DIV_DENOM) = t;

    (*(vu16 *) REG_SQRTCNT)   = SQRT_MODE_64;
    (*(u64 *) REG_SQRT_PARAM) = t << 2;

    while ((*(vu16 *) REG_SQRTCNT) & MATH_BUSY) {
    }
    sqrt = (s32) (*(vu32 *) REG_SQRT_RESULT);

    while ((*(vu16 *) REG_DIVCNT) & MATH_BUSY) {
    }
    div = (s64) (*(u64 *) REG_DIV_RESULT);

    div *= sqrt;

    d->x = (s32) ((div * s->x + (1LL << 44)) >> 45);
    d->y = (s32) ((div * s->y + (1LL << 44)) >> 45);
    d->z = (s32) ((div * s->z + (1LL << 44)) >> 45);
}

static inline s32 fx_mul(s32 a, Fx b) {
    return (s32) (((s64) a * b.v + FX_HALF) >> FX_SHIFT);
}

static inline void vec_scale_x(Vec3 *v, const Fx *s) {
    v->x = fx_mul(v->x, *s);
}

static inline void vec_scale_xy(Vec3 *v, const Fx *s) {
    vec_scale_x(v, s);
    v->y = fx_mul(v->y, *s);
}

static inline void vec_scale(Vec3 *v, const Fx *s) {
    vec_scale_xy(v, s);
    v->z = fx_mul(v->z, *s);
}

static inline void vec_set_xyz(Vec3 *d, s32 x, s32 y, s32 z) {
    d->x = x;
    d->y = y;
    d->z = z;
}

// Steps the object at arg0 toward the fx32 position `pos` in the XZ plane.
// Returns at once when pos equals the object's position (+0xb0). Otherwise
// dir = pos - position with y zeroed, normalised (data_020ff9b4 when zero),
// and step = dir * 2.0 (func_02007f24 converts the double to fx32).
// func_02007eb4 gives the XZ distance to pos: below 6.0 the object's flag
// 0x200 (+8) is set and +0x60 gets the full offset pos - position; otherwise
// +0x60 gets step and the flag is cleared.
void func_02006b74(void *arg0, Vec3 pos) {
    Vec3 *cur = &M2C_FIELD(arg0, Vec3 *, 0xb0);
    Vec3 dir;
    Vec3 step;
    Vec3 rest;
    u32 dist;
    Fx step_len;

    {
        int px = pos.x;
        int py = pos.y;
        int pz = pos.z;
        int cx = cur->x;
        int cy = cur->y;
        int cz = cur->z;

        if (px == cx && py == cy && pz == cz) {
            return;
        }
        vec_set_xyz(&dir, px - cx, py - cy, pz - cz);
        dir.y = 0;
    }

    if (dir.x == 0 && dir.y == 0 && dir.z == 0) {
        vec_set(&dir, &data_020ff9b4);
    } else {
        VEC_Normalize((const VecFx32 *) &dir, (VecFx32 *) &dir);
    }

    step.x = dir.x;
    step.y = dir.y;
    step.z = dir.z;

    func_02007f24(&step_len.v, 2.0);
    vec_scale(&step, &step_len);

    func_02007eb4(&dist, &cur->x, &pos.x);

    if ((s32) dist < NEAR_TARGET_DIST) {
        M2C_FIELD(arg0, u32 *, 8) |= FLAG_NEAR_TARGET;
        vec_sub(&rest, &pos, cur);
        vec_set(&M2C_FIELD(arg0, Vec3 *, 0x60), &rest);
    } else {
        vec_set(&M2C_FIELD(arg0, Vec3 *, 0x60), &step);
        M2C_FIELD(arg0, u32 *, 8) &= ~FLAG_NEAR_TARGET;
    }
}
