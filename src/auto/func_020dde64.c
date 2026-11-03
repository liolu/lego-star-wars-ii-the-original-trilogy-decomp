#include "m2c.h"

void r11(void);

// ASM: clz, swp/swpb element swaps, a manual stack buffer (`sub sp, sp, r2`) and a compare against sp; no C form produces
// this.
// clang-format off
asm void func_020dde64(void) {
    stmdb sp!, {r4, r5, r6, r7, r8, r9, r10, r11, lr}
    cmp r1, #1
    ble L_020de0f8
    ldr r4, [sp, #36]
    mov r11, r3
    mov r8, r2
    cmp r4, #0
    bne L_020dde9c
    clz r2, r1
    rsb r2, r2, #32
    mov r2, r2, lsl #3
    sub sp, sp, r2
    mov r4, sp
    str r2, [sp, #-4]!
L_020dde9c:
    sub r1, r1, #1
    mla r1, r1, r8, r0
    mov r5, r4
    str r0, [r4], #4
    str r1, [r4], #4
    clz r2, r8
    rsb r2, r2, #32
    str r2, [sp, #-4]!
    b L_020ddec0
L_020ddec0:
    cmp r4, r5
    beq L_020de0f8
    ldr r7, [r4, #-4]
    ldr r6, [r4, #-8]!
    sub r2, r7, r6
    cmp r2, r8
    bne L_020ddf3c
    mov r0, r6
    mov r1, r7
    blx r11
    cmp r0, #0
    ble L_020ddec0
    mov r0, r8
    tst r0, #3
    beq L_020ddf1c
    b L_020ddf00
L_020ddf00:
    ldrb r1, [r6]
    subs r0, r0, #1
    swpb r1, r1, [r7]
    add r7, r7, #1
    strb r1, [r6], #1
    bne L_020ddf00
    b L_020ddec0
L_020ddf1c:
    b L_020ddf20
L_020ddf20:
    ldr r1, [r6]
    subs r0, r0, #4
    swp r1, r1, [r7]
    add r7, r7, #4
    str r1, [r6], #4
    bne L_020ddf20
    b L_020ddec0
L_020ddf3c:
    ldr r3, [sp]
    sub r2, r7, r6
    mov r2, r2, lsr r3
    mla r2, r2, r8, r6
    mov r3, r6
    mov r0, r8
    mov r2, r2
    tst r0, #3
    beq L_020ddf80
    b L_020ddf64
L_020ddf64:
    ldrb r1, [r2]
    subs r0, r0, #1
    swpb r1, r1, [r3]
    add r3, r3, #1
    strb r1, [r2], #1
    bne L_020ddf64
    b L_020ddf9c
L_020ddf80:
    b L_020ddf84
L_020ddf84:
    ldr r1, [r2]
    subs r0, r0, #4
    swp r1, r1, [r3]
    add r3, r3, #4
    str r1, [r2], #4
    bne L_020ddf84
L_020ddf9c:
    mov r9, r6
    mov r10, r7
    add r9, r9, r8
    b L_020ddfac
L_020ddfac:
    cmp r9, r7
    bge L_020ddfcc
    mov r1, r6
    mov r0, r9
    blx r11
    cmp r0, #0
    addlt r9, r9, r8
    blt L_020ddfac
L_020ddfcc:
    b L_020ddfd0
L_020ddfd0:
    mov r1, r6
    mov r0, r10
    blx r11
    cmp r0, #0
    subgt r10, r10, r8
    bgt L_020ddfd0
    cmp r9, r10
    bge L_020de050
    mov r2, r9
    mov r3, r10
    mov r0, r8
    tst r0, #3
    beq L_020de024
    b L_020de008
L_020de008:
    ldrb r1, [r2]
    subs r0, r0, #1
    swpb r1, r1, [r3]
    add r3, r3, #1
    strb r1, [r2], #1
    bne L_020de008
    b L_020de040
L_020de024:
    b L_020de028
L_020de028:
    ldr r1, [r2]
    subs r0, r0, #4
    swp r1, r1, [r3]
    add r3, r3, #4
    str r1, [r2], #4
    bne L_020de028
L_020de040:
    add r9, r9, r8
    sub r10, r10, r8
    cmp r9, r10
    ble L_020ddfac
L_020de050:
    mov r2, r6
    mov r3, r10
    mov r0, r8
    tst r0, #3
    beq L_020de084
    b L_020de068
L_020de068:
    ldrb r1, [r2]
    subs r0, r0, #1
    swpb r1, r1, [r3]
    add r3, r3, #1
    strb r1, [r2], #1
    bne L_020de068
    b L_020de0a0
L_020de084:
    b L_020de088
L_020de088:
    ldr r1, [r2]
    subs r0, r0, #4
    swp r1, r1, [r3]
    add r3, r3, #4
    str r1, [r2], #4
    bne L_020de088
L_020de0a0:
    sub r2, r10, r6
    sub r3, r7, r10
    cmp r2, r3
    ble L_020de0d4
    sub r2, r10, r8
    cmp r6, r2
    strlt r6, [r4], #4
    strlt r2, [r4], #4
    add r2, r10, r8
    cmp r2, r7
    strlt r2, [r4], #4
    strlt r7, [r4], #4
    b L_020ddec0
L_020de0d4:
    add r2, r10, r8
    cmp r2, r7
    strlt r2, [r4], #4
    strlt r7, [r4], #4
    sub r2, r10, r8
    cmp r6, r2
    strlt r6, [r4], #4
    strlt r2, [r4], #4
    b L_020ddec0
L_020de0f8:
    add sp, sp, #4
    sub r4, r4, #4
    cmp r4, sp
    ldreq r0, [sp]
    addeq r0, r0, #4
    addeq sp, sp, r0
    ldmia sp!, {r4, r5, r6, r7, r8, r9, r10, r11, lr}
    bx lr
}
// clang-format on
