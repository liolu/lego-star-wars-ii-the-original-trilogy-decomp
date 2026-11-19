#include "m2c.h"

typedef volatile u16 vu16;
typedef volatile u32 vu32;
typedef volatile u64 vu64;

typedef struct {
    s32 x, y, z;
} Vec3;

typedef struct {
    s32 v;
} Fx;

/* 20.12 fixed point */
enum {
    FX_SHIFT = 12,
    FX_ONE   = 0x1000,
    FX_HALF  = 0x800
};

/* NDS ARM9 divider / square-root unit */
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
    DIVSQRT_BUSY = 0x8000
};

extern u8 func_02067b54(void *list);
extern void func_020a6a9c(s32 *out, void *self, Vec3 *a, Vec3 *pos, Vec3 *dir);
extern void func_020a6bb4(void *self, void *list, Vec3 *o1, Vec3 *o2, Vec3 *o3, u8 idx1, u8 idx2);
extern Vec3 data_020ff9b4;

static inline s32 fx_mul_p(s32 a, const Fx *b) {
    return (s32) (((s64) a * b->v + FX_HALF) >> FX_SHIFT);
}

static inline s32 fx_mul_v(s32 a, Fx b) {
    Fx t;

    t = b;
    return fx_mul_p(a, &t);
}

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

static inline s32 sqrt_len(s64 t) {
    (*(vu16 *) REG_SQRTCNT)   = 1;
    (*(u64 *) REG_SQRT_PARAM) = t << 2;
    while ((*(vu16 *) REG_SQRTCNT) & DIVSQRT_BUSY) {
    }
    return (s32) ((*(vu32 *) REG_SQRT_RESULT) + 1) >> 1;
}

static inline s32 vec_mag(const Vec3 *v) {
    s64 t = (s64) v->x * v->x;

    t += (s64) v->y * v->y;
    t += (s64) v->z * v->z;
    return sqrt_len(t);
}

static inline s32 vec_dist(const Vec3 *p, const Vec3 *q) {
    s32 dx = p->x - q->x;
    s32 dy = p->y - q->y;
    s32 dz = p->z - q->z;
    s64 t  = (s64) dz * dz;

    t += (s64) dx * dx + (s64) dy * dy;
    return sqrt_len(t);
}

static inline void vec_normalize(Vec3 *d, const Vec3 *s) {
    s64 t;
    s32 sqrt;
    s64 div;

    t = (s64) s->x * s->x;
    t += (s64) s->y * s->y;
    t += (s64) s->z * s->z;

    (*(vu16 *) REG_DIVCNT)    = 2;
    (*(vu64 *) REG_DIV_NUMER) = 1LL << 56;
    (*(vu64 *) REG_DIV_DENOM) = t;

    (*(vu16 *) REG_SQRTCNT)   = 1;
    (*(u64 *) REG_SQRT_PARAM) = t << 2;

    while ((*(vu16 *) REG_SQRTCNT) & DIVSQRT_BUSY) {
    }
    sqrt = (s32) (*(vu32 *) REG_SQRT_RESULT);

    while ((*(vu16 *) REG_DIVCNT) & DIVSQRT_BUSY) {
    }
    div = (s64) (*(u64 *) REG_DIV_RESULT);

    div *= sqrt;

    d->x = (s32) ((div * s->x + (1LL << 44)) >> 45);
    d->y = (s32) ((div * s->y + (1LL << 44)) >> 45);
    d->z = (s32) ((div * s->z + (1LL << 44)) >> 45);
}

static inline void vec_sub(Vec3 *d, const Vec3 *a, const Vec3 *b) {
    s32 z = a->z - b->z;
    s32 y = a->y - b->y;
    s32 x = a->x - b->x;
    d->x  = x;
    d->y  = y;
    d->z  = z;
}

/* d = unit(d) * len */
static inline void vec_set_len(Vec3 *d, Fx len) {
    Fx tx, ty, tz;

    if (d->x == 0 && d->y == 0 && d->z == 0) {
        vec_set(d, &data_020ff9b4);
    } else {
        vec_normalize(d, d);
    }
    tx   = len;
    d->x = fx_mul_p(d->x, &tx);
    ty   = len;
    d->y = fx_mul_p(d->y, &ty);
    tz   = len;
    d->z = fx_mul_p(d->z, &tz);
}

