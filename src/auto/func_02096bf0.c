#include "m2c.h"

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec;

typedef struct {
    s32 value;
} Fixed;

// Polymorphic region object: offset 0 holds the vtable pointer (data_020ff88c,
// or data_020ff8a4 for the derived class built by func_020974b4).
typedef struct {
    u8 pad0[4];
    Vec c;      // centre point (Q12)
    s32 r10;    // X extent (Q12); radius for KIND_CYLINDER
    s32 r14;    // Y extent (Q12); radius for KIND_BALL, height for KIND_CYLINDER
    u8 pad18[7];
    u8 f1f;     // nonzero: keep the result at the centre height (no Y jitter)
    u8 pad20[12];
    u8 kind;    // 0 (set by the constructors): box, handled by func_02096374
} A;

extern Vec data_020ff9b4;
extern void func_02007f24(Fixed *out, double d);
extern s32 func_02062a7c(s32 lo, s32 hi);
extern void func_02096374(A *a, Vec *p);

static inline Vec *initVec(Vec *v, s32 x, s32 y, s32 z) {
    v->x = x;
    v->y = y;
    v->z = z;
    return v;
}

static inline void VecCopy(const Vec *src, Vec *dst) {
    if (dst != src) {
        dst->x = src->x;
        dst->y = src->y;
        dst->z = src->z;
    }
}

static inline s32 VecNonZeroXY(const Vec *v) {
    s32 r = 1;
    s32 x = v->x;
    if (x == 0 && v->y == 0) {
        r = 0;
    }
    return r;
}

static inline s32 VecNonZero(const Vec *v) {
    s32 r = 1;
    if (!VecNonZeroXY(v) && v->z == 0) {
        r = 0;
    }
    return r;
}

static inline s64 VecDot64(const Vec *a, const Vec *b) {
    s64 t = (s64) a->x * b->x;
    t += (s64) a->y * b->y;
    t += (s64) a->z * b->z;
    return t;
}

static inline void VecNormalize(const Vec *src, Vec *dst) {
    s64 t;
    s32 sqrt;
    s64 div;

    if (src->x == 0 && src->y == 0 && src->z == 0) {
        VecCopy(&data_020ff9b4, dst);
        return;
    }
    t = VecDot64(src, src);

    *(volatile u16 *) 0x04000280 = 2;
    *(volatile u64 *) 0x04000290 = 1LL << 56;
    *(volatile u64 *) 0x04000298 = t;

    *(volatile u16 *) 0x040002B0 = 1;
    *(u64 *) 0x040002B8          = t << 2;

    while (*(volatile u16 *) 0x040002B0 & 0x8000) {
    }
    sqrt = (s32) * (volatile u32 *) 0x040002B4;

    while (*(volatile u16 *) 0x04000280 & 0x8000) {
    }
    div = (s64) * (u64 *) 0x040002A0;

    div *= sqrt;

    dst->x = (s32) ((div * src->x + (1LL << 44)) >> 45);
    dst->y = (s32) ((div * src->y + (1LL << 44)) >> 45);
    dst->z = (s32) ((div * src->z + (1LL << 44)) >> 45);
}

static inline s32 multiplyFixed(s32 value, const Fixed *scale) {
    return (s32) (((s64) value * scale->value + 0x800) >> 12);
}

static inline s32 fxMulV(const Fixed *a, const Fixed *b) {
    return (s32) (((s64) a->value * b->value + 0x800) >> 12);
}

static inline Fixed *fxSet(Fixed *f, double d) {
    func_02007f24(f, d);
    return f;
}

static inline void scaleVector(Vec *v, const Fixed *s) {
    Fixed x = *s;
    Fixed y;
    Fixed z;
    v->x = multiplyFixed(v->x, &x);
    y    = *s;
    v->y = multiplyFixed(v->y, &y);
    z    = *s;
    v->z = multiplyFixed(v->z, &z);
}

enum {
    KIND_BALL     = 1,
    KIND_CYLINDER = 2
};
enum {
    DIR_COMPONENT_RANGE = 1000
};

