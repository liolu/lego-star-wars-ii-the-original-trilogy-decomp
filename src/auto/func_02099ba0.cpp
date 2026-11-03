#include "m2c.h"

class Manager099ba0 {
public:
    virtual void unknown00();
    virtual void unknown04();
    virtual void unknown08();
    virtual void unknown0c();
    virtual void unknown10();
    virtual void unknown14();
    virtual void unknown18();
    virtual s32 unknown1c(); // Slot 0x1c.
};

typedef struct {
    u8 slot;
    s8 value;
} SlotPair;

typedef struct {
    u8 pad00[8];
    u16 f08;
    u8 pad0a[0x23 - 0x0a];
    u8 f23;
    u8 pad24[0x40 - 0x24];
} Player;

typedef struct {
    u8 pad[0x2240];
    Player players[8];
} Game;

typedef struct {
    u8 pad[0x2b04];
    u16 f2b04;
    u8 f2b06;
    u8 pad2b07[0x2b10 - 0x2b07];
    s8 team[4];
    u8 flag[4];
} Self;

extern "C" void func_02001688(s32);
extern "C" void func_020016c0(void);
extern "C" void func_0205ecc4(void *, M2C_UNK, M2C_UNK, M2C_UNK, s32, s32);
extern "C" s32 func_02073034(void);
extern "C" void func_02099914(void *);

extern "C" Manager099ba0 *data_021092f4;
extern "C" u32 data_02107654;
extern "C" u32 data_02101ad4;
extern "C" u32 data_020ffa00[];
extern "C" Game data_02104d90;
extern "C" u8 data_02106d90[];
extern "C" M2C_UNK data_02102900;

static inline u32 *data_02107654_ptr(void) {
    return &data_02107654;
}

static inline void copy2(void *dst, const void *src) {
    u8 *d       = (u8 *) dst;
    const u8 *s = (const u8 *) src;
    u8 a        = s[0];
    u8 b        = s[1];
    d[0]        = a;
    d[1]        = b;
}

static inline void refresh(Self *self) {
    u8 i = 0;
    if (func_02073034() > 0) {
        do {
            data_02104d90.players[i].f08 = data_020ffa00[self->team[i]];
            data_02104d90.players[i].f23 = 1;
            i++;
        } while (i < func_02073034());
    }
    func_0205ecc4(&data_02102900, 4, 0, 0xf, *(u8 *) (*(u8 **) (data_02106d90 + 0xad0) + 0x40), 0);
    func_0205ecc4(&data_02102900, 0x2a, 1, 0, 0, 0);
}

// arg1 is the payload kind (only 3 and 9 do anything); arg2 points at the payload bytes.
// Kind 9: u8 slot at +0, s8 value at +1. Kind 3: u32 at +0, plus a u16 at +4 that is kept
// only when the slot-0x1c call on *data_021092f4 returns nonzero.
extern "C" void func_02099ba0(Self *arg0, s32 arg1, u8 *arg2) {
    SlotPair pair;
    u32 *dst;
    u32 v;

    func_02001688(1);
    switch (arg1) {
        case 9:
            copy2(&pair, arg2);
            arg0->team[pair.slot] = pair.value;
            arg0->flag[pair.slot] = 1;
            func_02099914(arg0);
            arg0->f2b06++;
            break;
        case 3:
            dst = data_02107654_ptr();
        // FAKE: always-true null test keeps dst out of copy propagation (register colouring)
            if (dst) {
                v             = *(u32 *) arg2;
                *dst          = v;
                data_02101ad4 = v;
            }
            if (data_021092f4->unknown1c() != 0) {
                refresh(arg0);
                arg0->f2b04 = *(u16 *) (arg2 + 4);
            } else {
                refresh(arg0);
            }
            break;
    }
    func_020016c0();
}
