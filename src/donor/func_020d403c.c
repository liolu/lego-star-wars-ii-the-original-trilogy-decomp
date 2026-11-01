/* compiler: 2.0/sp1p5  */
/* func_02056640 from projets/tom-and-jerry-tales/src/auto/func_02056640.c (same code in another game of this workspace),
 * reused by tools/donor_match.py */
typedef unsigned short u16;
typedef long s32;
typedef unsigned long u32;
typedef struct {
    s32 pad[3];
    void *pre;
    s32 pad10;
    void *post;
} PmLists;
extern PmLists data_0211748c;
extern void PMi_ExecuteList(void *);
extern u32 OS_DisableInterrupts(void);
extern void OS_RestoreInterrupts(u32);
extern u32 OS_DisableIrqMask(u32);
extern u32 OS_SetIrqMask(u32);
extern s32 CTRDG_IsExisting(void);
extern s32 PM_GetLCDPower(void);
extern void PM_GetBackLight(u32 *, u32 *);
extern void PM_SetBackLight(s32, s32);
extern s32 func_020d3c38(u16, u16);
extern void OS_Halt(void);
extern void OS_SpinWait(u32);
extern void PMi_SetLCDPower(s32, s32, s32, s32);
extern void PMi_SetLED(s32);
extern void PM_ForceToPowerOff(void);
static inline u16 disableIrq(void) {
    u16 prev                    = *(volatile u16 *) 0x4000208;
    *(volatile u16 *) 0x4000208 = 0;
    return prev;
}
static inline u16 enableIrq(void) {
    u16 prev                    = *(volatile u16 *) 0x4000208;
    *(volatile u16 *) 0x4000208 = 1;
    return prev;
}
static inline u16 restoreIrq(u16 v) {
    u16 prev                    = *(volatile u16 *) 0x4000208;
    *(volatile u16 *) 0x4000208 = v;
    return prev;
}
static inline u32 getVCount(void) {
    return *(volatile u32 *) 0x27ffc3c;
}
void func_020d403c(u32 trigger, u16 logic, u16 keys) {
    u16 preIme;
    u32 enabled;
    u32 preIe;
    s32 powerOff = 0;
    u32 preGx;
    u32 preGxs;
    s32 lcd;
    u32 led0;
    u32 led1;
    volatile u32 cnt;
    u16 param;
    u16 keyParam;

    PMi_ExecuteList(data_0211748c.pre);
    preIme  = disableIrq();
    enabled = OS_DisableInterrupts();
    preIe   = OS_DisableIrqMask(0x3fffff);
    OS_SetIrqMask(0x40000);
    OS_RestoreInterrupts(enabled);
    enableIrq();
    if (trigger & 8) {
        if (*(u16 *) 0x27ffc40 == 2) {
            trigger &= ~8;
        }
    }
    if (trigger & 0x10) {
        if (CTRDG_IsExisting() == 0) {
            trigger &= ~0x10;
        }
    }
    preGx  = *(volatile u32 *) 0x4000000;
    preGxs = *(volatile u32 *) 0x4001000;
    lcd    = PM_GetLCDPower();
    PM_GetBackLight(&led0, &led1);
    PM_SetBackLight(2, 0);
    cnt = getVCount();
    while (cnt == getVCount()) {
    }
    cnt = getVCount();
    *(volatile u32 *) 0x4000000 &= ~0x30000;
    *(volatile u32 *) 0x4001000 &= ~0x10000;
    while (cnt == getVCount()) {
    }
    cnt = getVCount();
    while (cnt == getVCount()) {
    }
    param = trigger | (led0 << 5) | (led1 << 6);
    while (func_020d3c38(param, logic | keys) != 0) {
    }
    OS_Halt();
    OS_SpinWait(0x996a00);
    if (trigger & 8) {
        if (*(volatile u32 *) 0x4000214 & 0x100000) {
            powerOff = 1;
        }
    }
    if (!powerOff) {
        if (lcd == 1) {
            PMi_SetLCDPower(1, 1, 1, 0);
        } else {
            PMi_SetLED(1);
        }
        *(volatile u32 *) 0x4000000 = preGx;
        *(volatile u32 *) 0x4001000 = preGxs;
    }
    OS_DisableInterrupts();
    OS_SetIrqMask(preIe);
    OS_RestoreInterrupts(enabled);
    restoreIrq(preIme);
    if (powerOff) {
        PM_ForceToPowerOff();
    }
    PMi_ExecuteList(data_0211748c.post);
}
