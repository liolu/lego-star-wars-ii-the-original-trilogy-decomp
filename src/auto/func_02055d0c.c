#include "m2c.h"

// ASM: NitroSDK hand-asm idiom (MIi_CpuCopy16 shape): a fully predicated loop body closed by blt, with r4 saved
// around a bx lr return (lr unused). Copies r3 bytes of halfwords from r0 to both r1 and r2.
// clang-format off
asm void func_02055d0c(void) {
    stmdb sp!, {r4}
    mov ip, #0
L_02055d14:
    cmp ip, r3
    ldrlth r4, [r0, ip]
    strlth r4, [r1, ip]
    strlth r4, [r2, ip]
    addlt ip, ip, #2
    blt L_02055d14
    ldmia sp!, {r4}
    bx lr
}
// clang-format on
