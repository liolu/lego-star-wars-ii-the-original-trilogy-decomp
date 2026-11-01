#include "m2c.h"

typedef struct {
    u8 unk_00[8];
    u32 flags;
} Menu;

typedef struct {
    u8 unk_00[6];
    u8 enabled;
} Slot;

typedef struct {
    u8 unk_00[0x30];
    Menu *menu;
    u8 unk_34[8];
    u8 unk_3c[0xa4];
    u8 flag_e0;
    u8 flag_e1;
    u8 flag_e2;
    u8 flag_e3;
    u8 unk_e4[8];
    u32 state;
    s32 unk_f0;
} UnkFile;

extern u32 data_027e02a8;
extern u8 data_0210930c[];
extern u8 data_02102900[];
extern u8 data_0210794c[];
extern u8 data_02106d90[];
extern u8 data_02104d90[];
extern u8 data_02104df4[];

extern void func_0205365c(void *a0, s32 a1);
extern void *func_0205e80c(s32, s32, s32);
extern void func_0205eb6c(void *arg0, void *arg1, s32 arg2);
extern void func_0205ecc4(void *arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5);
extern void func_020601ac(void *a0, s32 a1, s32 a2, s32 a3, s32 a4, s32 a5);
extern void *func_02069350(void *self, void *arg1, void *arg2, void *arg3, s32 arg4);
extern void *func_0206afa8(void *self, void *arg1, void *arg2, void *arg3);
extern void *func_02084c04(void *self, void *arg1, void *arg2, void *arg3);
extern s32 func_02085cb0(void *arg0, s32 arg1);
extern void func_0208642c(void *arg0);
extern s32 func_02086534(void *arg0, s32 arg1);
extern void func_0208896c(void *p);
extern s32 func_02088b84(void *arg0, u8 arg1);
extern void func_0208e874(UnkFile *file, s32 flag);
extern u8 func_020917b4(void *arg0);
extern void func_02093c6c(void *arg0, s32 arg1);
extern void func_020b04f0(void *obj, u8 id, void *src, u8 size);
extern void func_020b10a8(void *arg0);
extern void func_020b111c(void *arg0, s32 arg1);
extern void func_020b8f70(s32 arg0, s32 arg1);

typedef struct {
    u8 pad[0x6bb];
    u8 val;
} Slot2;
typedef struct {
    u8 pad[0x4c];
    Slot2 *slots[2];
} Ctx;

/*
 * Runs one step of the file object's state (0..5; other values do nothing).
 * When bit 0x40 of data_027e02a8 is set, most states only post a message id
 * through func_020b04f0. States 2, 3 and the flag_e1 branch of 4 allocate an
 * object with func_0205e80c (size, 3, 4), construct it when the allocation
 * succeeded and hand the result to func_0205eb6c. States 4 and 5 clear bit 0
 * of menu->flags and call func_020601ac when flag_e3 is clear.
 */
