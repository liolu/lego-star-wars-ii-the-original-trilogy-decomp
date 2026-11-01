#include "m2c.h"

// Partial interface: unknown virtuals retain their observed vtable slots.
// Declarations only: the original vtable remains in the ROM data.
class Obj {
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
    virtual void unknown24(s32, s32, s32, s32); // Slot 0x24.
    virtual void unknown28();
    virtual void unknown2c();
    virtual void unknown30();
    virtual void unknown34();
    virtual void unknown38();
    virtual void unknown3c();
    virtual void unknown40();
    virtual int unknown44(); // Slot 0x44.
};

struct Entry {
    u8 b;
    u8 pad[0x23];
};

extern "C" u8 func_0204a590(void *, u16 *);
extern "C" void func_02005854(Obj *, u8, s32, s32, void *);
extern "C" const Entry data_020f6bc0[];

struct Fx {
    u16 v;
    Fx() {}
    Fx(u16 x) :
        v(x) {}
    Fx operator+(Fx o) const {
        return Fx(v + o.v);
    }
};

// Returns through a hidden result pointer (r0): the matched body of
// func_0204a5bc writes *arg0. The user constructor makes Fx non-POD, which
// selects that convention.
extern "C" Fx func_0204a5bc(void *);

static inline void update_value(Obj *p, Fx v) {
    if (M2C_FIELD(p, u16 *, 0xca) != v.v) {
        M2C_FIELD(p, u16 *, 0xca) = v.v;
        M2C_FIELD(p, u8 *, 0xd0)  = 1;
    }
}
static inline void set_target(Obj *p, Fx v) {
    M2C_FIELD(p, u16 *, 0x6b4) = v.v;
}

// arg1 != 0: sets counter +0x17 to 3, flag +0x19 to 1 and the owner timer
// +0x6c1 to 0. Otherwise ticks +0x6c1 down; while +0x17 is non-zero and
// (+0x19 or owner +0x5d2 bit 0x800) is set, a running +0x6c1 sets +0x19,
// else: if func_0204a590 yields a value at owner +0x5ce (its byte +0xd set;
// it then copies its halfword +0xe over the +0xca default in a), stores a plus
// func_0204a5bc(+0x5ce) at owner +0x6b4 and +0xca (marking +0xd0 when +0xca
// changes); calls virtual slot 0x24 (0x48, 0, 1, 0) and func_02005854
// (2nd/5th args 0, 4th 5, as func_0207fc34.c does); clears
// +0x19, decrements +0x17 and reloads +0x6c1 with 4, or with the owner's
// data_020f6bc0 row (index +0x6c3) when +0x17 reaches 0. Owner = arg0 +0x4;
// every call sets owner +0x8 bit 0x200.
extern "C" void func_02047540(void *arg0, s32 arg1) {
    Obj *p = M2C_FIELD(arg0, Obj **, 4);
    Fx a;
    Fx s;
    M2C_FIELD(p, u32 *, 8) |= 0x200;
    if (arg1 != 0) {
        M2C_FIELD(arg0, u8 *, 0x17) = 3;
        M2C_FIELD(arg0, u8 *, 0x19) = 1;
        M2C_FIELD(p, u8 *, 0x6c1)   = 0;
        return;
    }
    if (M2C_FIELD(p, u8 *, 0x6c1) != 0) {
        M2C_FIELD(p, u8 *, 0x6c1) -= 1;
    }
    if (M2C_FIELD(arg0, u8 *, 0x17) == 0) {
        return;
    }
    if (M2C_FIELD(arg0, u8 *, 0x19) == 0) {
        if ((M2C_FIELD(p, u16 *, 0x5d2) & 0x800) == 0) {
            return;
        }
    }
    if (M2C_FIELD(p, u8 *, 0x6c1) != 0) {
        M2C_FIELD(arg0, u8 *, 0x19) = 1;
        return;
    }
    a.v = M2C_FIELD(p, u16 *, 0xca);
    if (func_0204a590((s8 *) p + 0x5ce, &a.v) != 0) {
        s = func_0204a5bc((s8 *) p + 0x5ce) + a;
        set_target(p, s);
        update_value(p, s);
    }
    p->unknown24(0x48, 0, 1, 0);
    func_02005854(p, 0, 0, 5, NULL);
    M2C_FIELD(arg0, u8 *, 0x19) = 0;
    M2C_FIELD(arg0, u8 *, 0x17) -= 1;
    if (M2C_FIELD(arg0, u8 *, 0x17) != 0) {
        M2C_FIELD(p, u8 *, 0x6c1) = 4;
        return;
    }
    M2C_FIELD(p, u8 *, 0x6c1) = data_020f6bc0[M2C_FIELD(p, u8 *, 0x6c3)].b;
}
