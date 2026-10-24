#include "m2c.h"

extern M2C_UNK data_021041c0;
extern M2C_UNK data_021041c1;
extern M2C_UNK data_021041c2;
extern u32 data_027e02a8;

typedef struct Obj {
    u32 pad0[2];
    volatile u32 flags;
    u32 pad1[2];
    u32 info;
} Obj;

void func_02060e6c(Obj *arg0) {
    u32 idx;
    u32 slot;
    volatile u8 *cnt;
    u32 left;

    if ((arg0->flags & 0x80000) != 0x80000) {
        return;
    }
    arg0->flags &= ~0x80000;
    slot = (arg0->info << 16) >> 28;
    if ((data_027e02a8 & 0x200) != 0) {
        idx = 0;
    } else if ((arg0->flags & 0x80000000) != 0) {
        idx = 0;
    } else {
        idx = 1;
    }
    cnt = (volatile u8 *) &data_021041c1 + idx * 0x84;
    if (cnt[slot * 8] == 0) {
        return;
    }
    left          = cnt[slot * 8] - 1;
    cnt[slot * 8] = left;
    if ((left & 0xFF) != 0) {
        return;
    }
    if (((u8 *) &data_021041c2 + idx * 0x84)[slot * 8] & 2) {
        return;
    }
    ((u8 *) &data_021041c0 + idx * 0x84)[slot * 8] = 0xFF;
}
