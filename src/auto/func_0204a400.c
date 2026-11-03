#include "m2c.h"

void func_0204a1d4(void);
void func_0207f478(void);
extern u8 data_020ed4ec[];

// ASM: register-form bics (`bics r0, r1, r0`, flag test (a & ~b) == 0) that mwcc never emits from C; the rest is a plain table
// scan.
// clang-format off
asm void func_0204a400(void) {
    stmdb sp!, {r3, r4, r5, r6, r7, r8, r9, r10, r11, lr}
    mov r10, r0
    ldrb r0, [r10, #11]
    mov r9, r1
    mov r4, r2
    bl func_0207f478
    ldr r2, =data_020ed4ec
    mov r1, #13
    mla r8, r4, r1, r2
    mov r7, #0
    mov r6, r0
    mov r4, r7
    mov r5, #4
    mov r11, #6
L_0204a438:
    ldrb r0, [r8]
    cmp r0, #0
    beq L_0204a518
    add r0, r6, r7, lsl #3
    ldrb r0, [r0, #2]
    mov r2, r7, lsl #3
    cmp r0, #0
    beq L_0204a46c
    cmp r0, #1
    beq L_0204a490
    cmp r0, #2
    beq L_0204a4b4
    b L_0204a518
L_0204a46c:
    ldrh r1, [r10, #4]
    ldrh r0, [r6, r2]
    tst r1, r0
    beq L_0204a518
    mov r0, r10
    mov r1, r9
    mov r2, r7
    bl func_0204a1d4
    b L_0204a518
L_0204a490:
    ldrh r1, [r6, r2]
    ldrh r0, [r10]
    bics r0, r1, r0
    bne L_0204a518
    mov r0, r10
    mov r1, r9
    mov r2, r7
    bl func_0204a1d4
    b L_0204a518
L_0204a4b4:
    ldrh r0, [r10]
    tst r0, #256
    beq L_0204a518
    ldrh r0, [r10, #4]
    tst r0, #240
    beq L_0204a518
    mov r0, r10
    mov r1, r9
    mov r2, r7
    bl func_0204a1d4
    cmp r0, #0
    beq L_0204a518
    ldrh r0, [r10, #4]
    tst r0, #64
    strneb r5, [r10, #10]
    bne L_0204a4fc
    tst r0, #128
    strneb r4, [r10, #10]
L_0204a4fc:
    ldrh r0, [r10, #4]
    tst r0, #32
    strneb r11, [r10, #10]
    bne L_0204a518
    tst r0, #16
    movne r0, #2
    strneb r0, [r10, #10]
L_0204a518:
    add r7, r7, #1
    cmp r7, #13
    add r8, r8, #1
    bcc L_0204a438
    ldmia sp!, {r3, r4, r5, r6, r7, r8, r9, r10, r11, pc}
}
// clang-format on
