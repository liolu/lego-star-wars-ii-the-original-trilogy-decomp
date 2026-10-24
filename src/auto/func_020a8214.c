/* Reconstructed from USA ARM9 at 0x020a8214 (104 bytes).
 *
 * Orientation test for three points in the X/Z plane: the sign of the 20.12
 * fixed-point cross product (c - a) x (c - b).  Returns 1 when it is positive.
 * The first argument is unused; the callee-saved scratch it provides and the
 * in-place differences keep the loads and subtractions in the original order.
 */
#include "m2c.h"

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec;

s32 func_020a8214(s32 arg0, Vec *a, Vec *b, Vec *c) {
    s32 cz = c->z;
    s32 az = a->z;
    s32 bz = b->z;
    s32 cx = c->x;
    s32 ax = a->x;
    s32 bx = b->x;

    arg0 = cz - az;
    bx   = cx - bx;
    cz   = cz - bz;
    cx   = cx - ax;
    return (s32) (((s64) cx * cz + 0x800) >> 12) - (s32) (((s64) bx * arg0 + 0x800) >> 12) > 0;
}
