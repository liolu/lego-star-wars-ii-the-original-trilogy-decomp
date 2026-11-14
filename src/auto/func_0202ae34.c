#include "m2c.h"

typedef volatile u64 vu64;
typedef volatile u32 vu32;
typedef volatile u16 vu16;

typedef struct {
    s32 x, y, z;
} Vec3;

typedef struct {
    u8 pad[0x14];
    Vec3 pos;
} Actor;

typedef struct {
    u8 id;
    u8 *info;
} Node;

typedef struct {
    Actor ***actors;
    Node nodes[10];
    u8 pad[1];
    u8 count;
    u8 cur;
} Path;

/* 20.12 fixed-point value. */
typedef struct {
    s32 v;
} Whole;

/* NDS hardware divider / square-root unit registers. */
enum {
    REG_DIVCNT      = 0x04000280,
    REG_DIV_NUMER   = 0x04000290,
    REG_DIV_DENOM   = 0x04000298,
    REG_DIV_RESULT  = 0x040002A0,
    REG_SQRTCNT     = 0x040002B0,
    REG_SQRT_RESULT = 0x040002B4,
    REG_SQRT_PARAM  = 0x040002B8
};
enum {
    DIVSQRT_BUSY = 0x8000
};
enum {
    FX_SHIFT     = 12,
    FX_FRAC_MASK = 0xfff
};

extern void func_02007f24(s32 *out, f64 v);
extern void func_0200ad80(Vec3 *dst, const Vec3 *src);
extern s32 func_020de318(s32 v);
extern Vec3 data_020ff9b4;

static inline Node *path_node(Path *p, s32 i) {
    return &p->nodes[(i == -1) ? p->cur : i];
}

/* Constructor-style helpers: a Whole is zeroed (whole_init) before a raw
   value is assigned to it (whole_set). */
static inline Whole *whole_init(Whole *w) {
    // FAKE: dead store (whole_set overwrites it at once); the zero it needs makes 5 of the 6 masks build as
    // mov rZ,#0; sub rM,rZ,#0x1000 (the natural mask form judges 32)
    w->v = 0;
    return w;
}

static inline Whole *whole_set(Whole *w, s32 raw) {
    whole_init(w)->v = raw;
    return w;
}

/* Drops the fraction bits of w in place. */
static inline Whole *whole_trunc(Whole *w) {
    s32 t = w->v;
    return whole_set(w, t & ~FX_FRAC_MASK);
}

/* *out = *p truncated toward zero to a whole number (negatives are rounded
   up by FX_FRAC_MASK first). */
static inline Whole *fx_whole(Whole *out, const s32 *p) {
    if (*p < 0) {
        out->v = *p + FX_FRAC_MASK;
        return whole_trunc(out);
    }
    return whole_set(out, *p & ~FX_FRAC_MASK);
}

/* Square of the integer part. */
static inline s32 whole_sq(const Whole *w) {
    return (w->v >> FX_SHIFT) * (w->v >> FX_SHIFT);
}

static inline s32 vec_int_len(const Vec3 *v) {
    Whole wy, wx, wz;
    s32 sq;
    sq                    = whole_sq(fx_whole(&wx, &v->x)) + whole_sq(fx_whole(&wy, &v->y)) + whole_sq(fx_whole(&wz, &v->z));
    *(vu16 *) REG_SQRTCNT = 0;
    *(vu32 *) REG_SQRT_PARAM = sq;
    while (*(vu16 *) REG_SQRTCNT & DIVSQRT_BUSY) {
    }
    return *(vu16 *) REG_SQRT_RESULT;
}

static inline Vec3 *vec_sub(const Vec3 *a, const Vec3 *b, Vec3 *out) {
    s32 z  = a->z - b->z;
    s32 y  = a->y - b->y;
    s32 x  = a->x - b->x;
    out->x = x;
    out->y = y;
    out->z = z;
    return out;
}

