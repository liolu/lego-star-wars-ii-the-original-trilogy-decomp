#include "m2c.h"

s32 func_01ffa408(void *stream);
s32 func_01ffa41c(void *stream);

/* Reads four values from the stream (the callers pass the same stream used for their own
   func_01ffa41c reads) into bytes 0xc..0xf of the object: two func_01ffa408 results go to
   0xc and 0xd, the integer part of a 20.12 fixed-point func_01ffa41c result goes to 0xf, and
   the low byte of a second func_01ffa41c result goes to 0xe. Always returns 1. */
s32 func_01ffa430(u8 *obj, void *stream) {
    enum {
        FX_FRAC_BITS = 12,
    };

    obj[0xc] = func_01ffa408(stream);
    obj[0xd] = func_01ffa408(stream);
    obj[0xf] = func_01ffa41c(stream) >> FX_FRAC_BITS;
    obj[0xe] = func_01ffa41c(stream);
    return 1;
}
