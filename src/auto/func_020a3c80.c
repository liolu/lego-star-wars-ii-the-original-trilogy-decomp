#include "m2c.h"

/* Add a scaled Q12 delta to a vector and clamp its length, or shrink the vector when the delta is zero.
 *
 * With a nonzero delta: *v += delta * amount (Q12 multiply). If the new length reaches limit, *v is replaced by
 * its own direction scaled to exactly limit (a zero vector stays zero).
 * With a zero delta: the length of *v is reduced by amount. If that would go negative, *v becomes the zero
 * vector; otherwise *v keeps its direction at the shorter length.
 *
 * Lengths use the hardware square-root unit; unit vectors use the divider and square-root units together.
 * arg0 is not used. */
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

static inline s32 fx_mul(s32 a, s32 b) {
    return (s32) (((s64) a * b + FX_HALF) >> FX_SHIFT);
}

static inline Vec3 *vec_init(Vec3 *v, s32 x, s32 y, s32 z) {
    v->x = x;
    v->y = y;
    v->z = z;
    return v;
}

static inline void vec_add(Vec3 *d, const Vec3 *a, const Vec3 *b) {
    d->x = a->x + b->x;
    d->y = a->y + b->y;
    d->z = a->z + b->z;
}

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

static inline s32 vec_ne(const Vec3 *a, const Vec3 *b) {
    return a->x != b->x || a->y != b->y || a->z != b->z;
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

static inline Fx vec_len(const Vec3 *v) {
    Fx r;

    r.v = vec_mag(v);
    return r;
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
    return (s32) (((s64) a * b->v + FX_HALF) >> FX_SHIFT);
}

/* Set *v to length (len - amount) along its own direction; zero it if that length is negative. */
static inline void vec_shorten(Vec3 *v, Fx len, s32 amount) {
    Fx newLen;
    Fx x, y, z;

    newLen = len;
    newLen.v -= amount;
    if (newLen.v < 0) {
        vec_set(v, &data_020ff9b4);
        return;
    }
    if (v->x == 0 && v->y == 0 && v->z == 0) {
        vec_set(v, &data_020ff9b4);
    } else {
        vec_normalize(v, v);
    }
    x    = newLen;
    v->x = fx_mul_p(v->x, &x);
    y    = newLen;
    v->y = fx_mul_p(v->y, &y);
    z    = newLen;
    v->z = fx_mul_p(v->z, &z);
}

void func_020a3c80(void *arg0, Vec3 *v, Vec3 delta, s32 amount, s32 limit) {
    if (vec_ne(&delta, &data_020ff9b4)) {
        Vec3 scaled;

        vec_add(v, v, vec_init(&scaled, fx_mul(delta.x, amount), fx_mul(delta.y, amount), fx_mul(delta.z, amount)));
        if (vec_mag(v) >= limit) {
            Vec3 unit;
            Vec3 clamped;
            s32 x, y, z;

            /* The zero test reads the copied component values, not the stored copy. */
            unit.x = x = v->x;
            unit.y = y = v->y;
            unit.z = z = v->z;
            if (x == 0 && y == 0 && z == 0) {
                vec_set(&unit, &data_020ff9b4);
            } else {
                vec_normalize(&unit, &unit);
            }
            clamped.x = fx_mul(unit.x, limit);
            clamped.y = fx_mul(unit.y, limit);
            clamped.z = fx_mul(unit.z, limit);
            vec_set(v, &clamped);
        }
    } else {
        vec_shorten(v, vec_len(v), amount);
    }
}
