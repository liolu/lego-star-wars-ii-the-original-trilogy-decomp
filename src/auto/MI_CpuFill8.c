#include "m2c.h"

// ASM: NitroSDK byte fill built from predicated halfword stores and conditional bx lr tails.
// clang-format off
asm void MI_CpuFill8(void) {
    cmp r2, #0
    bxeq lr
    tst r0, #1
    beq L_020cf370
    ldrh r12, [r0, #-1]
    and r12, r12, #255
    orr r3, r12, r1, lsl #8
    strh r3, [r0, #-1]
    add r0, r0, #1
    subs r2, r2, #1
    bxeq lr
L_020cf370:
    cmp r2, #2
    bcc L_020cf3bc
    orr r1, r1, r1, lsl #8
    tst r0, #2
    beq L_020cf390
    strh r1, [r0], #2
    subs r2, r2, #2
    bxeq lr
L_020cf390:
    orr r1, r1, r1, lsl #16
    bics r3, r2, #3
    beq L_020cf3b4
    sub r2, r2, r3
    add r12, r3, r0
    b L_020cf3a8
L_020cf3a8:
    str r1, [r0], #4
    cmp r0, r12
    bcc L_020cf3a8
L_020cf3b4:
    tst r2, #2
    strneh r1, [r0], #2
L_020cf3bc:
    tst r2, #1
    bxeq lr
    ldrh r3, [r0]
    and r3, r3, #65280
    and r1, r1, #255
    orr r1, r1, r3
    strh r1, [r0]
    bx lr
}
// clang-format on
