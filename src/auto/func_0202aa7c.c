#include "m2c.h"

typedef struct {
    s32 v;
} Fx;

typedef struct {
    s32 v;
} UnkVal;

typedef struct {
    Fx qx;
    Fx qz;
    UnkVal md;
    Fx dx;
    Fx dy;
    Fx dz;
} Loc;

extern s16 func_020985d8(s32, s32);
extern s32 func_0201eec4(s32 *, s32 *, UnkVal);
extern void func_0204a5f8(void *, u16 *);

void func_0202aa7c(void *self, void *ctx) {
    u16 yaw;
    Loc l;
    s32 v0;
    s32 v1;
    s32 v2;
    s32 bits;
    u8 **tbl;

    bits = 0;
    if (M2C_FIELD(ctx, u8 *, 0x40) != 0) {
        if (M2C_FIELD(self, u8 *, 0x8d) != 0) {
            M2C_FIELD(self, u8 *, 0x8c) = 2;
        } else {
            M2C_FIELD(self, u8 *, 0x8c) = 0;
        }
    } else {
        l.md.v = 0x5000;
        if (M2C_FIELD(self, u8 *, 0x54) != 0 && M2C_FIELD(self, u8 *, 0x90) == 0) {
            tbl    = **(u8 ****) self;
            l.md.v = M2C_FIELD(tbl[M2C_FIELD((u8 *) self + M2C_FIELD(self, u8 *, 0x56) * 8, u8 *, 4)], s32 *, 0x20);
        }
        if (func_0201eec4((s32 *) ((u8 *) ctx + 0xb0), (s32 *) ((u8 *) self + 0x64), l.md) == 0) {
            v2     = M2C_FIELD(self, s32 *, 0x6c) - M2C_FIELD(ctx, s32 *, 0xb8);
            l.qz.v = v2;
            v1     = M2C_FIELD(self, s32 *, 0x68) - M2C_FIELD(ctx, s32 *, 0xb4);
            l.dy.v = v1;
            v0     = M2C_FIELD(self, s32 *, 0x64) - M2C_FIELD(ctx, s32 *, 0xb0);
            l.dx.v = v0;
            l.dy.v = v1;
            l.dz.v = v2;
            l.qz   = l.dz;
            l.qx   = l.dx;
            yaw    = func_020985d8(l.dx.v, l.dz.v);
            func_0204a5f8((u8 *) ctx + 0x5ce, &yaw);
            if (M2C_FIELD(self, u8 *, 0x8c) == 4 && M2C_FIELD(ctx, s32 *, 0x64) <= 0) {
                bits = 2;
            }
        }
    }
    M2C_FIELD(ctx, u16 *, 0x5ce) |= (u16) bits;
    M2C_FIELD(ctx, u16 *, 0x5d2) =
        M2C_FIELD(ctx, u16 *, 0x5ce) & (M2C_FIELD(ctx, u16 *, 0x5ce) ^ M2C_FIELD(ctx, u16 *, 0x5d0));
    M2C_FIELD(ctx, u16 *, 0x5d4) =
        M2C_FIELD(ctx, u16 *, 0x5d0) & (M2C_FIELD(ctx, u16 *, 0x5ce) ^ M2C_FIELD(ctx, u16 *, 0x5d0));
}
