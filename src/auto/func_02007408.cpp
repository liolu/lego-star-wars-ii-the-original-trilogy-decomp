#include "m2c.h"

struct HalfVal {
    u16 v;
    HalfVal() {}
    HalfVal(u16 x) :
        v(x) {}
};

// The slot 0x3c result is returned through a hidden pointer in r0.
// The nontrivial copy declaration preserves that evidenced ABI.
typedef struct Ret {
    s32 v;
    Ret(const Ret &);
} Ret;

// func_0204a5bc returns this through a hidden pointer in r0 (its body
// writes *r0); the nontrivial copy declaration keeps that ABI.
struct Delta {
    u16 v;
    Delta(const Delta &);
    u16 operator+(HalfVal o) const {
        return v + o.v;
    }
};

typedef struct {
    s32 x, y, z;
} Vec3;

typedef struct Entity Entity;

struct Entity {
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
    virtual Ret action(u16); // Slot 0x3c; declarations only.
    u8 pad4[4];
    u32 flags;
    u8 pad0c[0xbe];
    HalfVal yaw;
    u8 padcc[4];
    u8 pad_d0[0xb2];
    u16 param182;
    u8 pad184[0x44a];
    u8 sub5ce[0xc];
    u8 mode5da;
    u8 pad5db[0xd9];
    HalfVal turn;
};

enum {
    ENT_FLAG_ACTIVE = 0x200,
    TURN_STEP       = 0xe38,
};

extern "C" s32 data_020ff9a8;
extern "C" const s16 data_020f88c4[];

extern "C" M2C_UNK *func_020aaeb4(void);
extern "C" u8 func_020aae08(s32, s32);
extern "C" void func_020075ec(Entity *);
extern "C" u8 func_0204a590(void *, u16 *);
extern "C" Delta func_0204a5bc(void *);
extern "C" void func_02017ee4(Entity *, Vec3 *, s32);
extern "C" s32 func_02018210(Entity *, HalfVal, HalfVal);

static inline Ret invoke(Entity *entity, HalfVal angle) {
    entity->turn = angle;
    return entity->action(entity->param182);
}

// Rotate v in the x/z plane: x' = x*cos + z*sin, z' = z*cos - x*sin.
static inline void vec_rot_y(Vec3 *v, u16 ang) {
    s32 s;
    s32 c;
    s32 x;
    s32 z;
    s32 nx;
    s32 t;

    s  = data_020f88c4[(ang >> 4) * 2];
    c  = data_020f88c4[(ang >> 4) * 2 + 1];
    x  = v->x;
    z  = v->z;
    t  = (s32) (((s64) z * s + 0x800) >> 12);
    nx = (s32) (((s64) x * c + 0x800) >> 12);
    nx += t;
    t    = (s32) (((s64) x * s + 0x800) >> 12);
    t    = (s32) (((s64) z * c + 0x800) >> 12) - t;
    v->x = nx;
    v->z = t;
}

// Movement step (sibling of func_020075ec, which it delegates to when
// func_020aae08 reports mode 9). func_0204a590 fills a copy of the heading
// at 0xca; with no result and arg1 == 0 nothing happens. On a result, flag
// 0x200 is cleared, heading + func_0204a5bc offset goes to 0x6b4, and the
// slot 0x3c distance (zero sets 0x200 again) is turned into the offset
// {k, k, dist} rotated about Y by that angle and passed with arg2 to
// func_02017ee4. func_02018210 is always called last with the 0x6b4 angle.
extern "C" void func_02007408(Entity *self, s32 arg1, s32 arg2) {
    Vec3 v;
    HalfVal a;
    HalfVal sum;
    u8 ok;

    if (func_020aae08((s32) func_020aaeb4(), self->mode5da) == 9) {
        func_020075ec(self);
        return;
    }
    a.v = self->yaw.v;
    ok  = func_0204a590(self->sub5ce, &a.v);
    if (ok == 0 && arg1 == 0) {
        return;
    }
    if (ok != 0) {
        self->flags &= ~ENT_FLAG_ACTIVE;
        sum.v = func_0204a5bc(self->sub5ce) + a;
        Ret r = invoke(self, sum);
        if (r.v == 0) {
            self->flags |= ENT_FLAG_ACTIVE;
        } else {
            v.x = data_020ff9a8;
            v.y = data_020ff9a8;
            v.z = r.v;
            vec_rot_y(&v, self->turn.v);
            func_02017ee4(self, &v, arg2);
        }
    } else {
        self->flags |= ENT_FLAG_ACTIVE;
    }
    func_02018210(self, self->turn, HalfVal(TURN_STEP));
}
