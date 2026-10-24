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
    virtual void unknown28();
    virtual void interact(Entity *, int, int); // Slot 0x2c.
};

struct Vec3 {
    s32 x, y, z;
};

struct Owner {
    u8 pad0[0xF4];
    u8 sub[0x2C];
    s32 field120;
    u8 pad124[0x1AD - 0x124];
    u8 field1ad;
};

struct Msg {
    u8 pad0[4];
    Entity *obj;
    u8 pad8[4];
    u8 flags_c;
    u8 padd[3];
    Owner *owner;
    u8 pad14[0x40 - 0x14];
    Vec3 pos;
    u8 field_4c;
};

struct Globals {
    u8 pad[0x2A];
    u8 field_2a;
};

extern "C" Globals data_021027f8;
extern "C" u32 data_027e02a8;
extern "C" u8 data_0210794c;

extern "C" s32 func_020aaeb4(void);
extern "C" void func_020a99bc(s32, s32);
extern "C" void func_020b04f0(void *, s32, void *, s32);
extern "C" void func_020c3c34(void *, s32, s32, s8, s32);
extern "C" void func_0200c848(void *, s32);

static inline int vec_differs(Vec3 *a, Vec3 *b) {
    if (a->x != b->x || a->y != b->y || a->z != b->z) {
        return 1;
    }
    return 0;
}

static inline void vec_set(Vec3 *dst, Vec3 *src) {
    if (dst != src) {
        dst->x = src->x;
        dst->y = src->y;
        dst->z = src->z;
    }
}

extern "C" void func_0204b29c(Msg *arg0) {
    Entity *obj = arg0->obj;
    volatile s16 vb;
    volatile s16 va;
    u8 buf[28];

    M2C_FIELD(obj, u32 *, 8) &= ~0x800000;
    M2C_FIELD(arg0->obj, u32 *, 8) &= ~0x2000000;
    vb = 0;
    va = 0;
    if (M2C_FIELD(obj, u16 *, 0xC8) != 0) {
        M2C_FIELD(obj, u16 *, 0xC8) = 0;
        M2C_FIELD(obj, u8 *, 0xD0)  = 1;
    }
    if (vec_differs((Vec3 *) ((s8 *) obj + 0xB0), &arg0->pos)) {
        vec_set((Vec3 *) ((s8 *) obj + 0xB0), &arg0->pos);
        M2C_FIELD(obj, u8 *, 0xD1) = 1;
    }
    obj->action(0, 1, 1, 0);
    if (M2C_FIELD(obj, u8 *, 0x5DA) == data_021027f8.field_2a) {
        if (data_027e02a8 & 0x40) {
            buf[0] = data_021027f8.field_2a;
            func_020b04f0(&data_0210794c, 0x45, &buf[0], 0xFF);
        } else {
            func_020a99bc(func_020aaeb4(), arg0->field_4c);
        }
    }
    if (arg0->owner != NULL) {
        arg0->owner->field1ad = 0;
    }
    M2C_FIELD(obj, u32 *, 8) &= ~0x2000;
    M2C_FIELD(obj, u8 *, 0x6BA) = 0;
    arg0->owner->field120       = 0;
    func_020c3c34(&arg0->owner->sub, 0, 0, 6, 2);
    u8 f = M2C_FIELD(obj, u8 *, 0x6C4);
    if ((f & 4) || (!(f & 1) && !(f & 2))) {
        func_0200c848(obj, 5);
        func_0200c848(obj, 4);
    }
    arg0->flags_c |= 1;
}
