#include "m2c.h"

/* Move self->pos one step toward self->target (Q12 vectors).
 *
 * If the remaining distance |target - pos| is at most max_step, pos snaps to target (pos_changed is set when it
 * actually moved) and func_020a99bc(self, 0) sets self's byte at 0x208 + the current slot index to 0 (directly, or
 * through func_020b04f0 when bit 0x40 of data_027e02a8 is set).
 * Otherwise pos advances by the unit direction scaled by max_step, and pos_changed is set if pos changed.
 *
 * Lengths use the hardware square-root unit; the unit vector uses the divider and square-root units together. */

enum {
    REG_DIVCNT      = 0x04000280,
    REG_DIV_NUMER   = 0x04000290,
    REG_DIV_DENOM   = 0x04000298,
    REG_DIV_RESULT  = 0x040002a0,
    REG_SQRTCNT     = 0x040002b0,
    REG_SQRT_RESULT = 0x040002b4,
    REG_SQRT_PARAM  = 0x040002b8,
    DIV_MODE_64_64  = 2,
    SQRT_MODE_64    = 1,
    MATH_BUSY       = 0x8000,
    FX_SHIFT        = 12,
    FX_HALF         = 0x800,
};

typedef volatile u16 vu16;
typedef volatile u32 vu32;
typedef volatile u64 vu64;

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct {
    s32 v;
} Fx;

typedef struct {
    u8 pad00[0x28];
    Vec3 pos;
    u8 pad34[0x15];
    u8 pos_changed;
    u8 pad4a[0x1c2];
    Vec3 target;
    Fx max_step;
} Mover;

extern Vec3 data_020ff9b4; /* zero vector */

void func_020a99bc(void *arg0, u8 arg1);

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

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

static inline s32 vec_ne(const Vec3 *a, const Vec3 *b) {
    if (a->x != b->x || a->y != b->y || a->z != b->z) {
        return 1;
    }
    return 0;
}

static inline s32 vec_mag(const Vec3 *v) {
    s64 t = (s64) v->x * v->x;
    t += (s64) v->y * v->y;
    t += (s64) v->z * v->z;
    (*(vu16 *) REG_SQRTCNT)   = SQRT_MODE_64;
    (*(u64 *) REG_SQRT_PARAM) = t << 2;
    while ((*(vu16 *) REG_SQRTCNT) & MATH_BUSY) {
    }
    return (s32) ((*(vu32 *) REG_SQRT_RESULT) + 1) >> 1;
}

static inline void vec_normalize(Vec3 *d, const Vec3 *s) {
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

static inline void vec_unit_self(Vec3 *v) {
    if (v->x == 0 && v->y == 0 && v->z == 0) {
        vec_set(v, &data_020ff9b4);
    } else {
        vec_normalize(v, v);
    }
}

static inline s32 fx_mul_p(s32 a, const Fx *b) {
    return (s32) (((s64) a * b->v + FX_HALF) >> FX_SHIFT);
}

void func_020a9a30(Mover *self) {
    Vec3 step;
    Vec3 next;
    Fx z, y, x;

    vec_sub(&step, &self->target, &self->pos);
    if (self->max_step.v >= vec_mag(&step)) {
        if (vec_ne(&self->pos, &self->target)) {
            vec_set(&self->pos, &self->target);
            self->pos_changed = 1;
        }
        func_020a99bc(self, 0);
        return;
    }
    vec_unit_self(&step);
    x      = self->max_step;
    step.x = fx_mul_p(step.x, &x);
    y      = self->max_step;
    step.y = fx_mul_p(step.y, &y);
    z      = self->max_step;
    step.z = fx_mul_p(step.z, &z);
    vec_add(&next, &self->pos, &step);
    if (vec_ne(&self->pos, &next)) {
        vec_set(&self->pos, &next);
        self->pos_changed = 1;
    }
}