/*
 * Moves pos along the node path by the signed step at self+0x374.
 * step > 0: toward a; while the leg to a is shorter than step, pos jumps to a,
 * the u8 indices at 0x295/0x296 count down (clamped to count - 1) and
 * func_020a6bb4 refreshes a, b and dir.
 * step <= 0: the same toward b with -step, indices counting up (wrapping to 0).
 * When the 0x295 index is at its end and the 0x241 byte is 0, *out and *moving
 * are cleared and pos is set from the leftover length along b - a.
 * Otherwise *out comes from func_020a6a9c.
 * t1 and t3 are reused for several values (t1: dist to a, its leftover,
 * the overshoot, the negated step; t3: -1.0, then dist to b).
 */
void func_020a5d88(void *self, void *list, Vec3 *pos, u8 *moving, Vec3 *a, Vec3 *b, s32 *out, Vec3 *dir) {
    Vec3 d_fwd1;
    Vec3 d_end1;
    Vec3 d_fwd2;
    Vec3 d_end2;
    Vec3 e_end1;
    Vec3 e_end2;
    Fx step;
    Fx t1;
    Fx rem_b;
    Fx t3;
    s32 res;

    step = M2C_FIELD(self, Fx *, 0x374);
    t1.v = vec_dist(pos, a);
    if (step.v > 0) {
        while (step.v > 0) {
            if (t1.v >= step.v) {
                { /* FAKE: block temp for t1.v -= step.v; stops the D1/F1 stack-slot join */
                    s32 rem_a = t1.v;

                    rem_a -= step.v;
                    t1.v = rem_a;
                }
                vec_sub(&d_fwd1, pos, a);
                vec_set_len(&d_fwd1, t1);
                d_fwd1.x += a->x;
                d_fwd1.y += a->y;
                d_fwd1.z += a->z;
                vec_set(pos, &d_fwd1);
                break;
            }
            if (M2C_FIELD(self, u8 *, 0x295) != 0 || M2C_FIELD(self, u8 *, 0x241) != 0) {
                step.v -= t1.v;
                vec_set(pos, a);
                M2C_FIELD(self, u8 *, 0x295)--;
                M2C_FIELD(self, u8 *, 0x296)--;
                if (M2C_FIELD(self, u8 *, 0x295) >= func_02067b54(list)) {
                    M2C_FIELD(self, u8 *, 0x295) = func_02067b54(list) - 1;
                }
                if (M2C_FIELD(self, u8 *, 0x296) >= func_02067b54(list)) {
                    M2C_FIELD(self, u8 *, 0x296) = func_02067b54(list) - 1;
                }
                func_020a6bb4(self, list, a, b, dir, M2C_FIELD(self, u8 *, 0x295), M2C_FIELD(self, u8 *, 0x296));
                t1.v = vec_mag(dir);
            } else {
                *out = 0;
                t1.v = step.v - t1.v;
                vec_sub(&d_end1, b, a);
                vec_set_len(&d_end1, t1);
                vec_sub(&e_end1, a, &d_end1);
                vec_set(pos, &e_end1);
                *moving = 0;
                return;
            }
        }
    } else {
        t3.v = -FX_ONE;
        t1.v = fx_mul_v(step.v, t3);
        t3.v = vec_dist(pos, b);
        while (t1.v > 0) {
            if (t3.v >= t1.v) {
                rem_b.v = t3.v - t1.v;
                vec_sub(&d_fwd2, pos, b);
                vec_set_len(&d_fwd2, rem_b);
                d_fwd2.x += b->x;
                d_fwd2.y += b->y;
                d_fwd2.z += b->z;
                vec_set(pos, &d_fwd2);
                break;
            }
            if (M2C_FIELD(self, u8 *, 0x295) < (u32) (func_02067b54(list) - 1) || M2C_FIELD(self, u8 *, 0x241) != 0) {
                t1.v -= t3.v;
                vec_set(pos, b);
                M2C_FIELD(self, u8 *, 0x295)++;
                M2C_FIELD(self, u8 *, 0x296)++;
                if (M2C_FIELD(self, u8 *, 0x295) >= func_02067b54(list)) {
                    M2C_FIELD(self, u8 *, 0x295) = 0;
                }
                if (M2C_FIELD(self, u8 *, 0x296) >= func_02067b54(list)) {
                    M2C_FIELD(self, u8 *, 0x296) = 0;
                }
                func_020a6bb4(self, list, a, b, dir, M2C_FIELD(self, u8 *, 0x295), M2C_FIELD(self, u8 *, 0x296));
                t3.v = vec_mag(dir);
            } else {
                *out   = 0;
                step.v = t1.v - t3.v;
                vec_sub(&d_end2, b, a);
                vec_set_len(&d_end2, step);
                vec_sub(&e_end2, b, &d_end2);
                vec_set(pos, &e_end2);
                *moving = 0;
                return;
            }
        }
    }
    func_020a6a9c(&res, self, a, pos, dir);
    *out = res;
}
