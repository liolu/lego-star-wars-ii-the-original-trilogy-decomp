#include "m2c.h"

typedef struct {
    s32 v;
} Fx;

extern void func_02007f24(s32 *, f64);
extern void func_0200af6c(Fx *, Fx *);
extern void func_02019554(void *);
extern u8 data_020f4d16[];

enum {
    FX_SHIFT   = 12,    // 20.12 fixed point (func_02007f24 scales its f64 by 4096.0)
    FX_HALF    = 0x800, // half a unit: FxMul rounds to nearest
    ROW_STRIDE = 12,    // data_020f4d16 rows are 12 bytes, flags byte first
    ROW_FLAG_4 = 0x4,
    ROW_FLAG_8 = 0x8,
    MODE_1     = 1,
};

static inline s32 FxMul(s32 x, s32 y) {
    return (s32) (((s64) x * y + FX_HALF) >> FX_SHIFT);
}

static inline void FxSetTo(Fx *out, s32 v) {
    out->v = v;
}

static inline void set_b4(void *obj, Fx v) {
    Fx *p = &M2C_FIELD(obj, Fx *, 0xb4);
    if (p->v != v.v) {
        *p                         = v;
        M2C_FIELD(obj, u8 *, 0xd1) = 1;
    }
}

/* mode is the code passed by func_020192d8 (2) and func_02019418 (byte +4 of its second
   argument); mode 1 sets byte +0x1c0. The flags byte comes from the 12-byte row of
   data_020f4d16 picked by byte +0x1c3. Row flag 0x4 with mode 1 returns at once. Row flag
   0x8 with mode 1: when s32 +0x64 is negative, +0xb4 grows by its magnitude (byte +0xd1 set
   when the value changes) and +0x64, +0x60, +0x68 are scaled by -0.75, 0.8, 0.8; it then
   returns unless |+0x64| (func_0200af6c) is negative, which only INT_MIN gives. Every other
   path ends in func_02019554(obj). */
void func_02018f68(void *obj, s32 mode) {
    Fx n;
    s32 f1;
    s32 f2;
    s32 f3;
    Fx absv;
    u8 flags;
    s32 vy;
    Fx *pa;

    if (mode == MODE_1) {
        M2C_FIELD(obj, u8 *, 0x1c0) = 1;
    }
    flags = data_020f4d16[M2C_FIELD(obj, u8 *, 0x1c3) * ROW_STRIDE];
    if ((flags & ROW_FLAG_4) && mode == MODE_1) {
        return;
    }
    if ((flags & ROW_FLAG_8) && mode == MODE_1) {
        vy = M2C_FIELD(obj, s32 *, 0x64);
        if (vy < 0) {
            FxSetTo(&n, M2C_FIELD(obj, s32 *, 0xb4) - vy);
            set_b4(obj, n);
            func_02007f24(&f1, -0.75);
            M2C_FIELD(obj, s32 *, 0x64) = FxMul(M2C_FIELD(obj, s32 *, 0x64), f1);
            func_02007f24(&f2, 0.8);
            M2C_FIELD(obj, s32 *, 0x60) = FxMul(M2C_FIELD(obj, s32 *, 0x60), f2);
            func_02007f24(&f3, 0.8);
            M2C_FIELD(obj, s32 *, 0x68) = FxMul(M2C_FIELD(obj, s32 *, 0x68), f3);
        }
        func_0200af6c(&absv, (Fx *) ((u8 *) obj + 0x64));
        pa = &absv;
        // FAKE: always-true null test (pa = &absv); the block split keeps add r0,sp,#4; ldr r0,[r0] after the call
        if (pa) {
            if (pa->v >= 0) {
                return;
            }
        }
    }
    func_02019554(obj);
}
