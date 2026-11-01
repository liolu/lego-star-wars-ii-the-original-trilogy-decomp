#include "m2c.h"

// ASM: predicated single-register stmltia with writeback in a `b L; L:` loop using ip as the end
// pointer; mwcc emits none of these (hand-written SDK MIi_CpuFill32 shape). Fills r2 bytes at r1 with word r0.
// clang-format off
asm void MIi_CpuClear32(void) {
    add ip, r1, r2
    b L_02000940
L_02000940:
    cmp r1, ip
    stmltia r1!, {r0}
    blt L_02000940
    bx lr
}
// clang-format on
