#include "m2c.h"

// ASM: 16-tap s8 FIR filter with qadd/qdadd, `subne lr, pc, #12` and a jump into the middle of the unrolled loop.
// clang-format off
asm void func_02001084(void) {
    stmdb sp!, {r4, r5, r6, r7, r8, r9, r10, r11, lr}
    ldmia r0, {r4, r5, r6, r7}
    stmdb sp!, {r4, r5, r6, r7}
    ldr r12, [sp, #52]
    cmp r12, #0
    bne L_020010b0
    mov r4, #0
    mov r5, #0
    stmdb sp!, {r4, r5}
    stmdb sp!, {r4, r5}
    b L_020010b8
L_020010b0:
    ldmia r12, {r4, r5, r6, r7}
    stmdb sp!, {r4, r5, r6, r7}
L_020010b8:
    ldmia r3, {r4, r5, r6, r7, r8, r9, r10, r11}
    add r12, r0, r2
    str r2, [sp, #-8]!
    tst r2, #3
    subne lr, pc, #12
    bne L_020012e0
    mov r2, r2, lsl #1
    sub r2, r2, #1
    sub r2, r2, #32
    add r3, r1, #20
    stmia sp, {r2, r3}
    cmp r2, #0
    bmi L_020012bc
    ldr lr, [r12, #-4]!
L_020010f0:
    bic lr, lr, #255
    bic lr, lr, #16711680
    smultb r3, lr, r4
    smlabb r3, lr, r5, r3
    smulbt r2, lr, r4
    smulbb r1, lr, r4
    ldr lr, [r12]
    mov lr, lr, lsl #8
    bic lr, lr, #16711680
    smlatt r3, lr, r4, r3
    smlabt r3, lr, r5, r3
    smlatb r2, lr, r4, r2
    smlabb r2, lr, r5, r2
    smlabt r1, lr, r4, r1
    smulbb r0, lr, r4
    ldr lr, [r12, #-4]!
    bic lr, lr, #255
    bic lr, lr, #16711680
    smlatb r3, lr, r6, r3
    smlabb r3, lr, r7, r3
    smlatt r2, lr, r5, r2
    smlabt r2, lr, r6, r2
    smlatb r1, lr, r5, r1
    smlabb r1, lr, r6, r1
    smlatt r0, lr, r4, r0
    smlabt r0, lr, r5, r0
    ldr lr, [r12]
    mov lr, lr, lsl #8
    bic lr, lr, #16711680
    smlatt r3, lr, r6, r3
    smlabt r3, lr, r7, r3
    smlatb r2, lr, r6, r2
    smlabb r2, lr, r7, r2
    smlatt r1, lr, r5, r1
    smlabt r1, lr, r6, r1
    smlatb r0, lr, r5, r0
    smlabb r0, lr, r6, r0
    ldr lr, [r12, #-4]!
    bic lr, lr, #255
    bic lr, lr, #16711680
    smlatb r3, lr, r8, r3
    smlabb r3, lr, r9, r3
    smlatt r2, lr, r7, r2
    smlabt r2, lr, r8, r2
    smlatb r1, lr, r7, r1
    smlabb r1, lr, r8, r1
    smlatt r0, lr, r6, r0
    smlabt r0, lr, r7, r0
    ldr lr, [r12]
    mov lr, lr, lsl #8
    bic lr, lr, #16711680
    smlatt r3, lr, r8, r3
    smlabt r3, lr, r9, r3
    smlatb r2, lr, r8, r2
    smlabb r2, lr, r9, r2
    smlatt r1, lr, r7, r1
    smlabt r1, lr, r8, r1
    smlatb r0, lr, r7, r0
    smlabb r0, lr, r8, r0
    ldr lr, [r12, #-4]!
    bic lr, lr, #255
    bic lr, lr, #16711680
    smlatb r3, lr, r10, r3
    smlabb r3, lr, r11, r3
    smlatt r2, lr, r9, r2
    smlabt r2, lr, r10, r2
    smlatb r1, lr, r9, r1
    smlabb r1, lr, r10, r1
    smlatt r0, lr, r8, r0
    smlabt r0, lr, r9, r0
    ldr lr, [r12]
    mov lr, lr, lsl #8
    bic lr, lr, #16711680
    smlatt r3, lr, r10, r3
    smlabt r3, lr, r11, r3
    smlatb r2, lr, r10, r2
    smlabb r2, lr, r11, r2
    smlatt r1, lr, r9, r1
    smlabt r1, lr, r10, r1
    smlatb r0, lr, r9, r0
    smlabb r0, lr, r10, r0
    ldr lr, [r12, #-4]!
    bic lr, lr, #255
    bic lr, lr, #16711680
    smlatt r2, lr, r11, r2
    smlatb r1, lr, r11, r1
    smlatt r0, lr, r10, r0
    smlabt r0, lr, r11, r0
    ldr lr, [r12]
    mov lr, lr, lsl #8
    bic lr, lr, #16711680
    smlatt r1, lr, r11, r1
    smlatb r0, lr, r11, r0
    qdadd lr, r0, r0
    qadd lr, lr, r0
    qdadd r0, r1, r1
    mov lr, lr, lsr #24
    qadd r0, r0, r1
    qdadd r1, r2, r2
    mov r0, r0, lsr #24
    qadd r1, r1, r2
    qdadd r2, r3, r3
    mov r1, r1, lsr #24
    qadd r2, r2, r3
    mov r2, r2, lsr #24
    orr r0, lr, r0, lsl #8
    ldr lr, [sp]
    orr r0, r0, r1, lsl #16
    ldr r1, [sp, #4]
    orr r0, r0, r2, lsl #24
    subs lr, lr, #8
    str lr, [sp]
    str r0, [r1, lr, asr #1]
    ldrgt lr, [r12, #12]!
    bgt L_020010f0
L_020012bc:
    ldrne r3, [sp, #4]
    movne r2, #32
    subne r3, r3, #20
    stmneia sp, {r2, r3}
    addne r12, sp, #40
    ldrne lr, [r12, #-4]!
    bne L_020010f0
    add sp, sp, #40
    ldmia sp!, {r4, r5, r6, r7, r8, r9, r10, r11, pc}
L_020012e0:
    ldrsb r2, [r12, #-1]!
    ldrsb r3, [r12, #-1]!
    smulbb r0, r2, r4
    ldrsb r2, [r12, #-1]!
    smlabt r0, r3, r4, r0
    ldrsb r3, [r12, #-1]!
    smlabb r0, r2, r5, r0
    ldrsb r2, [r12, #-1]!
    smlabt r0, r3, r5, r0
    ldrsb r3, [r12, #-1]!
    smlabb r0, r2, r6, r0
    ldrsb r2, [r12, #-1]!
    smlabt r0, r3, r6, r0
    ldrsb r3, [r12, #-1]!
    smlabb r0, r2, r7, r0
    ldrsb r2, [r12, #-1]!
    smlabt r0, r3, r7, r0
    ldrsb r3, [r12, #-1]!
    smlabb r0, r2, r8, r0
    ldrsb r2, [r12, #-1]!
    smlabt r0, r3, r8, r0
    ldrsb r3, [r12, #-1]!
    smlabb r0, r2, r9, r0
    ldrsb r2, [r12, #-1]!
    smlabt r0, r3, r9, r0
    ldrsb r3, [r12, #-1]!
    smlabb r0, r2, r10, r0
    ldrsb r2, [r12, #-1]!
    smlabt r0, r3, r10, r0
    ldrsb r3, [r12, #-1]!
    smlabb r0, r2, r11, r0
    add r12, r12, #15
    smlabt r0, r3, r11, r0
    ldr r2, [sp]
    mov r0, r0, asr #14
    cmp r0, #127
    movgt r0, #127
    cmnlt r0, #128
    mvnlt r0, #127
    subs r2, r2, #1
    str r2, [sp]
    strb r0, [r1, r2]
    bx lr
}
// clang-format on
