#include "m2c.h"

typedef struct Obj {
    u8 pad0[0x2A98];
    u8 a98[8];
    u8 aa0[0x2C];
    u8 acc[0x36];
    u8 b02;
    u8 pad_b03[3];
    u8 b06;
    s8 b07;
    u8 b08[4];
    u8 b0c[4];
    s8 b10[4];
    u8 b14[4];
} Obj;

typedef struct {
    u8 name[0x18];
} Name;

typedef struct {
    u8 pad[0x2A];
    u8 f2a;
} Globals;

extern void func_0205365c(void *, s32);
extern void func_02065600(void *, s32);
extern void func_0206574c(void *, s32, s32);
extern s32 func_02065be8(void *, s32, s32, s32, void *, s32);
extern void func_02066168(void *, u8 *);
extern void func_02066178(void *, void *);
extern void *func_02066708(s32);
extern s32 func_02073034(void);

extern Globals data_021027f8;
extern s32 data_020ffa00[];
extern u8 *data_02107860;

/* func_02099914 - run on the screen object set up by func_02098898. Works on
 * the part at +0x2ACC: two header func_02065be8 calls (the second uses the
 * data_020ffa00 entry picked by the s8 at +0x2B07), then one row per slot
 * i < func_02073034(), skipping slot data_021027f8.f2a. A row passes the
 * 0x18-byte name record at data_02107860 + 0x51 + i * 0x18 to func_02066168
 * and, when the per-slot flag at +0x2B14 is set, also the data_020ffa00 entry
 * picked by the per-slot s8 at +0x2B10. Each func_02065be8 result is passed to
 * the next call; rows after the skipped slot use offset (i - 1) * 0x18.
 * Callee and constant roles are not proven, so names stay positional. */
void func_02099914(Obj *self) {
    s32 y;
    Obj *slot;
    u8 i;
    s32 x;
    u8 *name;

    func_0205365c(self->a98, 0);
    func_0206574c(self->acc, 9, -2);
    y = func_02065be8(self->acc, 0x28, 0x80, 8, func_02066708(0x2A6), 1);
    func_0206574c(self->acc, 3, -2);
    if (self->b14[data_021027f8.f2a]) {
        func_0206574c(self->acc, 0xC, -2);
    }
    x = func_02065be8(self->acc, y, 0x80, 0x30, func_02066708(data_020ffa00[self->b07] + 0x1CB), 1);
    func_0206574c(self->acc, 0, -2);
    func_02065600(self->acc, 3);
    for (i = 0; i < func_02073034(); i++) {
        if (i != data_021027f8.f2a) {
            func_0206574c(self->acc, 0, -2);
            name = ((Name *) (data_02107860 + 0x51))[i].name;
            func_02066178(self->acc, func_02066708(0x294));
            /* byte view shifted by i: slot->b14[0] == self->b14[i] */
            slot = (Obj *) ((u8 *) self +
                            i); // FAKE: local declared before i so self + i gets r7 (a CSE temp is numbered last, r9)
            if (slot->b14[0]) {
                func_0206574c(self->acc, 0xC, -2);
                func_02066178(self->acc, func_02066708(data_020ffa00[slot->b10[0]] + 0x1CB));
            }
            func_02066168(self->acc, name);
            if (i < data_021027f8.f2a) {
                x = func_02065be8(self->acc, x, 0x18, i * 0x18 + 0x6B, func_02066708(0x293), 0);
            } else {
                x = func_02065be8(self->acc, x, 0x18, (i - 1) * 0x18 + 0x6B, func_02066708(0x293), 0);
            }
            func_0206574c(self->acc, 0, -2);
        }
    }
    func_02065600(self->acc, 4);
}
