/* Reconstructed from USA ARM9 at 0x0206a1f4 (1844 bytes), C++.
 *
 * Update callback reached through the pointer table data_020fe7fc. It runs
 * the state machine kept in the halfword at +0x84 (states 1, 2, 3, 5, 6, 7;
 * state 7 ends in 9) and then bumps the call counter at +0x14, calling
 * func_020b90b0 once when it reaches 0x21b and func_020b9204 plus
 * func_020b90b0 (last two arguments swapped) when it reaches 0x220.
 *
 * - state 3 polls the value behind model->anim (+0x148 of the object at +0x2c)
 *   against fixed checkpoints, selected by the halfword at +0x86; once the
 *   model pointer is null it sets up the entries at +0x40/+0x50 and goes to 5.
 * - state 5 adds FX_Div(data_020ff9a4, 0xa000) to the value at +0x30 each call
 *   and hands it to func_02053888 until the value from func_02053af8 reaches
 *   data_020ff9a0, where it is clamped and the state moves to 6.
 * - state 7 offsets the positions of the entries at +0x40 and +0x48 by
 *   (data_020ff9a8, 0x8000) each call; once the +0x40 y value reaches 0xa0000
 *   it is pinned there, the object at +0x28 is released through its second
 *   virtual slot (vtable +4) and cleared, and data_02102900 is reconfigured.
 *
 * C++ evidence: `ldr r1, [r0]; ldr r1, [r1, #4]; blx r1` with this in r0 is
 * a virtual call; the unfolded `add rX, sp, #n; ldr rY, [rX, #k]` reads come
 * from inline members called on by-value Fx/Vec2 temporaries.
 */
#include "m2c.h"

struct HalfWord {
    u16 v;
};

class Fx {
public:
    s32 v;
    Fx(s32 x) :
        v(x) {}
    bool operator==(const Fx &o) const {
        return v == o.v;
    }
    bool operator<(const Fx &o) const {
        return v < o.v;
    }
    bool operator<(s32 o) const {
        return v < o;
    }
};

class Vec2 {
public:
    Fx x;
    Fx y;
    Vec2(s32 ax, s32 ay) :
        x(ax),
        y(ay) {}
    Vec2 offset(s32 dx, s32 dy) const {
        return Vec2(x.v + dx, y.v + dy);
    }
};

struct Quad {
    u8 a;
    u8 b;
    u8 c;
    u8 d;
    Quad(u8 aa, u8 ab, u8 ac, u8 ad) :
        a(aa),
        b(ab),
        c(ac),
        d(ad) {}
};

struct ShortPair {
    s16 a;
    s16 b;
    ShortPair(s16 aa, s16 ab) :
        a(aa),
        b(ab) {}
};

class Actor {
public:
    virtual void unknown00();
    virtual void release(); // Slot 0x04.
};

struct Model {
    u8 pad0[0x148];
    Fx *anim;
    u8 pad14c[2];
    u16 bit0 : 1;
    u16 bit1 : 1;
};

struct Obj {
    u8 pad0[0x14];
    s32 timer;
    u8 pad18[0x10];
    Actor *actor;
    Model *model;
    Fx f30;
    u8 pad34[4];
    s32 f38[2];
    s32 f40[2];
    s32 f48[2];
    u8 f50[0x30];
    u8 f80;
    u8 f81;
    u8 pad82[2];
    u16 state;
    u16 mode;
    void *f88;
    void *f8c;
};

struct Flags8 {
    u8 b0 : 1;
    u8 b1 : 1;
    u8 b2 : 1;
    u8 b3 : 1;
    u8 b4 : 1;
};

extern "C" {
void func_020534f4(void *, s32, s32, HalfWord);
void func_0205365c(s32 *, s32);
void func_020536a8(void *, s32);
void func_020536ec(void *, const Vec2 &);
void func_02053888(const s32 *, u32);
Vec2 func_02053aac(void *);
Fx func_02053af8(void *);
s32 func_02053b14(s32 *);
Vec2 func_02054c80(void *, s32, s32, s32, s32, s32, s32, const ShortPair &, Quad);
s32 func_0205d0d0(void *, s32, s32);
void func_0205ecc4(void *, s32, s32, s32, s32, s32);
void func_02063a90(s32, s32);
void func_02063ae0(s32, s32);
void func_02065600(void *, s32);
void func_02065658(void *, void *);
void func_0206574c(void *, s32, s32);
s32 func_02065a78(void *, s32, s32, s32, s32, void *, s32);
void func_020660c0(void *, s32, s32, s32, u16);
void *func_02066708(s32);
void func_0206a96c(Obj *);
s32 func_0206aad4(Obj *);
void func_0206ab54(Obj *);
void func_0206d6cc(void *);
void func_0206d7fc(void *, s8);
void func_020b90b0(s32, s32, s32, s32);
void func_020b90e4(u32, u8);
void func_020b9204(void);
s32 FX_Div(s32, s32);
void G3X_SetClearColor(s32, s32, s16, s32, s32);
void MI_DmaFill16(u32, void *, u16, u32);

/* 8-byte .rodata table of halfwords (src/data/main_rodata_020ede9c.c);
 * the array form lets the element load be scheduled before the state store. */
extern const HalfWord data_020ede9c[];
extern Fx data_020ff9a0;
extern s32 data_020ff9a4;
extern s32 data_020ff9a8;
extern u8 data_021027f8[];
extern u8 data_02102900[];
extern u8 data_02104364[];
extern Flags8 data_021044a4;
}