/* Region virtual method at vtable slot 2 (+0x08) of data_020ff88c and data_020ff8a4: pick a random point
 * for region a and write it to p. The point starts at the centre a->c. Kind 1 draws a random 3D direction
 * (integer components in [-1000, 1000], retried while all are zero), normalises it and scales it by a
 * random length in [0.01, int(r14)] (func_02062a7c returns an integer in [lo, hi]; func_02007f24 converts
 * a double to Q12). Kind 2 does the same in the XZ plane with radius int(r10), then adds a random Y offset
 * in [-int(r14) / 2, int(r14) / 2] unless f1f is set. Other kinds jitter inside a box (func_02096374).
 */
void func_02096bf0(A *a, Vec *p) {
    Vec dir1;
    Vec dir2;
    Vec tmp1;
    Vec tmp2;
    Fixed rx1, ry1, rz1;
    Fixed scale1;
    Fixed len1;
    Fixed unit1;
    Fixed rx2, ry2, rz2;
    Fixed scale2;
    Fixed len2;
    Fixed unit2;
    Fixed dy;
    s32 r;
    const Fixed *pl1;
    const Fixed *pl2;

    if (a->kind == KIND_BALL) {
        VecCopy(&a->c, p);
        initVec(&dir1, 0, 0, 0);
        do {
            func_02007f24(&rz1, (double) func_02062a7c(-DIR_COMPONENT_RANGE, DIR_COMPONENT_RANGE));
            func_02007f24(&ry1, (double) func_02062a7c(-DIR_COMPONENT_RANGE, DIR_COMPONENT_RANGE));
            func_02007f24(&rx1, (double) func_02062a7c(-DIR_COMPONENT_RANGE, DIR_COMPONENT_RANGE));
            VecCopy(initVec(&tmp1, rx1.value, ry1.value, rz1.value), &dir1);
            if (VecNonZero(&dir1)) {
                break;
            }
        } while (1);
        VecNormalize(&dir1, &dir1);
        func_02007f24(&unit1, 0.01); // the random length is drawn in hundredths
        pl1 = fxSet(&len1, (double) func_02062a7c(1, (a->r14 >> 12) * 100));
        if (pl1) { // FAKE: always-true test keeps &len1 out of block-local copy propagation
            scale1.value = fxMulV(pl1, &unit1);
        }
        // BUG?: the scaled offset dir1 is never added to p, so kind 1 returns the centre itself
        // (the f1f height reset below is then a no-op); the target keeps these dead stores too.
        scaleVector(&dir1, &scale1);
        if (a->f1f != 0) {
            p->y = a->c.y;
        }
    } else if (a->kind == KIND_CYLINDER) {
        VecCopy(&a->c, p);
        initVec(&dir2, 0, 0, 0);
        do {
            func_02007f24(&rz2, (double) func_02062a7c(-DIR_COMPONENT_RANGE, DIR_COMPONENT_RANGE));
            func_02007f24(&ry2, 0.0);
            func_02007f24(&rx2, (double) func_02062a7c(-DIR_COMPONENT_RANGE, DIR_COMPONENT_RANGE));
            VecCopy(initVec(&tmp2, rx2.value, ry2.value, rz2.value), &dir2);
            if (VecNonZero(&dir2)) {
                break;
            }
        } while (1);
        VecNormalize(&dir2, &dir2);
        func_02007f24(&unit2, 0.01);
        pl2 = fxSet(&len2, (double) func_02062a7c(1, (a->r10 >> 12) * 100));
        if (pl2) { // FAKE: always-true test keeps &len2 out of block-local copy propagation
            scale2.value = fxMulV(pl2, &unit2);
        }
        // BUG?: as in kind 1, dir2 is never added to p; only the Y jitter below moves the point.
        scaleVector(&dir2, &scale2);
        if (a->f1f != 0) {
            p->y = a->c.y;
            return;
        }
        r = a->r14 >> 12;
        func_02007f24(&dy, 0.01 * (double) func_02062a7c(-r * 50, r * 50));
        p->y += dy.value;
    } else {
        func_02096374(a, p);
    }
}
