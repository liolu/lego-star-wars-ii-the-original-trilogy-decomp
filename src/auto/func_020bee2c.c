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

extern Vec3 data_0210e9f4[64];

void func_020111c4(s32 *, Vec3 *);

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

static inline void vec_add(Vec3 *d, const Vec3 *a, const Vec3 *b) {
    s32 z = a->z + b->z;
    s32 y = a->y + b->y;
    s32 x = a->x + b->x;
    d->x  = x;
    d->y  = y;
    d->z  = z;
}

static inline void vec_add_shr4(Vec3 *d, const Vec3 *a, const Vec3 *b) {
    s32 z = a->z >> 4;
    s32 y = a->y >> 4;
    s32 x = a->x >> 4;
    z += b->z;
    y += b->y;
    x += b->x;
    d->x = x;
    d->y = y;
    d->z = z;
}

static inline int vec_ne(const Vec3 *a, const Vec3 *b) {
    if (a->x != b->x || a->y != b->y || a->z != b->z) {
        return 1;
    }
    return 0;
}

/*
 * Adds a0->vel to table entry data_0210e9f4[a1->idx] (the result is cur).
 * If a0->limit is non-zero, func_020111c4 first reads the entry's OLD value
 * into len; when len > a0->limit the function returns with nothing changed.
 * Otherwise a1->pos moves by cur >> 4 per component (a1->dirty = 1 only if
 * it actually changes) and cur is written back to the table entry.
 */
void func_020bee2c(Src *a0, Dst *a1) {
    Vec3 cur;
    Vec3 np;
    s32 len;
    s32 *p;

    vec_add(&cur, &data_0210e9f4[a1->idx], &a0->vel);
    if (a0->limit != 0) {
        func_020111c4(&len, &data_0210e9f4[a1->idx]);
        p = &len;
        if (p && *p > a0->limit) { // FAKE: always-true null test keeps &len in a register (add r1,sp,#0; ldr r1,[r1])
            return;
        }
    }
    vec_add_shr4(&np, &cur, &a1->pos);
    if (vec_ne(&a1->pos, &np)) {
        vec_set(&a1->pos, &np);
        a1->dirty = 1;
    }
    vec_set(&data_0210e9f4[a1->idx], &cur);
}