static inline Fx modelAnim(Model *m) {
    if (m->anim) {
        return *m->anim;
    }
    return Fx(0);
}

extern "C" void func_0206a1f4(Obj *o) {
    switch (o->state) {
        case 1:
            if (func_0206aad4(o) == 0) {
                break;
            }
            func_0205365c(o->f38, 0);
            o->state = 2;
            o->f81   = 0;
            func_02063a90(1, 0);
            break;
        case 2:
            if (o->f80 == 0) {
                func_02065a78(o->f50, 1, 0x80, 0x10, 0xf8, func_02066708(0), 1);
                o->f80 = 1;
            } else if (*M2C_FIELD(o->actor, s32 **, 0x160) == 0x1d000) {
                func_0206a96c(o);
                o->state = 3;
            }
            break;
        case 3:
            if (o->model != 0) {
                if (o->mode == 3 && modelAnim(o->model) == Fx(0x6a000)) {
                    G3X_SetClearColor(0, 0, 0x7fff, 0x3f, 0);
                    M2C_FIELD(o->actor, u32 *, 8) &= ~1;
                    func_0206d6cc(o->f88);
                    func_0206d7fc(o->f88, 1);
                    func_02063ae0(1, 0);
                } else if (o->mode == 3 && modelAnim(o->model) == Fx(0x7a000)) {
                    func_0206ab54(o);
                }
                if (o->mode == 2 && modelAnim(o->model) == Fx(0x78000)) {
                    func_02063ae0(1, 0);
                } else if (o->mode == 2 && modelAnim(o->model) == Fx(0x8c000)) {
                    MI_DmaFill16(3, (void *) func_02053b14(o->f38), 0, 0x800);
                    func_02065a78(o->f50, 1, 0x80, 0x59, 0xf8, func_02066708(1), 1);
                    func_02063a90(1, 0);
                }
                if (o->model->bit1 == 1) {
                    func_0206a96c(o);
                }
            }
            if (o->model == 0) {
                o->state = 5;
                func_020534f4(o->f40, 0, 1, data_020ede9c[2]);
                func_0205365c(o->f40, 0);
                func_02054c80(o->f40, 0x1399, 0, 1, 0x20, 0x20, 0xe0, ShortPair(0, 0x18), Quad(0, 0, 0, 0));
                func_02065658(o->f50, o->f40);
                func_02065600(o->f50, 4);
                func_0206574c(o->f50, 0, -1);
                func_02065a78(o->f50, 0x2d0, 0x80, 0x5a, 0xf0, func_02066708(0x2aa), 1);
                func_02065658(o->f50, o->f38);
                func_020536a8(o->f48, 1);
                func_020b90e4(0, 0xff);
            }
            break;
        case 5:
            o->f30.v += FX_Div(data_020ff9a4, 0xa000);
            if (func_02053af8(o->f48) < data_020ff9a0) {
                func_02053888(o->f48, o->f30.v);
            } else {
                o->f30 = data_020ff9a0;
                func_02053888(o->f48, o->f30.v);
                func_02065600(o->f50, 4);
                func_0206574c(o->f50, 6, -2);
                func_02065a78(o->f50, 0x2bc, 0x80, 0x8c, 0xf0, func_02066708(8), 1);
                o->state = 6;
            }
            break;
        case 6:
            if (!data_021044a4.b4 && func_0205d0d0(data_021027f8, 0x2f0f, 4) == 0) {
                break;
            }
            func_020660c0(o->f50, 0, 0x11, 0xc0, 0);
            o->state = 7;
            break;
        case 7: {
            func_020536ec(o->f40, func_02053aac(o->f40).offset(data_020ff9a8, 0x8000));
            func_020536ec(o->f48, func_02053aac(o->f48).offset(data_020ff9a8, 0x8000));
            if (func_02053aac(o->f40).y < 0xa0000) {
                break;
            }
            func_020536ec(o->f40, Vec2(0, 0xa0000));
            o->state = 9;
            if (o->actor != 0) {
                o->actor->release();
            }
            o->actor = 0;
            func_0205ecc4(data_02102900, 0x11, 0, 4, 0, 0);
            func_0205ecc4(data_02102900, 0xf, 1, 0, 0, 0);
            *(void **) (data_02102900 + 0x50) = o->f88;
            *(void **) (data_02102900 + 0x54) = o->f8c;
            *(s32 *) (data_02102900 + 0x64)   = 1;
            *(s32 *) (data_02102900 + 0x84)   = 1;
            break;
        }
    }

    o->timer++;
    if (o->timer == 0x21b) {
        func_020b90b0(0, data_02104364[10], 0, 0x14);
    } else if (o->timer == 0x220) {
        func_020b9204();
        func_020b90b0(0, 0, data_02104364[10], 0x14);
    }
}
