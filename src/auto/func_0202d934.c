#include "m2c.h"

/* Allocates a 0x40-byte object and constructs it with func_02032b44, then
   registers it in the list at arg0 + 0x5E0.  The binary also stores 0x2000
   into the outgoing stack slot at sp+0x14, which the callee never reads, so
   that slot is modelled as a volatile local (same technique as the matched
   siblings func_020581fc and func_02058280). */

M2C_UNK func_0202c1a8(s32, s32);
s32 func_02032b44(s32, s32, s32, s32, s32, s32, s32, s32, s32);
s32 func_0205e80c(s32, s32, s32);

s32 func_0202d934(s32 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4) {
    s32 temp_r0;
    s32 var_r4;
    volatile s32 sp14;

    temp_r0 = func_0205e80c(0x40, 1, 4);
    var_r4  = temp_r0;
    if (var_r4 != 0) {
        sp14   = 0x2000;
        var_r4 = func_02032b44(temp_r0, arg0, arg1, 0x2000, 0, 0, arg2, arg3, arg4);
    }
    func_0202c1a8(arg0 + 0x5E0, var_r4);
    return var_r4;
}
