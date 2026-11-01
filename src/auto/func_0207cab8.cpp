#include "m2c.h"

// Triple of 20.12 fixed-point values. Assignment skips a self copy, as the
// original code checks the destination against the source before copying.
class Vec3 {
public:
    s32 x;
    s32 y;
    s32 z;
    Vec3() {}
    Vec3(s32 ax, s32 ay, s32 az) {
        x = ax;
        y = ay;
        z = az;
    }
    Vec3 &operator=(const Vec3 &other) {
        if (this != &other) {
            x = other.x;
            y = other.y;
            z = other.z;
        }
        return *this;
    }
    s32 operator!=(const Vec3 &other) const {
        if (x != other.x || y != other.y || z != other.z) {
            return true;
        }
        return false;
    }
};

struct Cfg {
    u8 pad[0x24];
    s32 f24, f28, f2c;
    s32 f30, f34, f38;
    s16 f3c, f3e, f40;
    u8 gap[2];
    u8 f44, f45;
};

// Object built by the constructor func_0200ff50 (it stores the vtable pointer
// data_020fcf78 at offset 0). Only virtual slot 0x24 is used here; the other
// virtuals are declarations that keep that slot position.
class Widget {
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
    virtual void unknown24(s32, s32, s32, s32);
    u8 pad04[0xb0 - 4];
    Vec3 v_b0;
    Vec3 v_bc;
    u8 padc8[9];
    u8 dirty; // Set to 1 when either triple changes.

    void set_bc(const Vec3 &v) {
        if (v_bc != v) {
            v_bc  = v;
            dirty = 1;
        }
    }
    void set_bc_uniform(s32 s) {
        set_bc(Vec3(s, s, s));
    }
    void set_b0(const Vec3 &v) {
        if (v_b0 != v) {
            v_b0  = v;
            dirty = 1;
        }
    }
};

extern "C" M2C_UNK data_02104d90;
extern "C" M2C_UNK data_02104d94;
extern "C" M2C_UNK data_02104dc4;
extern "C" u8 data_02106d90[];
extern "C" s32 data_027e02a8;
extern "C" u8 data_020ff16c[];

extern "C" void *func_0200ff50(void *arg0, s32 arg1, s32 arg2, s32 arg3, void *arg4);
extern "C" void *func_0205e80c(s32, s32, s32);
extern "C" void func_0206574c(void *arg0, s32 arg1, s32 arg2);
extern "C" s32 func_02065be8(void *, s32, s32, s32, s32, s32);
extern "C" void func_020661b4(void *, s32);
extern "C" void *func_02066708(s32 arg0);
extern "C" s32 func_0208559c(void *arg0);
extern "C" void func_02086e1c(void *base, s32 bit);
extern "C" void func_02086ea0(void *base, s32 bit);
extern "C" u32 func_0208c7b0(void *arg0);
extern "C" s32 func_0208c7e8(void *arg0);
extern "C" s32 func_02094038(s32 arg0);

// Bit queried through func_0208c77c: func_0208c7b0 counts how many of its 16
// entries are set and func_0208c7e8 reports whether all 16 are set.
static inline s32 get_flag(void *p) {
    return func_0208c7e8(p);
}

enum {
    PERCENT_MAX          = 100,
    ALL_SET_BIT          = 0x13,
    OVERRIDE_MASK        = 0x40,
    STRING_ID            = 0x155,
    TEXT_X               = 0xd2,
    TEXT_Y               = 0xa6,
    TEXT_CENTRED         = 1,
    OBJECT_SIZE          = 0x1bc,
    OBJECT_TYPE          = 0x246,
    ID_ALL_FIRST         = 0xf4,
    ID_ALL_SECOND        = 0x89,
    ID_COUNT_FIRST_BASE  = 0xe3,
    ID_COUNT_SECOND_BASE = 0x78,
    FX_ONE               = 0x1000,
};

