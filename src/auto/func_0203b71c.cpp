#include "m2c.h"

typedef struct Vec3 {
    s32 x, y, z;
} Vec3;

typedef struct Flags14E {
    u16 b0 : 1;
    u16 b1 : 1;
    u16 rest : 14;
} Flags14E;

// Partial interface: unknown virtuals keep their observed vtable slots.
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

    u8 pad04[4];
    u32 flags8;
    u8 pad0C[0x60 - 0xC];
    Vec3 pos60;
    u8 pad6C[0x14E - 0x6C];
    Flags14E f14E;
    u8 pad150[0x1A8 - 0x150];
    u16 h1A8;
    u8 pad1AA[0x5E0 - 0x1AA];
    u8 sub5E0[4];
};

typedef struct Self {
    void **vtbl;
    Obj *obj;
    u8 pad08[0x14 - 0x8];
    s32 v14;
    s32 v18;
    u8 pad1C[0x20 - 0x1C];
    u8 b20;
    u8 pad21[0x30 - 0x21];
    s16 h30;
    u8 pad32[0x34 - 0x32];
    Vec3 v34;
    u8 pad40[0x4C - 0x40];
    s32 v4C;
    s32 state;
    s32 v54;
} Self;

extern "C" {
extern M2C_UNK data_0210930c;
extern Vec3 data_020ff9b4;

void func_02005854(Obj *, s32, s32, s32, s32);
void func_02018200(Obj *, s32, s32);
s32 func_0202cb44(void *);
void func_0203bad0(Self *);
s32 func_0203bc38(Self *);
void func_0203bca0(Self *, s32);
u32 func_02062ae4(s32);
void func_020906a8(Vec3 *, s32, s32, s32);
}

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

// Virtual method (reached only through the vtable at data_020fe18c) that
// advances a six-step state machine kept in self->state (0x50). States 0, 3
// and 5 call obj's slot-0x24 virtual with 0x68 and then go back to state 1;
// state 1 waits for func_0203bc38, resets obj->pos60 from data_020ff9b4 and
// picks state 4 if a scaled 0..99 value from func_02062ae4 exceeds 50, else
// state 2; states 2 and 4 call the slot with 0x6A
// and move on to 3 or 5, which wait for bit 1 of obj's halfword at 0x14E.
extern "C" void func_0203b71c(Self *self) {
    Vec3 tmp;
    s32 r;

    switch (self->state) {
        case 0:
            self->v54 = func_0202cb44(self->obj->sub5E0);
            r         = self->v14;
            self->v18 = r;
            func_020906a8(&tmp, M2C_FIELD(&data_0210930c, s32 *, 0x68), r, self->b20);
            vec_set(&self->v34, &tmp);
            self->obj->unknown24(0x68, 1, 1, 0);
            self->v4C = 0x68;
            func_0203bca0(self, 0x68);
            self->state = 1;
            self->h30   = 5;
            break;
        case 1:
            if (func_0203bc38(self) == 0) {
                return;
            }
            vec_set(&self->obj->pos60, &data_020ff9b4);
            if (func_02062ae4(99) > 50) {
                self->state = 4;
            } else {
                self->state = 2;
            }
            break;
        case 2:
            self->v54 = func_0202cb44(self->obj->sub5E0);
            if (self->v54 != 0) {
                func_02018200(self->obj, self->v54, 0);
            }
            self->obj->h1A8 |= 2;
            self->obj->unknown24(0x6A, 0, 1, 0);
            self->state = 3;
            break;
        case 3:
            if (self->obj->f14E.b1 != 1) {
                return;
            }
            r         = func_0202cb44(self->obj->sub5E0);
            self->v54 = r;
            if (r != 0) {
                func_02005854(self->obj, 0xC, 0, 5, r);
            }
            func_0203bad0(self);
            self->obj->unknown24(0x68, 1, 1, 0);
            self->obj->flags8 &= ~0x200;
            self->v4C = 0x68;
            func_0203bca0(self, 0x68);
            self->state = 1;
            self->h30   = 5;
            break;
        case 4:
            self->obj->flags8 |= 0x200;
            self->v54 = func_0202cb44(self->obj->sub5E0);
            if (self->v54 != 0) {
                func_02018200(self->obj, self->v54, 0);
            }
            self->obj->h1A8 |= 2;
            self->obj->unknown24(0x6A, 0, 1, 0);
            self->state = 5;
            break;
        case 5:
            if (self->obj->f14E.b1 != 1) {
                return;
            }
            r         = func_0202cb44(self->obj->sub5E0);
            self->v54 = r;
            if (r != 0) {
                func_02005854(self->obj, 0xA, 0, 5, r);
            }
            func_0203bad0(self);
            self->obj->unknown24(0x68, 1, 1, 0);
            self->obj->flags8 &= ~0x200;
            self->v4C = 0x68;
            func_0203bca0(self, 0x68);
            self->state = 1;
            self->h30   = 5;
            break;
    }
}
