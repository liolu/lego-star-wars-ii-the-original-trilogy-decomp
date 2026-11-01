#include "m2c.h"

typedef struct Vec3 {
    s32 x, y, z;
} Vec3;

typedef struct Obj {
    s32 unk0;
    Vec3 pos;
    u8 pad10[0x20];
    s32 scale;
} Obj;

enum {
    FX32_SHIFT = 12,
    FX32_HALF  = 0x800
};

/* 20.12 fixed-point multiply, rounded to nearest. */
static inline s32 FxMul(s32 a, s32 b) {
    return (s32) (((s64) a * b + FX32_HALF) >> FX32_SHIFT);
}

/* out = obj->pos + in * obj->scale (fx32 components; z is computed first). */
void func_0204f4c8(Vec3 *out, Obj *obj, Vec3 *in) {
    int z  = FxMul(in->z, obj->scale);
    int y  = FxMul(in->y, obj->scale);
    int x  = FxMul(in->x, obj->scale);
    z      = obj->pos.z + z;
    y      = obj->pos.y + y;
    x      = obj->pos.x + x;
    out->x = x;
    out->y = y;
    out->z = z;
}
