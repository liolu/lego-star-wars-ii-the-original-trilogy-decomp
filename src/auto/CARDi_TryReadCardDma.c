#include "m2c.h"

u32 func_020ce380(void);
s32 OS_DisableInterrupts(void);
void OS_RestoreInterrupts(s32);
void CARDi_SetCardDma(void);
void DC_InvalidateRange(u32, s32);
void DC_StoreRange(u32, s32);
void DC_WaitWriteBufferEmpty(void);
void func_020cdd74(u32, s32);
void OS_SetIrqFunction(s32, void (*)(void));
void OS_ResetRequestIrqMask(s32);
void OS_EnableIrqMask(s32);
void func_01ff8000(void);
void CARDi_OnReadCard(void);
typedef struct D {
    u8 pad[0x1c];
    u32 f1c;
    u32 f20;
    s32 f24;
    u32 f28;
} D;
extern D cardi_common;

s32 CARDi_TryReadCardDma(u32 *arg0) {
    s32 r4;
    s32 sv;
    s32 r1b;
    s32 r5;
    s32 r6;
    s32 r7;
    u32 r8;
    D *d;
    s32 r2;
    u32 t0;
    u32 t3;
    u32 r9;
    s32 r1;
    d  = &cardi_common;
    r5 = 0;
    r8 = d->f20;
    r6 = 0;
    r7 = 0;
    r1 = 0;
    r9 = r8 & 0x1F;
    r4 = d->f24;
    if (r9 == 0 && d->f28 <= 3) {
        r1 = 1;
    }
    if (r1 != 0) {
        t0  = func_020ce380();
        r1b = 1;
        r2  = 0;
        t3  = r8 + r4;
        if (t3 > (u32) &func_01ff8000 && r8 < 0x02000000U) {
            r2 = 1;
        }
        if (r2 == 0 && (t0 >= t3 || t0 + 0x4000 <= r8)) {
            r1b = 0;
        }
        if (r1b == 0) {
            r7 = 1;
        }
    }
    if (r7 != 0 && !((d->f1c | r4) & 0x1FF)) {
        r6 = 1;
    }
    if (r6 != 0 && r4 != 0) {
        r5 = 1;
    }
    arg0[1] = (*(u32 *) 0x027FFE60 & ~0x07000000) | 0xA1000000;
    if (r5 != 0) {
        sv = OS_DisableInterrupts();
        func_020cdd74(r8, r4);
        if (r9 != 0) {
            r8 -= r9;
            DC_StoreRange(r8, 0x20);
            DC_StoreRange(r8 + r4, 0x20);
            r4 += 0x20;
        }
        DC_InvalidateRange(r8, r4);
        DC_WaitWriteBufferEmpty();
        OS_SetIrqFunction(0x80000, CARDi_OnReadCard);
        OS_ResetRequestIrqMask(0x80000);
        OS_EnableIrqMask(0x80000);
        OS_RestoreInterrupts(sv);
        CARDi_SetCardDma();
    }
    return r5;
}