// Handler for case 0x1e of the dispatcher func_0207c7d8.
//
// 1. Express the value at offset 8 of data_02104d94 as a percentage of the
//    table word that func_02094038 returns for owner->0x2c, clamped to 100.
// 2. Count the set entries of data_02104dc4 (0..16). When all 16 are set,
//    set bit 0x13 in both bit tables of data_02104d90 (func_02086ea0 also runs
//    the matching table entries of kind 4). While bit 0x40 of data_027e02a8 is
//    set, the count and the all-set flag come from bytes 0x8d2 / 0x8d3 of
//    data_02106d90 instead.
// 3. Configure the element at owner+0x40 and pass it the percentage, then
//    hand string 0x155 (loaded into a 0x400-byte buffer by func_02066708) to
//    func_02065be8 at (210, 166); mode 1 subtracts half the measured width
//    from X, centring it. Its result replaces owner->0xa0.
// 4. If any entry is set, allocate a 0x1bc-byte object (func_0205e80c), build it
//    with func_0200ff50 (type 0x246) and store it at owner->0xac. Call its slot
//    0x24 twice with ids chosen by the all-set flag or derived from the count,
//    then set its two triples, marking the object dirty when one changes.
extern "C" void func_0207cab8(void *owner) {
    s32 percent;
    s32 set_count;
    s32 all_set;
    Widget *w;

    s32 raw   = func_0208559c(&data_02104d94);
    percent   = raw * PERCENT_MAX / func_02094038(M2C_FIELD(owner, s32 *, 0x2c));
    set_count = func_0208c7b0(&data_02104dc4);
    all_set   = get_flag(&data_02104dc4);
    if (all_set) {
        func_02086ea0(&data_02104d90, ALL_SET_BIT);
        func_02086e1c(&data_02104d90, ALL_SET_BIT);
    }
    if (data_027e02a8 & OVERRIDE_MASK) {
        set_count = data_02106d90[0x8d2];
        all_set   = data_02106d90[0x8d3] != 0;
    }
    if (percent > PERCENT_MAX) {
        percent = PERCENT_MAX;
    }
    func_0206574c((u8 *) owner + 0x40, 0xc, -1);
    func_020661b4((u8 *) owner + 0x40, percent);
    M2C_FIELD(owner, s32 *, 0xa0) = func_02065be8((u8 *) owner + 0x40, M2C_FIELD(owner, s32 *, 0xa0), TEXT_X, TEXT_Y,
                                                  (s32) func_02066708(STRING_ID), TEXT_CENTRED);
    if (set_count <= 0) {
        return;
    }
    w = (Widget *) func_0205e80c(OBJECT_SIZE, 3, 4);
    if (w != NULL) {
        // Construction record: three zero words, then three words of 1.0
        // (0x1000 in 20.12 fixed point).
        Cfg cfg;
        cfg.f24 = 0;
        cfg.f28 = 0;
        cfg.f2c = 0;
        cfg.f30 = FX_ONE;
        cfg.f34 = FX_ONE;
        cfg.f38 = FX_ONE;
        cfg.f3c = 0;
        cfg.f3e = 0;
        cfg.f40 = 0;
        cfg.f44 = 1;
        cfg.f45 = 1;
        w       = (Widget *) func_0200ff50(w, 4, OBJECT_TYPE, 0, &cfg);
    }
    M2C_FIELD(owner, Widget **, 0xac) = w;
    if (all_set) {
        M2C_FIELD(owner, Widget **, 0xac)->unknown24(ID_ALL_FIRST, 0, 1, 0);
        M2C_FIELD(owner, Widget **, 0xac)->unknown24(ID_ALL_SECOND, 0, 1, 0);
    } else {
        M2C_FIELD(owner, Widget **, 0xac)->unknown24(set_count + ID_COUNT_FIRST_BASE, 0, 1, 0);
        M2C_FIELD(owner, Widget **, 0xac)->unknown24(set_count + ID_COUNT_SECOND_BASE, 0, 1, 0);
    }
    // The table byte is an integer; << 12 converts it to fixed point.
    M2C_FIELD(owner, Widget **, 0xac)->set_bc_uniform(data_020ff16c[0xf4] << 12);
    M2C_FIELD(owner, Widget **, 0xac)->set_b0(Vec3(0x78000, 0x78000 - 0xdc000, 0x78000 - 0x1a4000));
}
