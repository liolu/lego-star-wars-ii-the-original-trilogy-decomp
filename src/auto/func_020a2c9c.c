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

typedef struct {
    s32 v;
} Fx;

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
    u8 pad358[0x20];
    u8 f378;
    u8 pad379[0x3];
    Fx speed;
    s32 maxSpeed;
    s32 accel;
    s32 snap;
    Fx factor;
} Unk;

extern s32 FX_Div(s32, s32);
void func_020a389c(Unk *, Fx);
extern Vec3 data_020ff9b4;

static inline s32 fx_mul(s32 a, Fx b) {
    return (s32) (((s64) a * b.v + FX_HALF) >> FX_SHIFT);
}

static inline s32 fx_div(s32 a, Fx b) {
    return FX_Div(a, b.v);
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

static inline s32 fx_mul_p(s32 a, const Fx *b) {
    return (s32) (((s64) a * b->v + FX_HALF) >> FX_SHIFT);
}

static inline void vec_scale(Vec3 *v, const Fx *s) {
    Fx x, y, z;

    x    = *s;
    v->x = fx_mul_p(v->x, &x);
    y    = *s;
    v->y = fx_mul_p(v->y, &y);
    z    = *s;
    v->z = fx_mul_p(v->z, &z);
}

static inline void vec_add(Vec3 *d, const Vec3 *a, const Vec3 *b) {
    s32 x, y, z;

    z    = a->z + b->z;
    y    = a->y + b->y;
    x    = a->x + b->x;
    d->x = x;
    d->y = y;
    d->z = z;
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

static inline Fx vec_len(const Vec3 *v) {
    Fx r;

    r.v = vec_mag(v);
    return r;
}

static inline Fx *vec_len_p(Fx *r, const Vec3 *v) {
    r->v = vec_mag(v);
    return r;
}

static inline s32 fx_div_p(const Fx *a, s32 b) {
    return FX_Div(a->v, b);
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

/* Rounded length from three squared components: sqrt(4 * (xx + yy + zz)) = 2|v|, then (r + 1) / 2. */
static inline s32 mag_sq3(s64 xx, s64 yy, s64 zz) {
    s64 t = xx;
    t += yy;
    t += zz;
    (*(vu16 *) REG_SQRTCNT)   = SQRT_MODE_64;
    (*(u64 *) REG_SQRT_PARAM) = t << 2;
    while ((*(vu16 *) REG_SQRTCNT) & MATH_BUSY) {
    }
    return (s32) ((*(vu32 *) REG_SQRT_RESULT) + 1) >> 1;
}

/* Move pos (0x28) toward target (0x34c) along the target-to-pos line, Q12 units.
 *
 * Runs only when f378 is set and f23f/f240 are both clear. With d = pos - target and dist = |d| (rounded):
 * - speed (0x37c) < dist: a zero speed becomes dist when dist <= snap (0x388), else accel (0x384); a non-zero
 *   speed is multiplied by factor (0x38c, Q12); the result is capped at maxSpeed (0x380). If dist still exceeds
 *   speed, pos becomes target + unit(d) * speed (data_020ff9b4 as the direction when d is zero), f49 is set when
 *   pos changed, and func_020a389c gets |unit(d) * speed| / dist (Q12 FX_Div); otherwise speed is cleared.
 * - otherwise, when 0 < dist < maxSpeed: speed is divided by factor (Q12 FX_Div) and cleared when it is <= accel
 *   or >= dist; else pos is placed and func_020a389c called the same way.
 * speed is cleared whenever dist is 0. The squared components xx/yy/zz of d are kept for the second |d|. */
void func_020a2c9c(Unk *p) {
    Vec3 d;
    s64 xx, yy, zz;
    s32 dist;
    Vec3 dir;
    Vec3 dir2;
    Vec3 next;
    Vec3 next2;
    Fx ratio;
    Fx ratio2;
    Fx len;
    Fx len2;

    if (p->f378 == 0) {
        return;
    }
    if (p->f23f != 0 || p->f240 != 0) {
        return;
    }
    d.z  = p->pos.z - p->target.z;
    d.y  = p->pos.y - p->target.y;
    d.x  = p->pos.x - p->target.x;
    xx   = (s64) d.x * d.x;
    yy   = (s64) d.y * d.y;
    zz   = (s64) d.z * d.z;
    dist = mag_sq3(xx, yy, zz);
    if (p->speed.v < dist) {
        if (p->speed.v == 0) {
            if (dist <= p->snap) {
                p->speed.v = dist;
            } else {
                p->speed.v += p->accel;
            }
        } else {
            p->speed.v = fx_mul(p->speed.v, p->factor);
        }
        if (p->speed.v > p->maxSpeed) {
            p->speed.v = p->maxSpeed;
        }
        if (dist > p->speed.v) {
            dir.x = d.x;
            dir.y = d.y;
            dir.z = d.z;
            if (d.x == 0 && d.y == 0 && d.z == 0) {
                vec_set(&dir, &data_020ff9b4);
            } else {
                vec_normalize(&dir, &dir);
            }
            vec_scale(&dir, &p->speed);
            vec_add(&next, &p->target, &dir);
            if (vec_ne(&p->pos, &next)) {
                vec_set(&p->pos, &next);
                p->f49 = 1;
            }
            {
                Fx *lp  = vec_len_p(&len, &dir);
                ratio.v = FX_Div(lp->v, mag_sq3(xx, yy, zz));
            }
            func_020a389c(p, ratio);
        } else {
            p->speed.v = 0;
        }
    } else if (dist < p->maxSpeed && dist != 0) {
        p->speed.v = fx_div(p->speed.v, p->factor);
        if (p->speed.v <= p->accel) {
            p->speed.v = 0;
        } else if (p->speed.v >= dist) {
            p->speed.v = 0;
        } else {
            dir2.x = d.x;
            dir2.y = d.y;
            dir2.z = d.z;
            if (d.x == 0 && d.y == 0 && d.z == 0) {
                vec_set(&dir2, &data_020ff9b4);
            } else {
                vec_normalize(&dir2, &dir2);
            }
            vec_scale(&dir2, &p->speed);
            vec_add(&next2, &p->target, &dir2);
            if (vec_ne(&p->pos, &next2)) {
                vec_set(&p->pos, &next2);
                p->f49 = 1;
            }
            {
                Fx *lp   = vec_len_p(&len2, &dir2);
                ratio2.v = FX_Div(lp->v, mag_sq3(xx, yy, zz));
            }
            func_020a389c(p, ratio2);
        }
    }
    if (dist == 0) {
        p->speed.v = 0;
    }
}
