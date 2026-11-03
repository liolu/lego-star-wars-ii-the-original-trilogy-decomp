#include "m2c.h"

// ASM: NitroSDK MI_UncompressHuffman hand asm: `mov r10, #1; and r9, r10, r5, lsr #31`, label-structured loops with `b` to the
// next instruction, hand-spilled [sp] and an r4-r11/lr prologue.
// clang-format off
asm void func_020cf544(void) {
    stmdb sp!, {r4, r5, r6, r7, r8, r9, r10, r11, lr}
    sub sp, sp, #4
    add r2, r0, #4
    add r7, r2, #1
    ldrb r10, [r0]
    and r4, r10, #15
    mov r3, #0
    mov lr, #0
    and r10, r4, #7
    add r11, r10, #4
    str r11, [sp]
    ldr r10, [r0]
    mov r12, r10, lsr #8
    ldrb r10, [r2]
    add r10, r10, #1
    add r0, r2, r10, lsl #1
    mov r2, r7
    b L_020cf58c
L_020cf58c:
    cmp r12, #0
    ble L_020cf618
    mov r8, #32
    ldr r5, [r0], #4
    b L_020cf5a0
L_020cf5a0:
    subs r8, r8, #1
    blt L_020cf58c
    mov r10, #1
    and r9, r10, r5, lsr #31
    ldrb r6, [r2]
    mov r6, r6, lsl r9
    mov r10, r2, lsr #1
    mov r10, r10, lsl #1
    ldrb r11, [r2]
    and r11, r11, #63
    add r11, r11, #1
    add r10, r10, r11, lsl #1
    add r2, r10, r9
    tst r6, #128
    beq L_020cf608
    mov r3, r3, lsr r4
    ldrb r10, [r2]
    rsb r11, r4, #32
    orr r3, r3, r10, lsl r11
    mov r2, r7
    add lr, lr, #1
    ldr r11, [sp]
    cmp lr, r11
    streq r3, [r1], #4
    subeq r12, r12, #4
    moveq lr, #0
L_020cf608:
    cmp r12, #0
    movgt r5, r5, lsl #1
    bgt L_020cf5a0
    b L_020cf58c
L_020cf618:
    add sp, sp, #4
    ldmia sp!, {r4, r5, r6, r7, r8, r9, r10, r11, lr}
    bx lr
}
// clang-format on
