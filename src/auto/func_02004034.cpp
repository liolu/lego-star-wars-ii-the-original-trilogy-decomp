#include "m2c.h"

typedef struct {
    s32 v;
} UnkVal;

typedef struct {
    s32 x, y, z;
} Vec3;

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
    virtual void unknown24();
    virtual void unknown28();
    virtual void unknown2c();
    virtual void unknown30();
    virtual void unknown34();
    virtual void unknown38();
    virtual void unknown3c();
    virtual void unknown40(s32); // Slot 0x40.
    virtual void unknown44();    // Slot 0x44.

    u8 pad4[4];
    u32 flags;
    u8 padC[0x34];
    u8 b40;
    u8 pad41[0xb0 - 0x41];
    Vec3 pos;
    u8 padBC[0x1a8 - 0xbc];
    u16 h1a8;
    u8 pad1aa[0x5e0 - 0x1aa];
    u8 a5e0[0x69c - 0x5e0];
    u8 a69c[0x6be - 0x69c];
    u8 b6be;
    u8 b6bf;
    u8 b6c0;
    u8 pad6c1[0x6e4 - 0x6c1];
    s32 w6e4;
    u8 pad6e8[0x6ee - 0x6e8];
    u16 h6ee;
    u8 pad6f0[2];
    u8 b6f2;
};

typedef struct {
    u8 pad0[0x20];
    u16 h20;
    u16 h22;
    u16 h24;
} Info;

extern "C" {
extern u8 data_02104d90[];
extern u8 data_021027f8[];

void func_0200b998(void *arg0);
void func_0200b9a8(void *o);
void func_0200c28c(void *arg0, UnkVal arg1, ...);
void func_0200c834(Obj *arg0, s32 arg1);
void func_0200c848(Obj *arg0, s32 arg1);
void func_0202c218(void *obj);
u16 func_0204fa30(void *arg0);
u32 func_02062b10(s32 arg0);
s32 func_020867ec(void *arg0, s32 arg1);
Info *func_0208dad8(s32 arg0);
s32 func_020b8f70(s32 arg0, s32 arg1);
s32 func_020b9304(s32);
}

typedef struct {
    u8 pad0[0x4c];
    Obj *objs[1];
} ObjTable;

extern "C" ObjTable data_0210930c;

enum {
    FX_SHIFT     = 12,        // 20.12 fixed point
    FX_FRAC_MASK = 0xfff, // fraction bits
};

// 20.12 fixed-point scalar.
class Fx {
public:
    s32 v;

    // Sets this to `a` truncated toward zero to a whole number (fraction cleared).
    Fx &truncOf(const Fx &a) {
        if (a.v < 0) {
            v = (a.v + FX_FRAC_MASK) & ~FX_FRAC_MASK;
            return *this;
        }
        v = a.v & ~FX_FRAC_MASK;
        return *this;
    }
};

// Square of the integer part of a whole-number Fx.
static inline s32 fx_sq(const Fx &a) {
    s32 i = a.v >> FX_SHIFT;
    return i * i;
}

static inline s32 fx_sum3(const Fx &a, const Fx &b, const Fx &c) {
    return fx_sq(b) + fx_sq(a) + fx_sq(c);
}

class VecF {
public:
    Fx x, y, z;

    // this = a - b
    VecF &sub(const Vec3 &a, const Vec3 &b) {
        s32 vz = a.z - b.z;
        s32 vy = a.y - b.y;
        s32 vx = a.x - b.x;
        x.v    = vx;
        y.v    = vy;
        z.v    = vz;
        return *this;
    }
};

// Squared distance between the positions of a and b in whole units: each delta
// component is truncated toward zero before squaring.
static inline s32 obj_dist(const Obj *a, const Obj *b) {
    VecF d;
    Fx tz, tx, ty;
    const VecF &rd = d.sub(a->pos, b->pos);
    return fx_sum3(ty.truncOf(rd.y), tx.truncOf(rd.x), tz.truncOf(rd.z));
}

// Per-frame update: refreshes the 0xa/0xb states via func_0200c848/func_0200c834,
// calls virtual slot 0x44, ticks the +0x6f2/+0x6be/+0x6bf byte timers (func_0200b998
// when +0x6be reaches 0), calls virtual slot 0x40 with 0 when func_0204fa30(+0x40)
// returns 5 and flags & 0x802 is clear, and, when h1a8 bit 0x2000 is set, sets flags
// bit 2 if no table object is within FAR_DIST whole units.
extern "C" void func_02004034(Obj *o, UnkVal arg1, ...) {
    Info *info;

    if ((o->flags & 0x20000) == 0x20000) {
        func_0202c218(o->a5e0);
    }
    if (func_020867ec(data_02104d90, 0xb) != 0 || func_020867ec(data_02104d90, 7) != 0 || (o->h1a8 & 0x400)) {
        func_0200c848(o, 0xa);
    } else {
        func_0200c834(o, 0xa);
    }
    if (func_020867ec(data_02104d90, 0xa) != 0 || (o->h1a8 & 0x800)) {
        func_0200c848(o, 0xb);
    } else {
        func_0200c834(o, 0xb);
    }
    o->unknown44();
    o->b6c0 = 0;
    if (o->h6ee != 0xfa && o->b6f2 == 0) {
        info = func_0208dad8(o->h6ee);
        if (info->h20 != 0xc9 && func_020b9304(o->w6e4) == 0 && func_02062b10(info->h22) == 0) {
            o->w6e4 = func_020b8f70(info->h20, 0);
            o->b6f2 = info->h24;
        }
    }
    if (o->b6f2 != 0) {
        o->b6f2--;
    }
    if (o->b6be != 0) {
        if (--o->b6be == 0) {
            func_0200b998(o->a69c);
        }
    }
    if (o->b6bf != 0) {
        o->b6bf--;
    }
    if (o->b40 != 0 && func_0204fa30(&o->b40) == 5 && (o->flags & 0x802) == 0) {
        o->unknown40(0);
    }
    if (o->h1a8 & 0x2000) {
        enum {
            FAR_DIST = 300
        };
        s32 best = 0x7fffffff;
        u8 n     = data_021027f8[0x29];
        u32 i;

        for (i = 0; i < n; i++) {
            Obj *other = data_0210930c.objs[i];
            if (other != (Obj *) 0) {
                s32 dist = obj_dist(other, o);

                if (dist < best) {
                    best = dist;
                }
            }
        }
        if (best > FAR_DIST * FAR_DIST) {
            o->flags |= 2;
        }
    }
    func_0200b9a8(o->a69c);
    func_0200c28c(o, arg1);
}
