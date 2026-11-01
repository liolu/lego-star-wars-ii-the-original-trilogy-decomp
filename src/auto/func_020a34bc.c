#include "m2c.h"

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

/* One-word Q12 value passed by value: the copies through it reproduce the target's stack temporaries. */
typedef struct {
    s32 v;
} Fx;

extern Vec3 data_020ff9b4; /* zero vector */

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

/* Rounded length: sqrt(4 * |v|^2) = 2|v|, then (r + 1) / 2. */
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

/* d = s / |s| as a Q12 unit vector: (2^56 / |s|^2) * sqrt(4 |s|^2) * s / 2^45 = s * 2^12 / |s|, rounded.
 * Callers handle the zero vector first. */
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

static inline s32 fx_mul_p(s32 a, const Fx *b) {
    return (s32) (((s64) a * (s64) b->v + (s64) FX_HALF) >> FX_SHIFT);
}

typedef struct {
    u8 pad00[0x28];
    Vec3 pos;
    u8 pad34[0x15];
    u8 f49;
    u8 pad4a[0x1f5];
    u8 f23f;
    u8 f240;
    u8 pad241[0x10b];
    Vec3 target;
    u8 pad358[0x8];
    u8 f360;
    u8 pad361[3];
    Fx limit;
} Self;

s32 FX_Div(s32, s32);
void func_020a389c(Self *, Fx);

static inline u32 vec_ne_u(const Vec3 *a, const Vec3 *b) {
    u32 ne = 0;

    if (a->x != b->x || a->y != b->y || a->z != b->z) {
        ne = 1;
    }
    return ne;
}

static inline void vec_sub(Vec3 *d, const Vec3 *a, const Vec3 *b) {
    s32 z = a->z - b->z;
    s32 y = a->y - b->y;
    s32 x = a->x - b->x;
    d->x  = x;
    d->y  = y;
    d->z  = z;
}

static inline s32 mag3(s32 x, s32 y, s32 z) {
    s64 t = (s64) x * x;
    t += (s64) y * y;
    t += (s64) z * z;
    (*(vu16 *) REG_SQRTCNT)   = SQRT_MODE_64;
    (*(u64 *) REG_SQRT_PARAM) = t << 2;
    while ((*(vu16 *) REG_SQRTCNT) & MATH_BUSY) {
    }
    return (s32) ((*(vu32 *) REG_SQRT_RESULT) + 1) >> 1;
}

/* When f360 is set and neither f23f nor f240 is, and pos (0x28) lies farther than limit (0x364,
 * Q12) from target (0x34c), move pos onto the sphere of radius limit around target along the
 * target-to-pos line (pos = target - unit(target - pos) * limit), set f49 if pos changed, then pass
 * func_020a389c the ratio |scaled dir| / old distance (Q12 FX_Div). */
void func_020a34bc(Self *self) {
    s32 dz, dy, dx;
    Vec3 dir;
    Vec3 pos;
    Fx ratio;
    Fx len;
    Fx *len_p;
    Fx lim_z, lim_y, lim_x;

    if (!self->f360) {
        return;
    }
    if (self->f23f != 0 || self->f240 != 0) {
        return;
    }
    dy = self->target.y - self->pos.y;
    dx = self->target.x - self->pos.x;
    dz = self->target.z - self->pos.z;
    if (mag3(dx, dy, dz) <= self->limit.v) {
        return;
    }
    dir.x = dx;
    dir.y = dy;
    dir.z = dz;
    if (dx == 0 && dy == 0 && dz == 0) {
        vec_set(&dir, &data_020ff9b4);
    } else {
        vec_normalize(&dir, &dir);
    }
    lim_x = self->limit;
    dir.x = fx_mul_p(dir.x, &lim_x);
    lim_y = self->limit;
    dir.y = fx_mul_p(dir.y, &lim_y);
    lim_z = self->limit;
    dir.z = fx_mul_p(dir.z, &lim_z);
    vec_sub(&pos, &self->target, &dir);
    if (vec_ne_u(&self->pos, &pos)) {
        vec_set(&self->pos, &pos);
        self->f49 = 1;
    }
    len.v   = vec_mag(&dir);
    len_p   = &len;
    ratio.v = FX_Div(len_p->v, mag3(dx, dy, dz));
    func_020a389c(self, ratio);
}
