#include "m2c.h"

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
    virtual void action(int, int, int, int);
};

typedef struct Rot {
    u16 x;
    u16 y;
    u16 z;
    u16 w;
} Rot;

typedef struct Vec {
    s32 x;
    s32 y;
    s32 z;
} Vec;

struct Ang;
static inline const Ang &ang_sum(const Ang &a, const Ang &b);

typedef struct Ang {
    u16 v;
    Ang() {}
    Ang(u16 value) :
        v(value) {}
    Ang &operator=(Ang a) {
        v = a.v;
        return *this;
    }
    const Ang &operator+(Ang b) const {
        return ang_sum(*this, b);
    }
} Ang;

/* The sum temp is created one inline level below the operator= parameter, which
   gives the target stack-slot order. */
// FAKE: returns a reference to a temporary; only this form gives the target stack-slot order.
static inline const Ang &ang_sum(const Ang &a, const Ang &b) {
    return Ang(a.v + b.v);
}

typedef struct Obj {
    u8 pad0[0xca];
    u16 ang;
    u8 padcc[0x218 - 0xcc];
    Entity *slot[6];
} Obj;

extern "C" u32 data_020fd8e0[];
extern "C" void func_02011384(void *, int);
extern "C" void func_020accac(void *, void *, void *, void *);
extern "C" void *func_02015138(int, int, void *);
extern "C" void func_02001990(void *, int);

static inline void set_ang(Entity *e, Ang a) {
    if (M2C_FIELD(e, u16 *, 0xca) != a.v) {
        M2C_FIELD(e, u16 *, 0xca) = a.v;
        M2C_FIELD(e, u8 *, 0xd0)  = 1;
    }
}

static inline void model_ang0(Entity *e) {
    M2C_FIELD(M2C_FIELD(e, u8 **, 0x20), Ang *, 0x34) = M2C_FIELD(e, Ang *, 0xca);
}

enum {
    SLOT_COUNT = 6,
    ANGLE_STEP = 0x2aaa
}; /* 0x10000 / 6: one sixth of a turn per slot */

extern "C" void *func_02023c80(Obj *obj, int kind) {
    func_02011384(obj, kind);
    M2C_FIELD(obj, u32 *, 0x0) = (u32) data_020fd8e0;
    if (kind != 0xf7) {
        u16 step = Ang(ANGLE_STEP).v;
        Ang ang(obj->ang);
        int i;
        u8 mtxB[0x48];
        u8 mtxA[0x48];
        Vec scale;
        Rot rotA;
        Rot rotB;
        for (i = 0; i < SLOT_COUNT; i++) {
            if (kind == 0xf4) {
                scale.x = 0x1000;
                scale.y = 0x1000;
                scale.z = 0x1000;
                rotA.x  = 0;
                rotA.y  = obj->ang;
                rotA.z  = 0;
                func_020accac(mtxB, (u8 *) obj + 0xb0, &rotA, &scale);
                obj->slot[i] = (Entity *) func_02015138(0xf3, 0, mtxB);
            } else if (kind == 0xf6) {
                scale.x = 0x1000;
                scale.y = 0x1000;
                scale.z = 0x1000;
                rotB.x  = 0;
                rotB.y  = obj->ang;
                rotB.z  = 0;
                func_020accac(mtxA, (u8 *) obj + 0xb0, &rotB, &scale);
                obj->slot[i] = (Entity *) func_02015138(0xf5, 0, mtxA);
            }
            set_ang(obj->slot[i], ang);
            ang = ang + Ang(step);
            model_ang0(obj->slot[i]);
            M2C_FIELD(obj->slot[i], u32 *, 0x8) &= ~0x1000000;
            obj->slot[i]->action(0x8a, 1, 1, 0);
            M2C_FIELD(obj->slot[i], u16 *, 0x210) |= 1;
        }
        M2C_FIELD(obj, u8 *, 0x1bc) = 1;
        M2C_FIELD(obj, u32 *, 0x8) &= ~1;
        func_02001990(obj, 0);
    } else {
        M2C_FIELD(obj, u8 *, 0x1bc) = 0;
    }
    M2C_FIELD(obj, u32 *, 0x8) &= ~0x1000000;
    return obj;
}
