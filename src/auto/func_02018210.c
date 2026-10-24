#include "m2c.h"

/* Reconstructed from USA ARM9 at 0x02018210 (104 bytes).
 *
 * A variadic function (the original keeps r0-r3 in an argument home area).
 * The two halfword aggregates are passed by value to func_02098504 together
 * with the address of a local copy of the halfword at +0xca; if the callee
 * changed that local, the new value is stored back and a flag byte is set.
 * The callee's result is returned unchanged, which keeps it in r0 across the
 * update code.
 */
typedef struct {
    u16 v;
} HalfVal;

extern s32 func_02098504(u16 *, HalfVal, HalfVal);

s32 func_02018210(void *self, HalfVal a1, HalfVal a2, ...) {
    u16 x;
    volatile u16 y;
    s32 r;

    x = M2C_FIELD(self, u16 *, 0xca);
    r = func_02098504(&x, a1, a2);
    y = x;
    if (M2C_FIELD(self, u16 *, 0xca) != x) {
        M2C_FIELD(self, u16 *, 0xca) = x;
        M2C_FIELD(self, u8 *, 0xd0)  = 1;
    }
    return r;
}
