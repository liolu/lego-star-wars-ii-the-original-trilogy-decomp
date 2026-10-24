#include "m2c.h"

struct Vec3 {
    s32 x;
    s32 y;
    s32 z;
};

// Partial interface: unknown virtuals retain their observed vtable slots.
// Declarations only: the original vtable remains in the ROM data.
class Parent {
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
    virtual void setFlag(int, int, int, int); // Slot 0x24.

    u8 pad4[4];
    u32 flags;
    u8 padc[0x38];
    Vec3 posA;
    u8 pad50[0x60];
    Vec3 posB;
};

struct Child {
    u8 pad0[0x24];
    Vec3 pos;
    u8 pad30[0x15];
    u8 dirty;
    u8 pad46[0x356];
    u8 flag;
};

struct Self {
    u8 pad0[4];
    Parent *parent;
    u8 pad8[8];
    Vec3 pos;
    u8 mode;
    u8 pad1d[3];
    Child *child;
    void *handle;
};

extern "C" u8 data_0210930c[];

extern "C" u8 func_020917b4(void *);
extern "C" void *func_020b95bc(s32, Parent *);
extern "C" void *func_0205e80c(s32, s32, s32);
extern "C" void *func_020bbfe8(void *);
extern "C" s32 func_0205ab44(s32);
extern "C" s32 func_0205ab58(s32);
extern "C" void func_020bbeb0(Child *, s32);
extern "C" void func_020bbf78(Child *, s32, s32, s32);

static inline s32 vec_ne(Vec3 *a, Vec3 *b) {
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

static inline void child_setpos(Child *c, Vec3 *p) {
    if (vec_ne(&c->pos, p)) {
        vec_set(&c->pos, p);
        c->dirty = 1;
    }
}

extern "C" void func_02048748(Self *self) {
    Parent *parent;
    Child *child;

    vec_set(&self->pos, &self->parent->posA);
    self->mode = 8;
    parent     = self->parent;
    parent->flags &= ~0x200;
    self->parent->setFlag(7, 1, 1, 0);

    if (func_020917b4(data_0210930c) == 46 || func_020917b4(data_0210930c) == 47 || func_020917b4(data_0210930c) == 48 ||
        func_020917b4(data_0210930c) == 49) {
        self->handle = func_020b95bc(155, self->parent);
        return;
    }
    self->handle = func_020b95bc(154, self->parent);

    if (self->child == NULL) {
        child = (Child *) func_0205e80c(928, 3, 4);
        if (child != NULL) {
            child = (Child *) func_020bbfe8(child);
        }
        self->child = child;
        func_020bbeb0(self->child, func_0205ab44(0x1124));
        func_020bbf78(self->child, 0x7A3, func_0205ab58(0x7A3), 0);
        child_setpos(self->child, &self->parent->posB);
        self->child->flag = 1;
    }
}
