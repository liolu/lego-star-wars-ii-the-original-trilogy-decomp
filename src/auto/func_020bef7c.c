#include "m2c.h"

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct {
    u8 pad0[0x14];
    Vec3 vel;
    s32 limit;
} Src;

typedef struct {
    u8 pad0[0x24];
    Vec3 pos;
    u8 pad30[0x45 - 0x30];
    u8 dirty;
    u8 pad46[0x4c - 0x46];
    s32 idx;
} Dst;

typedef struct {
    s32 v;
} Fixed;

extern Vec3 data_0210e9f4[64];

void func_020111c4(s32 *, Vec3 *);

enum {
    FX_SHIFT = 12,
    FX_HALF  = 0x800
};

/* 20.12 fixed-point multiply, rounded to nearest. */
static inline s32 fx_mul(s32 a, s32 b) {
    return (s32) (((s64) a * b + FX_HALF) >> FX_SHIFT);
}

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

static inline int vec_ne(const Vec3 *a, const Vec3 *b) {
    if (a->x != b->x || a->y != b->y || a->z != b->z) {
        return 1;
    }
    return 0;
}

/*
 * Same flow as func_020bee2c, with a0->vel scaled by the fixed-point factor a2.v:
 * cur = data_0210e9f4[a1->idx] + a0->vel * a2.v. If a0->limit is non-zero,
 * func_020111c4 first reads the entry's OLD value into len; when len > a0->limit
 * the function returns with nothing changed. Otherwise a1->pos moves by
 * (cur >> 4) * a2.v per component (a1->dirty = 1 only if it actually changes)
 * and cur is written back to the table entry.
 * The temporaries are int, not s32 (long): with long temps every sum is emitted
 * with the product as its first operand.
 */
void func_020bef7c(Src *a0, Dst *a1, Fixed a2) {
    Vec3 cur;
    Vec3 np;
    s32 len;
    s32 *p;
    int z, y, x;
    Vec3 *e;

    z     = fx_mul(a0->vel.z, a2.v);
    y     = fx_mul(a0->vel.y, a2.v);
    x     = fx_mul(a0->vel.x, a2.v);
    e     = &data_0210e9f4[a1->idx];
    z     = e->z + z;
    y     = e->y + y;
    x     = e->x + x;
    cur.x = x;
    cur.y = y;
    cur.z = z;
    if (a0->limit != 0) {
        func_020111c4(&len, &data_0210e9f4[a1->idx]);
        p = &len; // FAKE: pointer copy only so the null test below has something to test
        if (p && *p > a0->limit) { // FAKE: always-true null test keeps &len in a register (add r1,sp,#0; ldr r1,[r1])
            return;
        }
    }
    z    = cur.z >> 4;
    y    = cur.y >> 4;
    x    = cur.x >> 4;
    z    = fx_mul(z, a2.v);
    y    = fx_mul(y, a2.v);
    x    = fx_mul(x, a2.v);
    z    = a1->pos.z + z;
    y    = a1->pos.y + y;
    x    = a1->pos.x + x;
    np.x = x;
    np.y = y;
    np.z = z;
    if (vec_ne(&a1->pos, &np)) {
        vec_set(&a1->pos, &np);
        a1->dirty = 1;
    }
    vec_set(&data_0210e9f4[a1->idx], &cur);
}
