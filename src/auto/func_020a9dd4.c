#include "m2c.h"

typedef struct {
    s32 v;
} Cost;

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

s32 FX_Div(s32, s32);

/* DS math coprocessor square-root unit; FX32_ONE = 1.0 in 20.12 fixed point (fx_mul shifts by 12). */
enum {
    REG_SQRTCNT     = 0x040002B0,
    REG_SQRT_RESULT = 0x040002B4,
    REG_SQRT_PARAM  = 0x040002B8,
    SQRTCNT_64BIT   = 1,
    SQRTCNT_BUSY    = 0x8000
};
enum {
    FX32_ONE = 0x1000
};

static inline s32 fx_mul(s32 a, s32 b) {
    return (s32) (((s64) a * b + 0x800) >> 12);
}

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

static inline s32 vec_dot(const Vec3 *a, const Vec3 *b) {
    s32 r = fx_mul(a->x, b->x);
    r += fx_mul(a->y, b->y);
    r += fx_mul(a->z, b->z);
    return r;
}

static inline s32 vec_mag(const Vec3 *v) {
    s64 t;
    t = (s64) v->x * v->x;
    t += (s64) v->y * v->y;
    t += (s64) v->z * v->z;
    *(volatile u16 *) REG_SQRTCNT = SQRTCNT_64BIT;
    *(u64 *) REG_SQRT_PARAM       = t << 2;
    while (*(volatile u16 *) REG_SQRTCNT & SQRTCNT_BUSY) {
    }
    return ((s32) * (volatile u32 *) REG_SQRT_RESULT + 1) >> 1;
}

/* Reset a cost record before filling it in. */
// FAKE: inline helper that stores 0; only used as a dead store before the real value (see the call).
static inline void cost_init(Cost *c) {
    c->v = 0;
}

/* Distance from point p to segment a-b; 0x0FFFFFFF when p projects outside the segment. */
void func_020a9dd4(Cost *out, s32 unused, Vec3 *p, Vec3 *a, Vec3 *b) {
    Vec3 d;
    Vec3 diff;
    s32 len;
    s32 t;
    Vec3 proj;
    Vec3 av, ba, pa;

    vec_sub(&d, a, b);
    len = vec_mag(&d);

    av.z = a->z;
    ba.z = b->z;
    ba.z -= av.z;
    pa.z = p->z;
    pa.z -= av.z;
    av.y = a->y;
    ba.y = b->y;
    ba.y -= av.y;
    pa.y = p->y;
    pa.y -= av.y;
    av.x = a->x;
    ba.x = b->x;
    ba.x -= av.x;
    pa.x = p->x;
    pa.x -= av.x;
    t = FX_Div(vec_dot(&pa, &ba), fx_mul(len, len));
    if (t < 0 || t > FX32_ONE) {
        cost_init(out); // FAKE: dead store of 0, overwritten next line; forces mov #0 + sub #0xF0000001 for 0x0FFFFFFF
        out->v = 0x0FFFFFFF;
        return;
    }

    proj.x = a->x;
    proj.x += fx_mul(t, b->x - a->x);
    proj.y = a->y;
    proj.y += fx_mul(t, b->y - a->y);
    proj.z = a->z;
    proj.z += fx_mul(t, b->z - a->z);
    vec_sub(&diff, p, &proj);
    vec_set(&d, &diff);

    out->v = vec_mag(&d);
}
