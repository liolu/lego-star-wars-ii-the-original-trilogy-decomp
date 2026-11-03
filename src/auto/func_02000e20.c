#include "m2c.h"

// ASM: 16-tap FIR with qadd saturation, a computed `add pc, pc, r4, lsl #2` entry and `subne lr, pc` loop re-entry.
// clang-format off
asm void func_02000e20(void) {
    stmdb sp!, {r4, r5, r6, r7, r8, r9, r10, r11, lr}
    cmp r2, #16
    blt L_02000f84
    ldmia r0, {r4, r5, r6, r7, r8, r9, r10, r11}
L_02000e30:
    stmdb sp!, {r4, r5, r6, r7, r8, r9, r10, r11}
    ldr r12, [sp, #68]
    cmp r12, #0
    moveq r4, #0
    moveq r5, #0
    moveq r6, #0
    moveq r7, #0
    stmeqdb sp!, {r4, r5, r6, r7}
    stmeqdb sp!, {r4, r5, r6, r7}
    ldmneia r12, {r4, r5, r6, r7, r8, r9, r10, r11}
    stmnedb sp!, {r4, r5, r6, r7, r8, r9, r10, r11}
    ldmia r3, {r4, r5, r6, r7, r8, r9, r10, r11}
    add r0, r0, r2, lsl #1
    add r1, r1, r2, lsl #1
    str r2, [sp, #-4]!
    cmp r2, #16
    blt L_02000fb4
    tst r2, #1
    blne L_02000fcc
    mov r2, r2, lsl #1
    sub r2, r2, #1
    subs r2, r2, #32
    str r2, [sp]
    bmi L_02000f68
    ldr r2, [r0, #-4]!
L_02000e94:
    ldr r3, [r0, #-4]!
    smulbb r12, r2, r4
    smultb lr, r2, r4
    smlabt lr, r2, r4, lr
    ldr r2, [r0, #-4]!
    smlatt r12, r3, r4, r12
    smlabb r12, r3, r5, r12
    smlatb lr, r3, r5, lr
    smlabt lr, r3, r5, lr
    ldr r3, [r0, #-4]!
    smlatt r12, r2, r5, r12
    smlabb r12, r2, r6, r12
    smlatb lr, r2, r6, lr
    smlabt lr, r2, r6, lr
    ldr r2, [r0, #-4]!
    smlatt r12, r3, r6, r12
    smlabb r12, r3, r7, r12
    smlatb lr, r3, r7, lr
    smlabt lr, r3, r7, lr
    ldr r3, [r0, #-4]!
    smlatt r12, r2, r7, r12
    smlabb r12, r2, r8, r12
    smlatb lr, r2, r8, lr
    smlabt lr, r2, r8, lr
    ldr r2, [r0, #-4]!
    smlatt r12, r3, r8, r12
    smlabb r12, r3, r9, r12
    smlatb lr, r3, r9, lr
    smlabt lr, r3, r9, lr
    ldr r3, [r0, #-4]!
    smlatt r12, r2, r9, r12
    smlabb r12, r2, r10, r12
    smlatb lr, r2, r10, lr
    smlabt lr, r2, r10, lr
    ldr r2, [r0, #-4]!
    smlatt r12, r3, r10, r12
    smlabb r12, r3, r11, r12
    smlatb lr, r3, r11, lr
    smlabt lr, r3, r11, lr
    smlatt r12, r2, r11, r12
    qdadd r3, r12, r12
    ldr r2, [sp]
    qadd r3, r3, r12
    qdadd r12, lr, lr
    subs r2, r2, #4
    qadd r12, r12, lr
    str r2, [sp]
    mov lr, r12, lsr #16
    ldr r2, [r0, #28]!
    mov r12, r3, lsr #16
    orr r12, r12, lr, lsl #16
    str r12, [r1, #-4]!
    bgt L_02000e94
L_02000f68:
    movne r2, #32
    strne r2, [sp]
    addne r0, sp, #68
    ldrne r2, [r0, #-4]!
    bne L_02000e94
    add sp, sp, #68
    ldmia sp!, {r4, r5, r6, r7, r8, r9, r10, r11, pc}
L_02000f84:
    rsb r4, r2, #14
    mov r4, r4, lsr #1
    add pc, pc, r4, lsl #2
    ldr r11, [r0, #28]
    ldr r10, [r0, #24]
    ldr r9, [r0, #20]
    ldr r8, [r0, #16]
    ldr r7, [r0, #12]
    ldr r6, [r0, #8]
    ldr r5, [r0, #4]
    ldr r4, [r0]
    b L_02000e30
L_02000fb4:
    add r0, sp, #36
    add r0, r0, r2, lsl #1
    subne lr, pc, #8
    bne L_02000fcc
    add sp, sp, #68
    ldmia sp!, {r4, r5, r6, r7, r8, r9, r10, r11, pc}
L_02000fcc:
    ldrsh r2, [r0, #-2]!
    ldrsh r3, [r0, #-2]!
    smulbb r12, r2, r4
    ldrsh r2, [r0, #-2]!
    smlabt r12, r3, r4, r12
    ldrsh r3, [r0, #-2]!
    smlabb r12, r2, r5, r12
    ldrsh r2, [r0, #-2]!
    smlabt r12, r3, r5, r12
    ldrsh r3, [r0, #-2]!
    smlabb r12, r2, r6, r12
    ldrsh r2, [r0, #-2]!
    smlabt r12, r3, r6, r12
    ldrsh r3, [r0, #-2]!
    smlabb r12, r2, r7, r12
    ldrsh r2, [r0, #-2]!
    smlabt r12, r3, r7, r12
    ldrsh r3, [r0, #-2]!
    smlabb r12, r2, r8, r12
    ldrsh r2, [r0, #-2]!
    smlabt r12, r3, r8, r12
    ldrsh r3, [r0, #-2]!
    smlabb r12, r2, r9, r12
    ldrsh r2, [r0, #-2]!
    smlabt r12, r3, r9, r12
    ldrsh r3, [r0, #-2]!
    smlabb r12, r2, r10, r12
    ldrsh r2, [r0, #-2]!
    smlabt r12, r3, r10, r12
    ldrsh r3, [r0, #-2]!
    smlabb r12, r2, r11, r12
    add r0, r0, #30
    smlabt r12, r3, r11, r12
    qdadd r3, r12, r12
    ldr r2, [sp]
    qadd r3, r3, r12
    mov r3, r3, lsr #16
    subs r2, r2, #1
    str r2, [sp]
    strh r3, [r1, #-2]!
    bx lr
}
// clang-format on
