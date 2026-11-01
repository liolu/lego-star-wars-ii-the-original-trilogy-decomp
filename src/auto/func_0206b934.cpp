#include "m2c.h"

class VObj {
public:
    virtual void unknown00(s32, s32); // Slot 0x00.
};

typedef struct {
    s32 x;
    s32 y;
} Vec2;

typedef struct {
    u16 f0 : 1;
    u16 f1 : 1;
    u16 f2 : 1;
    u16 f3 : 1;
    u16 f4 : 1;
    u16 f5 : 1;
    u16 f6 : 1;
    u16 f7 : 1;
    u16 f8 : 1;
    u16 f9 : 1;
    u16 f10 : 1;
} Flags;

typedef struct {
    VObj *obj;
    u8 pad04[0x14];
    u16 f18;
    u16 f1a;
    u16 f1c;
    u8 pad1e[2];
} Slot;

typedef struct {
    Slot slots[2];
    u8 pad40[4];
    s32 pos_x;
    s32 pos_y;
    u8 pad4c[8];
    s32 sub54;
    u8 pad58[8];
    s32 state;
    s32 f64;
    s32 f68;
    s32 f6c;
    s32 f70;
    u8 pad74[0x80C];
    Flags flags;
    u16 f82;
    u16 f84;
    u16 f86;
    u8 pad88[4];
    u16 f8c;
    u16 f8e;
    u16 f90;
} Obj;

extern "C" s32 data_027e02a8;
extern "C" void *data_02107864;
extern "C" M2C_UNK data_02101b78;
extern "C" M2C_UNK data_021027f8;

extern "C" u8 func_0206af20(void *);
extern "C" void func_0206aecc(void *);
extern "C" void func_0206d0a8(void *, s32);
extern "C" u8 func_02053940(void *);
extern "C" void func_02053aac(void *, void *);
extern "C" void func_020536a8(void *, s32);
extern "C" void func_02053d90(void *);
extern "C" void func_0206ce44(void *);
extern "C" void func_0206549c(s32 *);
extern "C" void func_02065600(void *, s32);
extern "C" void func_0206574c(void *, s32, s32);
extern "C" void func_0206bc7c(void *, void *);
extern "C" void func_0206d00c(s32, s32);
extern "C" s32 func_0205d088(void *, u32);
extern "C" void func_020536ec(void *, void *);
extern "C" s32 func_020654a8(s32);

extern "C" void func_0206b934(Obj *self, s32 index, s32 a2, s32 a3, s32 a4, s32 a5) {
    s32 local[12];
    void **pp;
    Vec2 vec;
    Vec2 pos;

    self->f8c              = a2;
    self->f8e              = a3;
    self->f84              = a4;
    self->f90              = a5;
    self->slots[index].f1a = self->slots[index].f1c = self->slots[index].f18 = 0;
    if (data_027e02a8 & 0x30) {
        self->state     = 1;
        self->flags.f10 = (u16) ((index != 0) ? 1 : 0);
        return;
    }
    self->flags.f9 = (data_027e02a8 & 3) != 0;
    pp             = &data_02107864;
    if (func_0206af20(*pp)) {
        func_0206aecc(*pp);
        self->flags.f5 = 1;
    } else {
        self->flags.f5 = 0;
    }
    if (self->slots[index].obj != 0) {
        VObj *o = self->slots[index].obj;
        o->unknown00(1, -1);
    }
    func_0206d0a8(self, index);
    self->flags.f3 = func_02053940(&self->sub54);
    func_02053aac(&pos, &self->sub54);
    self->pos_x = pos.x;
    self->pos_y = pos.y;
    func_020536a8(&self->sub54, 0);
    func_02053d90((char *) &data_02101b78 + self->sub54 * 0x20);
    func_0206ce44(self);
    func_0206549c(local);
    func_02065600(local, 3);
    func_0206574c(local, 0, -1);
    func_0206bc7c(self, local);
    if (self->f84 != 4 && !self->flags.f9) {
        func_0206d00c((s32) self, 1);
    }
    self->state    = 2;
    self->f86      = 3;
    self->f6c      = -1;
    self->flags.f0 = 0;
    self->flags.f4 = 0;
    self->flags.f2 = 0;
    self->flags.f1 = !(func_0205d088(&data_021027f8, 1) || func_0205d088(&data_021027f8, 2));
    self->flags.f6 = 0;
    self->flags.f7 = 0;
    self->flags.f8 = 0;
    if (self->f84 < 3) {
        self->f82 = 0;
    } else {
        self->f82 = 0x5A;
    }
    self->f68 = self->f64 * 4 - 0x60;
    if (a5 == 1) {
        self->f68      = self->f64 * 8 - 0xB4;
        self->flags.f8 = 1;
    }
    {
        s32 x = ((self->f70 >> 1) - 0x80) << 12;
        s32 y = self->f68 << 12;
        vec.x = x;
        vec.y = y;
    }
    func_020536ec(&self->sub54, &vec);
    func_020536a8(&self->sub54, 1);
    func_020654a8((s32) local);
}
