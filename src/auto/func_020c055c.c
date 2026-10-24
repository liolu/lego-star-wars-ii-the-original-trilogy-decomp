#include "m2c.h"

typedef struct Vec3 {
    s32 x, y, z;
} Vec3;

typedef struct Obj {
    u8 pad0[0x30];
    Vec3 pos;
    u8 pad3c[0x45 - 0x3c];
    u8 dirty;
    u8 pad46[0x4c - 0x46];
    s32 index;
} Obj;

extern s32 data_0210e2f8[64];
extern s32 data_0210e3f8[64];

static inline int Vec3_equal(const Vec3 *a, const Vec3 *b) {
    return a->x == b->x && a->y == b->y && a->z == b->z;
}
static inline int Vec3_equal_v(const Vec3 *a, s32 x, s32 y, s32 z) {
    return a->x == x && a->y == y && a->z == z;
}

static inline void Vec3_assign(Vec3 *dst, const Vec3 *src) {
    if (dst != src) {
        dst->x = src->x;
        dst->y = src->y;
        dst->z = src->z;
    }
}
static inline void Vec3_assign_v(Vec3 *dst, const Vec3 *src, s32 x) {
    if (dst != src) {
        dst->x = x;
        dst->y = src->y;
        dst->z = src->z;
    }
}

int func_020c055c(void *unused, Obj *obj) {
    Vec3 v;
    int changed;
    s32 z   = obj->pos.z;
    s32 y   = obj->pos.y + data_0210e3f8[obj->index];
    s32 x   = obj->pos.x + data_0210e2f8[obj->index];
    v.x     = x;
    v.y     = y;
    v.z     = z;
    changed = !Vec3_equal_v(&obj->pos, x, y, z);
    if (!changed) {
        return changed;
    }
    Vec3_assign_v(&obj->pos, &v, x);
    obj->dirty = 1;
    return 1;
}
