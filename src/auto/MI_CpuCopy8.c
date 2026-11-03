#include "m2c.h"

// ASM: NitroSDK byte copy built from predicated halfword accesses and conditional bx lr tails.
// clang-format off
asm void MI_CpuCopy8(void) {
    cmp r2, #0
    bxeq lr
    tst r1, #1
    beq L_020cf41c
    ldrh r12, [r1, #-1]
    and r12, r12, #255
    tst r0, #1
    ldrneh r3, [r0, #-1]
    movne r3, r3, lsr #8
    ldreqh r3, [r0]
    orr r3, r12, r3, lsl #8
    strh r3, [r1, #-1]
    add r0, r0, #1
    add r1, r1, #1
    subs r2, r2, #1
    bxeq lr
L_020cf41c:
    eor r12, r1, r0
    tst r12, #1
    beq L_020cf474
    bic r0, r0, #1
    ldrh r12, [r0], #2
    mov r3, r12, lsr #8
    subs r2, r2, #2
    bcc L_020cf458
    b L_020cf440
L_020cf440:
    ldrh r12, [r0], #2
    orr r12, r3, r12, lsl #8
    strh r12, [r1], #2
    mov r3, r12, lsr #16
    subs r2, r2, #2
    bcs L_020cf440
L_020cf458:
    tst r2, #1
    bxeq lr
    ldrh r12, [r1]
    and r12, r12, #65280
    orr r12, r12, r3
    strh r12, [r1]
    bx lr
L_020cf474:
    tst r12, #2
    beq L_020cf4a4
    bics r3, r2, #1
    beq L_020cf4f4
    sub r2, r2, r3
    add r12, r3, r1
    b L_020cf490
L_020cf490:
    ldrh r3, [r0], #2
    strh r3, [r1], #2
    cmp r1, r12
    bcc L_020cf490
    b L_020cf4f4
L_020cf4a4:
    cmp r2, #2
    bcc L_020cf4f4
    tst r1, #2
    beq L_020cf4c4
    ldrh r3, [r0], #2
    strh r3, [r1], #2
    subs r2, r2, #2
    bxeq lr
L_020cf4c4:
    bics r3, r2, #3
    beq L_020cf4e8
    sub r2, r2, r3
    add r12, r3, r1
    b L_020cf4d8
L_020cf4d8:
    ldr r3, [r0], #4
    str r3, [r1], #4
    cmp r1, r12
    bcc L_020cf4d8
L_020cf4e8:
    tst r2, #2
    ldrneh r3, [r0], #2
    strneh r3, [r1], #2
L_020cf4f4:
    tst r2, #1
    bxeq lr
    ldrh r2, [r1]
    ldrh r0, [r0]
    and r2, r2, #65280
    and r0, r0, #255
    orr r0, r2, r0
    strh r0, [r1]
    bx lr
}
// clang-format on
