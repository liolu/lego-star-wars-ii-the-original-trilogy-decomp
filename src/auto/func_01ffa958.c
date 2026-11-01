#include "m2c.h"

typedef struct Vec3 {
    s32 x, y, z;
} Vec3;

enum {
    FX32_SHIFT = 12, /* 20.12 fixed-point fraction bits */
};

/*
 * Multiply vector v by the 3x3 fixed-point matrix m (9 s32 entries)
 * as a row vector: out[i] = v.x*m[i] + v.y*m[3+i] + v.z*m[6+i].
 * Products accumulate in 64 bits and are scaled back by FX32_SHIFT.
 * out is cleared before the components are written. Callers pass a
 * rotation matrix built by func_01ffa6f8 and an offset vector.
 */
void func_01ffa958(Vec3 *out, const s32 *m, const Vec3 *v) {
    s32 x = v->x;
    s32 y = v->y;
    s32 z = v->z;

    out->x = 0;
    out->y = 0;
    out->z = 0;
    out->x = (s32) (((s64) x * m[0] + (s64) y * m[3] + (s64) z * m[6]) >> FX32_SHIFT);
    out->y = (s32) (((s64) x * m[1] + (s64) y * m[4] + (s64) z * m[7]) >> FX32_SHIFT);
    out->z = (s32) (((s64) x * m[2] + (s64) y * m[5] + (s64) z * m[8]) >> FX32_SHIFT);
}
