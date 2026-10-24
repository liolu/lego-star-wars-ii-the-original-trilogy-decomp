#include "m2c.h"

struct Rect12 {
    s32 a;
    s32 b;
    s32 c;
};

struct Short3 {
    s16 x;
    s16 y;
    s16 z;
};

struct Big {
    u8 data[0x48];
};

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
};

class Owner {
public:
    s32 f0;
    s32 f4;
    s32 f8;
    s32 fc;
    s32 f10;
    Entity *child;
};

extern "C" s32 data_020fdbb0[];
extern "C" void *func_0205e80c(s32, s32, s32);
extern "C" void func_02029398(Rect12 *, s32);
extern "C" void func_020accac(Big *, Rect12 *, Short3 *, Rect12 *);
extern "C" Entity *func_0200ff50(void *, s32, s32, s32, Big *);

extern "C" void func_02028db8(Owner *self) {
    Short3 s;
    Rect12 q;
    Rect12 r;
    Big big;
    s32 p;
    Rect12 *qp;
    Short3 *sp;
    Entity *obj;

    obj = (Entity *) func_0205e80c(444, 3, 4);
    if (obj != NULL) {
        qp  = &q;
        sp  = &s;
        q.a = 0x1000;
        q.b = 0x1000;
        s.x = 0;
        s.y = 0;
        s.z = 0;
        q.c = data_020fdbb0[2];
        p   = self->f8;
        if (p != 0) {
            p -= 4;
        }
        func_02029398(&r, p);
        func_020accac(&big, &r, sp, qp);
        obj = func_0200ff50(obj, 512, 338, 0, &big);
    }
    self->child = obj;
    self->child->action(0, 1, 1, 0);
    self->child->action(138, 1, 1, 0);
}
