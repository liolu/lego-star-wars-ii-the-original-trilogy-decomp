#include "m2c.h"

typedef volatile u64 vu64;
typedef volatile u32 vu32;
typedef volatile u16 vu16;

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

/* NDS ARM9 hardware divider and square-root unit registers. */
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
    CNT_BUSY       = 0x8000
};

extern Vec3 data_020ff9b4;
extern Vec3 data_0210e6f4[];

static inline s32 vec_ne(const Vec3 *a, const Vec3 *b) {
    if (a->x != b->x || a->y != b->y || a->z != b->z) {
        return 1;
    }
    return 0;
}

static inline s32 vec_nz(const Vec3 *a) {
    if (a->x != 0 || a->y != 0 || a->z != 0) {
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

/*
 * Unit vector of *s in fx32 (0x1000 = 1.0) into *d, via the hardware units:
 * 2^56 / |s|^2 times sqrt(4 |s|^2) = 2^57 / |s|, then each component times
 * that >> 45 (rounded). A zero vector yields data_020ff9b4 instead.
 */
static inline void vec_normalize(Vec3 *d, const Vec3 *s) {
    s64 t;
    s32 sqrt;
    s64 div;
    if (s->x == 0 && s->y == 0 && s->z == 0) {
        vec_set(d, &data_020ff9b4);
    } else {
        t = (s64) s->x * s->x;
        t += (s64) s->y * s->y;
        t += (s64) s->z * s->z;

        (*(vu16 *) REG_DIVCNT)    = DIV_MODE_64_64;
        (*(vu64 *) REG_DIV_NUMER) = 1LL << 56;
        (*(vu64 *) REG_DIV_DENOM) = t;

        (*(vu16 *) REG_SQRTCNT)   = SQRT_MODE_64;
        (*(u64 *) REG_SQRT_PARAM) = t << 2;

        while ((*(vu16 *) REG_SQRTCNT) & CNT_BUSY) {
        }
        sqrt = (s32) (*(vu32 *) REG_SQRT_RESULT);

        while ((*(vu16 *) REG_DIVCNT) & CNT_BUSY) {
        }
        div = (s64) (*(u64 *) REG_DIV_RESULT);

        div *= sqrt;

        d->x = (s32) ((div * s->x + (1LL << 44)) >> 45);
        d->y = (s32) ((div * s->y + (1LL << 44)) >> 45);
        d->z = (s32) ((div * s->z + (1LL << 44)) >> 45);
    }
}

/*
 * Reads the vector at arg1+0x54; unless it equals data_020ff9b4, normalises it
 * and scales it by (arg0+0x14 >> 2) as an fx32 multiply (rounded). The result
 * is stored into data_0210e6f4[arg1+0x4c].
 */
void func_020bea10(void *arg0, void *arg1) {
    Vec3 v;
    s32 q[4];
    s32 scale;
    s32 k;
    Vec3 *dst;

    v.x = M2C_FIELD(arg1, Vec3 *, 0x54).x;
    v.y = M2C_FIELD(arg1, Vec3 *, 0x54).y;
    v.z = M2C_FIELD(arg1, Vec3 *, 0x54).z;

    if (vec_ne(&v, &data_020ff9b4)) {
        vec_normalize(&v, &v);
        scale                   = M2C_FIELD(arg0, s32 *, 0x14) >> 2;
        M2C_FIELD(q, s32 *, 12) = scale;
        M2C_FIELD(q, s32 *, 0)  = scale;
        v.x                     = (s32) (((s64) v.x * scale + 0x800) >> 12);
        M2C_FIELD(q, s32 *, 4)  = scale;
        v.y                     = (s32) (((s64) v.y * scale + 0x800) >> 12);
        M2C_FIELD(q, s32 *, 8)  = scale;
        v.z                     = (s32) (((s64) v.z * scale + 0x800) >> 12);
    } else {
        vec_set(&v, &data_020ff9b4);
    }

    k   = M2C_FIELD(arg1, s32 *, 0x4c);
    dst = &data_0210e6f4[k];
    vec_set(dst, &v);
}
