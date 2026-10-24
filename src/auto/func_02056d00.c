#include "m2c.h"

typedef struct {
    u8 pad0[8];
    u16 value;
    u16 limits[5];
    u8 pad1[0x23 - 0x14];
    u8 flag;
    u8 pad2[0x40 - 0x24];
} Slot;

typedef struct {
    u8 pad[0x2240];
    Slot slots[4];
} Big;

extern s32 data_027e02a8;
extern void **data_021092f4;
extern Big data_02104d90;
extern u8 data_0210794c[];
extern u8 data_02102900[];

extern void func_02097bfc(u32 a, u32 b, u32 c, u32 d);
extern void func_020b04f0(void *a, u32 b, void *c, u32 d);

void func_02056d00(s32 arg0, u8 *arg1) {
    u8 sp0[0x1c];
    u32 i;
    u32 j;

    if ((data_027e02a8 & 0x40) && (M2C_FIELD(*data_021092f4, s32(**)(), 0x1C)(data_021092f4) == 0)) {
        func_02097bfc(arg1[0], arg1[1], 0, 0);
        sp0[0] = arg1[3];
        sp0[1] = arg1[4];
        sp0[2] = arg1[5];
        sp0[3] = arg1[6];
        sp0[4] = arg1[7];
        sp0[5] = arg1[2];
        func_020b04f0(&data_0210794c[0], 0x22, sp0, 0xFF);
        return;
    }
    if (data_027e02a8 & 0x40) {
        return;
    }
    func_02097bfc(arg1[0], arg1[1], 0, 0);
    if (arg1[3] != 0) {
        for (i = 0; i < 4; i++) {
            for (j = 0; j < 5; j++) {
                data_02104d90.slots[i].limits[j] = 250;
            }
            data_02104d90.slots[i].value = 100;
            data_02104d90.slots[i].flag  = 0;
        }
        if (arg1[4] != 100) {
            data_02104d90.slots[0].value = arg1[4];
            data_02104d90.slots[0].flag  = 1;
        }
        if (arg1[5] != 100) {
            data_02104d90.slots[1].value = arg1[5];
            data_02104d90.slots[1].flag  = 1;
        }
        if (arg1[6] != 100) {
            data_02104d90.slots[2].value = arg1[6];
            data_02104d90.slots[2].flag  = 1;
        }
        if (arg1[7] != 100) {
            data_02104d90.slots[3].value = arg1[7];
            data_02104d90.slots[3].flag  = 1;
        }
    }
    M2C_FIELD(data_02102900, s32 *, 0x58) = arg1[2];
}