void func_0208ea58(UnkFile *file) {
    Slot *slot;
    void *obj;
    u8 i;
    s32 value;

    switch (file->state) {
        case 0:
            if (!(data_027e02a8 & 0x40)) {
                func_02093c6c(data_0210930c, 0);
                func_0205ecc4(data_02102900, file->unk_f0, 1, 0, 0, 0);
                return;
            }
            func_020b04f0(data_0210794c, 0x2c, 0, 0xff);
            return;
        case 1:
            slot = *(Slot **) (data_02106d90 + 0xbb8);
            if (slot->enabled) {
                if (data_027e02a8 & 0x40) {
                    func_020b04f0(data_0210794c, 0x15, 0, 0xff);
                } else {
                    func_020b111c(slot, 1);
                    func_02093c6c(data_0210930c, 0);
                    func_0205ecc4(data_02102900, file->unk_f0, 1, 0, 0, 0);
                }
            } else {
                if (data_027e02a8 & 0x40) {
                    func_020b04f0(data_0210794c, 0x2d, 0, 0xff);
                } else {
                    func_020b10a8(slot);
                    func_02093c6c(data_0210930c, 0);
                    data_02106d90[0xbb2] = func_020917b4(data_0210930c);
                    if (func_02085cb0(data_02104d90, 0)) {
                        func_0205ecc4(data_02102900, file->unk_f0, 1, 0, 0, 0);
                    } else {
                        value = func_020917b4(data_0210930c);
                        func_02088b84(data_02104df4, value);
                        func_0205ecc4(data_02102900, 0xf, 0, 0, 0, 0);
                    }
                }
            }
            func_0205365c(file->unk_34, 0);
            func_020b8f70(0xb4, 0);
            func_0208e874(file, 1);
            return;
        case 2:
            file->flag_e0 = 1;
            func_0205365c(file->unk_34, 0);
            file->menu->flags &= ~1;
            obj = func_0205e80c(0x50, 3, 4);
            if (obj != 0) {
                obj = func_0206afa8(obj, file, file->unk_3c, file->unk_34);
            }
            func_0205eb6c(file, obj, 0);
            return;
        case 3:
            file->flag_e0 = 1;
            func_0205365c(file->unk_34, 0);
            file->menu->flags &= ~1;
            obj = func_0205e80c(0x90, 3, 4);
            if (obj != 0) {
                obj = func_02084c04(obj, file, file->unk_3c, file->unk_34);
            }
            func_0205eb6c(file, obj, 0);
            return;
        case 4:
            if (file->flag_e1 == 0) {
                if (file->flag_e3 == 0) {
                    file->menu->flags &= ~1;
                    func_020601ac(data_02102900, 1, 0x45, 0x45 - 0x46, 2, 0);
                    return;
                }
                file->flag_e3 = 0;
                if (!(data_027e02a8 & 0x40)) {
                    func_02093c6c(data_0210930c, 0);
                    func_0208896c(data_02104df4);
                    for (i = 0; i < 2; i++) {
                        if ((((Ctx *) data_0210930c)->slots[i]) != 0 &&
                            func_02086534(data_02104d90, (((Ctx *) data_0210930c)->slots[i])->val)) {
                            u8 v = (((Ctx *) data_0210930c)->slots[i])->val;
                            if (v < 0x54) {
                                *(u16 *) (data_02104d90 + i * 0x40 + 0x2248) = v;
                                data_02104d90[i * 0x40 + 0x2262]             = 4;
                                data_02104d90[i * 0x40 + 0x2263]             = 1;
                            }
                        }
                    }
                    func_0205ecc4(data_02102900, 4, 0, 4, 0xe, 0);
                    func_0205ecc4(data_02102900, file->unk_f0, 1, 0, 0, 0);
                    func_0208642c(data_02104d90);
                    return;
                }
                func_020b04f0(data_0210794c, 0x3c, 0, 0xff);
                return;
            }
            if (data_027e02a8 & 0x40) {
                if (func_02085cb0(data_02104d90, 0) == 0) {
                    return;
                }
                func_020b04f0(data_0210794c, 0x2c, 0, 0xff);
                return;
            }
            file->flag_e0 = 1;
            file->menu->flags &= ~1;
            func_0205365c(file->unk_34, 0);
            obj = func_0205e80c(0x2c, 3, 4);
            if (obj != 0) {
                obj = func_02069350(obj, file, file->unk_3c, file->unk_34, 1);
            }
            func_0205eb6c(file, obj, 0);
            return;
        case 5:
            if (file->flag_e3 == 0) {
                file->menu->flags &= ~1;
                func_020601ac(data_02102900, 1, 0x45, 0x45 - 0x46, 2, 0);
                return;
            }
            if ((data_027e02a8 & 0x40) && file->flag_e1) {
                func_020b04f0(data_0210794c, 0x42, 0, 0xff);
                return;
            }
            if (file->flag_e1 == 0) {
                return;
            }
            file->flag_e3 = 0;
            func_02093c6c(data_0210930c, 0);
            slot = *(Slot **) (data_02106d90 + 0xbb8);
            if (slot->enabled) {
                func_020b111c(slot, 1);
            }
            func_0205ecc4(data_02102900, 0x11, 0, 0, 0, 0);
            func_0205ecc4(data_02102900, 0xf, 1, 0, 0, 0);
            return;
    }
}
