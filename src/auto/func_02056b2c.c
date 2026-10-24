#include "m2c.h"

struct Vt {
    void *slots[7];
    s32 (*check)(void *self);
};

struct Iface {
    struct Vt *vt;
};

struct Slot {
    u8 pad0[8];
    u16 level;
    u16 vals[5];
    u8 pad1[0x23 - 0x14];
    u8 dirty;
    u8 pad2[0x40 - 0x24];
};

struct Table {
    u8 pad[0x2240];
    struct Slot slot[4];
};

extern u32 data_027e02a8;
extern struct Iface *data_021092f4;
extern u8 data_0210794c[];
extern struct Table data_02104d90;
extern u8 data_02102900[];

void func_02097bfc(u8 a, u8 b, s32 c, s32 d);
void func_020b04f0(void *obj, s32 id, void *buf, s32 len);

void func_02056b2c(void *unused, u8 *msg) {
    u8 buf[28];
    u32 i;
    u32 j;

    if ((data_027e02a8 & 0x40) && data_021092f4->vt->check(data_021092f4) == 0) {
        func_02097bfc(msg[0], msg[1], 0, 1);
        buf[0] = msg[3];
        buf[1] = msg[4];
        buf[2] = msg[5];
        buf[3] = msg[6];
        buf[4] = msg[7];
        buf[5] = msg[2];
        func_020b04f0(data_0210794c, 0x22, buf, 0xff);
        return;
    }
    if (data_027e02a8 & 0x40) {
        return;
    }
    func_02097bfc(msg[0], msg[1], 0, 1);
    if (msg[3] != 0) {
        for (i = 0; i < 4; i++) {
            for (j = 0; j < 5; j++) {
                data_02104d90.slot[i].vals[j] = 250;
            }
            data_02104d90.slot[i].level = 100;
            data_02104d90.slot[i].dirty = 0;
        }
        if (msg[4] != 100) {
            data_02104d90.slot[0].level = msg[4];
            data_02104d90.slot[0].dirty = 1;
        }
        if (msg[5] != 100) {
            data_02104d90.slot[1].level = msg[5];
            data_02104d90.slot[1].dirty = 1;
        }
        if (msg[6] != 100) {
            data_02104d90.slot[2].level = msg[6];
            data_02104d90.slot[2].dirty = 1;
        }
        if (msg[7] != 100) {
            data_02104d90.slot[3].level = msg[7];
            data_02104d90.slot[3].dirty = 1;
        }
    }
    *(u32 *) (data_02102900 + 0x58) = msg[2];
}
