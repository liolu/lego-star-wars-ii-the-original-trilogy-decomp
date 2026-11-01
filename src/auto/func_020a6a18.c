#include "m2c.h"
typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec;

enum {
    FX_SHIFT = 12,     /* 20.12 fixed point */
    FX_HALF  = 0x800,   /* rounding bias: 0.5 in 20.12 */
};

/*
 * out = base + delta * (*scale), per component, with a rounded 20.12
 * fixed-point multiply. arg0 is unused. The only caller (func_020a542c)
 * passes a segment start, a second segment vector and a parameter t, so
 * this evaluates a point along the segment at t.
 */
void func_020a6a18(void *arg0, Vec *out, Vec *base, Vec *delta, s32 *scale) {
    s32 t;
    t      = (s32) (((s64) *scale * delta->x + FX_HALF) >> FX_SHIFT);
    out->x = base->x + t;
    t      = (s32) (((s64) *scale * delta->y + FX_HALF) >> FX_SHIFT);
    out->y = base->y + t;
    t      = *scale;
    t      = (s32) (((s64) t * delta->z + FX_HALF) >> FX_SHIFT);
    out->z = base->z + t;
}
