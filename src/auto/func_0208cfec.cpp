#include "m2c.h"

// Partial interface: unknown virtuals retain their observed vtable slots.
// Declarations only: the original vtable remains in the ROM data.
class Entity {
public:
    virtual void unknown00();
    virtual void unknown04();
    virtual void unknown08();
    virtual void unknown0c();
    virtual void unknown10();
    virtual void unknown14();
    virtual void unknown18();
    virtual void unknown1c();
    virtual void unknown20();
    virtual void action(int, int, int, int); // Slot 0x24.
    u32 unk4;
    u32 flags;
    u8 pad0C[0x34];
    u8 unk40;
    u8 pad41[0x167];
    u16 unk1A8;
    u8 pad1AA[0x42C];
    u8 unk5D6;
};

struct Game {
    u8 pad[0x4C];
    Entity *entities[4];
};

struct Owner {
    u8 pad0[0x2C];
    u8 sub[0x78];
    u32 unkA4;
};

extern "C" M2C_UNK func_02001688(s32);
extern "C" M2C_UNK func_020016c0(void);
extern "C" M2C_UNK func_0208aaf0(void *);
extern "C" M2C_UNK func_0208b374(void *, s32);
extern "C" M2C_UNK func_0208b3f0(void *, s32);
extern "C" M2C_UNK func_0208b54c(void *, s32);
extern "C" M2C_UNK func_0208bf58(void *);
extern "C" M2C_UNK func_0208bf74(void *);
extern "C" M2C_UNK func_0208bfec(void *);
extern "C" M2C_UNK func_0208c024(void *, s32, s32, s32);
extern "C" Game data_0210930c;

extern "C" s32 func_0208cfec(Owner *arg0, s32 arg1, s32 *arg2) {
    u32 i;
    Entity *e;

    switch (arg1) {
        case 7:
            for (i = 0; i < 4; i++) {
                e = data_0210930c.entities[i];
                if (e != NULL) {
                    e->unk5D6 = 1;
                    e->unk1A8 |= 0x10;
                    e->flags |= 0x200;
                    if (e->unk40 != 0) {
                        e->action(0, 1, 0, 0);
                    }
                }
            }
            func_0208bf58(arg0->sub);
            return 1;
        case 17:
            arg0->unkA4 |= 2;
            return 1;
        case 25:
            func_0208b374(arg0->sub, arg2[0]);
            return 1;
        case 26:
            func_0208b3f0(arg0->sub, arg2[0]);
            return 1;
        case 18:
            func_0208bf74(arg0->sub);
            return 1;
        case 19:
            func_0208bfec(arg0->sub);
            func_02001688(0);
            func_0208aaf0(arg0->sub);
            func_020016c0();
            return 1;
        case 20:
            func_0208b54c(arg0->sub, arg2[0]);
            return 1;
        case 21:
            func_0208c024(arg0->sub, arg2[0], arg2[1], arg2[2]);
            return 1;
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 22:
        case 23:
        case 24:
        default:
            return 0;
    }
}
