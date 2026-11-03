#include "m2c.h"

typedef struct {
    u8 b0 : 1;
    u8 b1 : 1;
    u8 b2 : 1;
    u8 b3 : 1;
    u8 b4 : 1;
    u8 b5 : 1;
    u8 b6 : 1;
    u8 b7 : 1;
    u8 pad[0x29];
    u16 f2a;
    u16 f2c;
} Data;

typedef struct {
    u8 pad00[0x66];
    u16 f66;
} Pad;

typedef struct {
    Pad *(*get)(void *self, s32 arg);
} IfaceVt;

typedef struct {
    IfaceVt *vt;
} Iface;

typedef struct {
    u8 pad00[0x90];
    u8 f90;
    u8 f91;
    u16 f92;
} Menu;

typedef struct {
    u8 pad00[0x28];
    s32 f28;
    u8 pad2c[0x84 - 0x2c];
    s32 f84;
} Ctx;

extern Iface *data_021092f4;
extern Ctx data_02102900;
extern Data data_021044a4;
extern u8 data_021027f8[];
extern u8 data_02106d90[];

extern s32 func_0205d0d0(void *, s32, s32);
extern void func_0205ecc4(void *, s32, s32, s32, s32, s32);
extern void func_0206f200(void *);
extern void func_02072cd0(void);

static inline s32 between(s32 v, u32 lo, s32 hi) {
    return v >= lo && v < hi;
}
static inline s32 in_a(u32 x, u32 y) {
    return between(x, 0x24, 0x64) && y >= 0x70 && (s32) y < 0x90;
}

void func_0206ef0c(Menu *arg0) {
    u32 y, old, x;
    x   = 0;
    y   = 0;
    old = arg0->f90;

    if (data_021092f4->vt->get(data_021092f4, 0)->f66 & 4) {
        if (arg0->f91 == 0 || arg0->f92 != data_021092f4->vt->get(data_021092f4, 0)->f66) {
            arg0->f91 = 1;
            func_02072cd0();
            func_0205ecc4(&data_02102900, 0xf, 0, 0, 0, 0);
            func_0205ecc4(&data_02102900, 0x26, 1, 0, 0, 0);
            data_02102900.f84                      = 1;
            M2C_FIELD(&data_02102900, s32 *, 0x64) = 1;
            return;
        }
    }
    if (arg0->f91 != 0) {
        return;
    }
    arg0->f92 = data_021092f4->vt->get(data_021092f4, 0)->f66;
    if (data_021044a4.b4) {
        x = data_021044a4.f2a & 0xff;
        y = data_021044a4.f2c & 0xff;
    }
    if (func_0205d0d0(data_021027f8, 0x20, 4) != 0 || in_a(x, y)) {
        arg0->f90 = 1;
        func_0206f200(arg0);
    } else if (func_0205d0d0(data_021027f8, 0x10, 4) != 0 || (x >= 0xa4 && (s32) x < 0xe4 && y >= 0x70 && (s32) y < 0x90)) {
        arg0->f90 = 2;
        func_0206f200(arg0);
    }
    if (func_0205d0d0(data_021027f8, 1, 4) == 0) {
        if (arg0->f90 != old) {
            return;
        }
        if (!in_a(x, y)) {
            if (!(x >= 0xa4 && (s32) x < 0xe4 && y >= 0x70 && (s32) y < 0x90)) {
                return;
            }
        }
    }
    if (arg0->f90 != 0) {
        M2C_FIELD(M2C_FIELD(data_02106d90, u8 **, 0xbb8), u8 *, 0xd) = arg0->f90;
        func_0205ecc4(&data_02102900, data_02102900.f28, 1, 0, 0, 0);
    }
}