static inline Vec3 *vec_set(Vec3 *dst, const Vec3 *src) {
    if (dst != src) {
        dst->x = src->x;
        dst->y = src->y;
        dst->z = src->z;
    }
    return dst;
}

/* dst = src / |src| (20.12), from the hardware divider (2^56 / |src|^2) and
   square root (2 |src|). */
static inline void vec_normalize(const Vec3 *src, Vec3 *dst) {
    s64 t;
    s32 sq;
    s64 div;

    t = (s64) src->x * src->x;
    t += (s64) src->y * src->y;
    t += (s64) src->z * src->z;

    *(vu16 *) REG_DIVCNT    = 2;
    *(vu64 *) REG_DIV_NUMER = 1LL << 56;
    *(vu64 *) REG_DIV_DENOM = t;
    *(vu16 *) REG_SQRTCNT   = 1;
    *(u64 *) REG_SQRT_PARAM = t << 2;
    while (*(vu16 *) REG_SQRTCNT & DIVSQRT_BUSY) {
    }
    sq = (s32) * (vu32 *) REG_SQRT_RESULT;
    while (*(vu16 *) REG_DIVCNT & DIVSQRT_BUSY) {
    }
    div = (s64) * (vu64 *) REG_DIV_RESULT;
    div *= sq;
    dst->x = (s32) ((div * src->x + (1LL << 44)) >> 45);
    dst->y = (s32) ((div * src->y + (1LL << 44)) >> 45);
    dst->z = (s32) ((div * src->z + (1LL << 44)) >> 45);
}

/* Normalizes v in place; a zero vector becomes data_020ff9b4 instead. */
static inline void vec_safe_normalize(Vec3 *v) {
    if (v->x == 0 && v->y == 0 && v->z == 0) {
        vec_set(v, &data_020ff9b4);
        return;
    }
    vec_normalize(v, v);
}

static inline s32 vec_dot(const Vec3 *a, const Vec3 *b) {
    return (s32) (((s64) a->x * b->x + (s64) a->y * b->y + (s64) a->z * b->z + 0x800) >> 12);
}

/* Returns 1 when the next path node's info byte is 1, func_020de318 of
   (target distance - segment length) exceeds a quarter of the segment length
   (both whole-unit lengths from the current node's actor), and the difference
   of the two dot products is below the func_02007f24 value of 0.866 (cos 30 deg). */
s32 func_0202ae34(Path *path, Vec3 *target) {
    Vec3 dir;
    Vec3 d1;
    Vec3 d2;
    Vec3 n1;
    Vec3 tmp;
    Vec3 n2;
    s32 limit;
    Actor *a;
    Actor *b;
    Actor **actors;
    s32 cur;
    s32 next;
    s32 len1;
    s32 len2;
    s32 dot1;
    s32 diff;

    cur = path->cur;
    if (cur < path->count - 1) {
        next   = cur + 1;
        actors = *path->actors;
        a      = actors[path->nodes[cur].id];
        b      = actors[path_node(path, next)->id];
        if (path_node(path, next)->info[0] != 1) {
            return 0;
        }
        len1 = vec_int_len(vec_sub(&b->pos, &a->pos, &d1));
        len2 = vec_int_len(vec_sub(target, &a->pos, &d2));
        if (func_020de318(len2 - len1) > len1 / 4) {
            vec_sub(&a->pos, target, &dir);
            vec_safe_normalize(&dir);
            func_0200ad80(&n1, &a->pos);
            dot1 = vec_dot(&dir, &n1);
            vec_sub(&a->pos, &b->pos, &tmp);
            vec_set(&dir, &tmp);
            vec_safe_normalize(&dir);
            func_0200ad80(&n2, &a->pos);
            diff = dot1 - vec_dot(&dir, &n2);
            func_02007f24(&limit, 0.866f);
            if (diff < limit) {
                return 1;
            }
        }
    }
    return 0;
}
