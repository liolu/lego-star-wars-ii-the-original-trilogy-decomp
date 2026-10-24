#include "m2c.h"

typedef struct {
    s16 x, y, z;
} Pt16;
typedef struct {
    s32 x, y, z;
} Vec3;

extern Pt16 *func_02067b48(void *, s32);
extern s32 func_02067b54(void *);
extern void func_020a5c68(s32 *, void *, void *);
extern void func_020a6a9c(s32 *, void *, Vec3 *, s32, Vec3 *);
extern void func_020a5af8(void *, s32, s32, s32, s32 *, s32 *);

static inline void vec_set(Vec3 *v, s32 x, s32 y, s32 z) {
    v->x = x;
    v->y = y;
    v->z = z;
}

static inline void vec_sub(Vec3 *a, Vec3 *b, Vec3 *ans) {
    ans->x = a->x - b->x;
    ans->y = a->y - b->y;
    ans->z = a->z - b->z;
}

void func_020a59dc(void *self, s32 a1, s32 a2, s32 a3, s32 a4) {
    Pt16 *first;
    Pt16 *last;
    Vec3 pos;
    Vec3 diff;
    Vec3 p1;
    s32 out_b;
    s32 out_a;
    s64 sum;

    first = func_02067b48(M2C_FIELD(self, void **, 0x21c), 0);
    last  = func_02067b48(M2C_FIELD(self, void **, 0x21c), func_02067b54(M2C_FIELD(self, void **, 0x21c)) - 1);

    vec_set(&pos, first->x << 12, first->y << 12, first->z << 12);
    vec_set(&p1, last->x << 12, last->y << 12, last->z << 12);
    vec_sub(&p1, &pos, &diff);
    sum = (s64) diff.x * diff.x;
    sum += (s64) diff.y * diff.y;
    sum += (s64) diff.z * diff.z;
    sum <<= 2;
    *(volatile u16 *) 0x040002b0 = 1;
    *(u64 *) 0x040002b8          = sum;
    while (*(volatile u16 *) 0x040002b0 & 0x8000) {
    }

    func_020a6a9c(&out_b, self, &pos, a4, &diff);
    func_020a5c68(&out_a, self, M2C_FIELD(self, void **, 0x21c));
    func_020a5af8(self, a1, a2, a3, &out_b, &out_a);
}
