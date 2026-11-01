#include "m2c.h"

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct {
    s32 m[16];
} Mtx44;

enum {
    FX_SHIFT = 12, /* Q12 fixed-point fraction bits */
};

/*
 * Transforms the Q12 point `in` by the 4x4 matrix `m` into `out`:
 * out.x = (in.x*m[0] + in.y*m[4] + in.z*m[8]) >> 12, then adds the
 * translation m[12]; likewise y uses m[1], m[5], m[9] + m[13] and
 * z uses m[2], m[6], m[10] + m[14]. Each sum is done in 64 bits.
 * `in` is read fully before any store, so `in` and `out` may be the same
 * vector (func_020a41f4 rotates a step vector in place this way).
 */
void func_01ffac98(Mtx44 *m, Vec3 *in, Vec3 *out) {
    s32 x = in->x;
    s32 y = in->y;
    s32 z = in->z;

    out->x = (s32) (((s64) x * m->m[0] + (s64) y * m->m[4] + (s64) z * m->m[8]) >> FX_SHIFT);
    out->x += m->m[12];
    out->y = (s32) (((s64) x * m->m[1] + (s64) y * m->m[5] + (s64) z * m->m[9]) >> FX_SHIFT);
    out->y += m->m[13];
    out->z = (s32) (((s64) x * m->m[2] + (s64) y * m->m[6] + (s64) z * m->m[10]) >> FX_SHIFT);
    out->z += m->m[14];
}
