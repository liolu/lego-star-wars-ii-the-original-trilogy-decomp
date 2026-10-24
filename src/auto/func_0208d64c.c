#include "m2c.h"

typedef struct {
    u8 pad0[8];
    u8 f8;
    u8 pad9;
    u16 fA;
} S;

typedef struct {
    u8 f0;
    u8 f1;
    u8 f2;
    u8 f3;
} Byte4;

typedef struct {
    u32 x;
    u32 y;
} Out;

typedef struct {
    s16 a;
    s16 b;
} ShortPair;

extern u8 data_020ff495[];
extern u8 data_020ff4a0[];
extern void func_02054c80(Out *, void *, u16, u32, u32, u32, u32, u32, ShortPair *, Byte4);

void func_0208d64c(S *arg0, u8 arg1) {
    Out out;
    ShortPair pair;
    Byte4 bytes;
    u8 v0;
    u8 d;
    u8 q;

    if (arg0->fA == 0xffff) {
        return;
    }
    bytes.f0 = 0;
    v0       = data_020ff495[arg0->f8 * 0x14];
    d        = data_020ff4a0[arg0->f8 * 0x14];
    q        = (u8) (arg1 * v0) / d;
    bytes.f1 = q;
    bytes.f2 = 0x20;
    bytes.f3 = v0;
    pair.a   = 0;
    pair.b   = 0;
    func_02054c80(&out, arg0, arg0->fA, 0x10d, 1, 0xd0, 0, 0, &pair, bytes);
}
