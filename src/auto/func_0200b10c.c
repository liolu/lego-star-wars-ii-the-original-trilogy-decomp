#include "m2c.h"

/* Variadic sibling of func_0200cad8/func_02001b08: arg1 is read back from the
 * register save area before the tail call into func_02010384.
 *
 * The state word (object +0x148 points at it, or 0) is copied to a stack slot;
 * bits 12..27 of it are compared against the u16 count at +0x1bc minus one.
 * Once the index reaches the last entry, bit 1 of the flags word at +8 is set
 * and bit 0 is cleared.
 */
typedef struct {
    s32 v;
} UnkVal;

void func_02010384(void *, UnkVal);

void func_0200b10c(void *arg0, UnkVal arg1, ...) {
    s32 v;
    s32 *vp;
    s32 *q;

    q  = M2C_FIELD(arg0, s32 **, 0x148);
    vp = &v;
    if (q != 0) {
        v = *q;
    } else {
        v = 0;
    }
    if (((*vp >> 12) & 0xffff) >= M2C_FIELD(arg0, u16 *, 0x1bc) - 1) {
        M2C_FIELD(arg0, u32 *, 8) = (M2C_FIELD(arg0, u32 *, 8) | 2) & ~1;
    }
    func_02010384(arg0, arg1);
}
