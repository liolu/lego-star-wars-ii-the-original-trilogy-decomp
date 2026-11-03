#include "m2c.h"

// ASM: MSL double to unsigned long long conversion (hand asm): shifted exponent decode on r0/r1.
// clang-format off
asm void _ll_ufrom_d(void) {
    tst r1, #-2147483648
    bne L_020e2188
    ldr r2, =0x43e
    subs r2, r2, r1, lsr #20
    blt L_020e21a0
    cmp r2, #64
    bge L_020e217c
    mov r12, r1, lsl #11
    orr r12, r12, #-2147483648
    orr r12, r12, r0, lsr #21
    cmp r2, #32
    ble L_020e2164
    sub r2, r2, #32
    mov r1, #0
    mov r0, r12, lsr r2
    bx lr
L_020e2164:
    mov r3, r0, lsl #11
    mov r1, r12, lsr r2
    mov r0, r3, lsr r2
    rsb r2, r2, #32
    orr r0, r0, r12, lsl r2
    bx lr
L_020e217c:
    mov r1, #0
    mov r0, #0
    bx lr
L_020e2188:
    cmn r1, #1048576
    cmpeq r0, #0
    bhi L_020e21a0
    mov r1, #0
    mov r0, #0
    bx lr
L_020e21a0:
    mvn r1, #0
    mvn r0, #0
    bx lr
}
// clang-format on
