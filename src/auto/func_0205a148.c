#include "m2c.h"

extern s32 func_0205a208(void *arg0);
extern s32 func_0205a2a8(void *arg0);
extern void NNS_G3dGeBufferOP_N(s32 op, void *param, s32 count);

void func_0205a148(void *arg0) {
    u32 sp0;
    u32 color0;
    u32 sizeT;
    u32 sizeS;
    u32 offset;
    s32 fmt;

    if (M2C_FIELD(arg0, u8 *, 9) == 0) {
        return;
    }
    offset = (M2C_FIELD(arg0, u32 *, 0x10) & 0xFFFF) << 3;
    fmt    = M2C_FIELD(arg0, s32 *, 0xC);
    switch (fmt) {
        case 1:
        case 6:
            color0 = 0;
            break;
        default:
            color0 = 1;
            break;
    }
    sizeT = func_0205a2a8(arg0);
    sizeS = func_0205a208(arg0);
    sp0   = (offset >> 3) | (M2C_FIELD(arg0, s32 *, 0xC) << 26) | 0x40000000 | (sizeS << 20) | (sizeT << 23) | (color0 << 29);
    NNS_G3dGeBufferOP_N(0x2A, &sp0, 1);
}
