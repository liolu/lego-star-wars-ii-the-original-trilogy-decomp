/* Reconstructed from USA ARM9 at 0x020c0798 (244 bytes).
 *
 * Entry of the pointer table at 0x0210127c, next to func_020c0708 (which
 * scales the vector at +0x60 of the second object by self->scale).
 *
 * Here the vector is pulled back by a fraction of itself, in 20.12 fixed
 * point with rounding on each multiply:
 *   1. d = v * (1.0 - self->scale)
 *   2. d = d * a.v[0]   (first word of the by-value third argument; its
 *                        r2/r3 home spill is the "push {r0-r3}" prologue)
 *   3. v = v - d, built in a stack copy and written back unless it aliases.
 */
#include "m2c.h"

enum {
    FX_ONE   = 0x1000,
    FX_HALF  = 0x800,
    FX_SHIFT = 12,
};

typedef struct {
    s32 x, y, z;
} V3;

typedef struct {
    s32 v[2];
} Arg;

typedef struct {
    u8 pad[0x60];
    V3 v;
} Target;

typedef struct {
    u8 pad[0x14];
    s32 scale;
} Ctl;

void func_020c0798(Ctl *self, Target *t, Arg a) {
    s32 x, y, z;
    s32 dz, dy, dx;
    s32 one_minus_scale;
    V3 v;
    one_minus_scale = FX_ONE - self->scale;
    x               = t->v.x;
    y               = t->v.y;
    z               = t->v.z;
    dz              = (s32) (((s64) z * (s64) one_minus_scale + FX_HALF) >> FX_SHIFT);
    dy              = (s32) (((s64) y * (s64) one_minus_scale + FX_HALF) >> FX_SHIFT);
    dx              = (s32) (((s64) x * (s64) one_minus_scale + FX_HALF) >> FX_SHIFT);
    dz              = (s32) (((s64) dz * (s64) a.v[0] + FX_HALF) >> FX_SHIFT);
    dy              = (s32) (((s64) dy * (s64) a.v[0] + FX_HALF) >> FX_SHIFT);
    dx              = (s32) (((s64) dx * (s64) a.v[0] + FX_HALF) >> FX_SHIFT);
    v.x             = x - dx;
    v.y             = y - dy;
    v.z             = z - dz;
    if (&t->v != &v) {
        t->v.x = v.x;
        t->v.y = v.y;
        t->v.z = v.z;
    }
}
