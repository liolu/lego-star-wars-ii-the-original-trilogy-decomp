#include "m2c.h"

typedef struct Inner24 {
    u8 pad0[8];
    u16 flags8;
} Inner24;

typedef struct Inner4 {
    u8 pad0[0x20];
    u32 flags20;
    Inner24 *p24;
} Inner4;

typedef struct Obj {
    u8 pad0[4];
    Inner4 *p4;
    u8 pad8[0xC];
    void *p14;
    u8 pad18[4];
    u32 state;
    u8 pad20[9];
    u8 f29;
    u8 f2a;
    u8 f2b;
} Obj;

typedef struct Holder {
    u8 pad0[6];
    u8 f6;
} Holder;

extern u8 data_02102900[];
extern u8 data_02106d90[];
extern u8 data_0210794c[];
extern u32 data_027e02a8;

void func_02069a8c(void *, s32);
void func_0205365c(void *, s32);
void func_02060158(void *, s32, s32, s32);
void func_020b04f0(void *, s32, void *, s32);
void func_020b111c(void *, s32);
void func_02069be8(void *, s32);

void func_02069514(Obj *obj, s32 a1, s32 a2) {
    Holder *h;

    if (a1 == 1) {
        obj->f29 = 1;
        return;
    }
    obj->f29 = 0;
    if (obj->f2b != 0) {
        obj->f2b = 0;
        func_02069a8c(obj, 1);
        return;
    }
    if (obj->state > 2) {
        if (a2 == 1) {
            func_0205365c(obj->p14, 0);
            func_02060158(data_02102900, 1, 1, 0);
            obj->p4->flags20 |= 4;
            obj->p4->p24->flags8 |= 1;
            h = *(Holder **) (data_02106d90 + 0xBB8);
            if (h->f6 == 0) {
                return;
            }
            if (data_027e02a8 & 0x40) {
                func_020b04f0(data_0210794c, 0x15, 0, 0xFF);
            } else {
                func_020b111c(h, 1);
            }
        } else {
            func_02069be8(obj, obj->state);
        }
    } else if (a2 == 1) {
        data_027e02a8 |= 0x100;
        obj->f2a = 1;
    }
}
