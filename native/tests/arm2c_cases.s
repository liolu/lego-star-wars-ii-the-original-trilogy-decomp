@ Fonctions ARM/Thumb du test sans rom (arm2c_test.py), chargees a 0x02000000.
@ Les octets sont recopies dans CASES de arm2c_test.py (pas d'assembleur ARM sur le PC Windows) ;
@ apres un changement ici : python arm2c_test.py --regen (clang avec la cible ARM, par exemple sur macOS).
    .syntax unified
    .text

    .arm
    .org 0x000
    .globl t_ldr
t_ldr:                      @ LDRW : mot non aligne -> rotation
    ldr r0, [r0]
    bx lr
    .size t_ldr, .-t_ldr

    .org 0x020
    .globl t_store
t_store:                    @ str/strh/strb : adresse alignee vers le bas
    str r1, [r0]
    strh r1, [r0, #6]
    strb r1, [r0, #9]
    bx lr
    .size t_store, .-t_store

    .org 0x040
    .globl t_cmpflags
t_cmpflags:                 @ NZCV de cmp r0, r1 -> 8|4|2|1
    mov r2, #0
    cmp r0, r1
    orrmi r2, r2, #8
    orreq r2, r2, #4
    orrcs r2, r2, #2
    orrvs r2, r2, #1
    mov r0, r2
    bx lr
    .size t_cmpflags, .-t_cmpflags

    .org 0x080
    .globl t_addflags
t_addflags:                 @ NZCV de adds r0, r1
    mov r2, #0
    adds r3, r0, r1
    orrmi r2, r2, #8
    orreq r2, r2, #4
    orrcs r2, r2, #2
    orrvs r2, r2, #1
    mov r0, r2
    bx lr
    .size t_addflags, .-t_addflags

    .org 0x0C0
    .globl t_lsrs
t_lsrs:                     @ retenue du decaleur : C = bit sorti
    movs r1, r0, lsr #1
    mov r0, #0
    orrcs r0, r0, #2
    orreq r0, r0, #4
    bx lr
    .size t_lsrs, .-t_lsrs

    .org 0x0E0
    .globl t_smax
t_smax:                     @ conditions signees
    cmp r0, r1
    movlt r0, r1
    bx lr
    .size t_smax, .-t_smax

    .org 0x100
    .globl t_pack10
t_pack10:                   @ 10 arguments (4 registres + 6 sur la pile) -> r0:r1, un quartet chacun
    orr r0, r0, r1, lsl #4
    orr r0, r0, r2, lsl #8
    orr r0, r0, r3, lsl #12
    ldr r12, [sp]
    orr r0, r0, r12, lsl #16
    ldr r12, [sp, #4]
    orr r0, r0, r12, lsl #20
    ldr r12, [sp, #8]
    orr r0, r0, r12, lsl #24
    ldr r12, [sp, #12]
    orr r0, r0, r12, lsl #28
    ldr r1, [sp, #16]
    ldr r12, [sp, #20]
    orr r1, r1, r12, lsl #4
    bx lr
    .size t_pack10, .-t_pack10

    .macro args10
    sub sp, sp, #24
    mov r4, #5
    str r4, [sp]
    mov r4, #6
    str r4, [sp, #4]
    mov r4, #7
    str r4, [sp, #8]
    mov r4, #8
    str r4, [sp, #12]
    mov r4, #9
    str r4, [sp, #16]
    mov r4, #10
    str r4, [sp, #20]
    mov r0, #1
    mov r1, #2
    mov r2, #3
    mov r3, #4
    .endm

    .org 0x180
    .globl t_call10
t_call10:                   @ bl d'une fonction traduite avec 10 arguments
    push {r4, lr}
    args10
    bl t_pack10
    add sp, sp, #24
    pop {r4, pc}
    .size t_call10, .-t_call10

    .org 0x200
    .globl t_callptr
t_callptr:                  @ pointeur de fonction du pool litteral (h_pack10, Thumb, fonction C) + blx
    push {r4, lr}
    args10
    ldr r12, .Lh_pack10
    blx r12
    add sp, sp, #24
    pop {r4, pc}
.Lh_pack10:
    .word 0x02000F01
    .size t_callptr, .-t_callptr

    .org 0x280
    .globl t_callreg
t_callreg:                  @ r0 = adresse DS d'une fonction, r1 = son argument
    push {r4, lr}
    mov r12, r0
    mov r0, r1
    blx r12
    pop {r4, pc}
    .size t_callreg, .-t_callreg

    .org 0x2C0
    .globl t_tail
t_tail:                     @ saut terminal bx rX
    mov r12, r0
    mov r0, r1
    bx r12
    .size t_tail, .-t_tail

    .org 0x300
    .globl t_gosub
t_gosub:                    @ sous-routines internes : retours bx lr, mov pc lr, pop {pc} ; x -> 4x+11
    push {r4, lr}
    bl .Ldbl
    bl .Linc
    bl .Lnest
    pop {r4, pc}
.Ldbl:
    mov r0, r0, lsl #1
    bx lr
.Linc:
    add r0, r0, #1
    mov pc, lr
.Lnest:
    push {lr}
    bl .Ldbl
    add r0, r0, #9
    pop {pc}
    .size t_gosub, .-t_gosub

    .thumb
    .org 0x380
    .globl t_thumb
    .thumb_func
t_thumb:                    @ Thumb, sous-routine interne ; x -> 4x+9
    push {lr}
    bl .Ltsub
    bl .Ltsub
    pop {pc}
.Ltsub:
    lsls r0, r0, #1
    adds r0, #3
    bx lr
    .size t_thumb, .-t_thumb

    .arm
    .org 0x400
    .globl t_args14
t_args14:                   @ arguments de pile au-dela du 6e (formatage a la printf) : r0 = 14e argument, r1 = 11e
    ldr r0, [sp, #36]
    ldr r1, [sp, #24]
    bx lr
    .size t_args14, .-t_args14
