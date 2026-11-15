#include "m2c.h"

typedef struct {
    s32 x, y, z;
} Vec3;

class Obj {
public:
    virtual void unknown0();
    virtual void unknown1();
    virtual void unknown2();
    virtual void unknown3();
    virtual void unknown4();
    virtual void unknown5();
    virtual void unknown6();
    virtual void unknown7();
    virtual void unknown8();
    virtual void f9(s32, s32, s32, s32);
    u8 pad4[4];
    u32 flags;
    u8 pad0c[0xa4];
    Vec3 pos;
};

typedef struct {
    u8 pad0[4];
    Obj *obj;
    u8 pad8[5];
    u8 state;
    u8 pad0e[2];
    Vec3 pos;
} Owner;

typedef struct {
    u8 pad0[0x4c];
    Obj *slots[1];
} Mgr;

typedef struct {
    u8 pad0[8];
    u32 f8;
    u8 pad0c[0x38];
    Vec3 pos;
    u8 active;
    u8 pad51[3];
} Sys;

extern "C" {
extern Mgr data_0210930c;
extern Sys *data_02107860;
extern u8 data_021027f8[];
extern void *data_02109358[];

typedef struct {
    s32 v;
} Fx;
// Stores the double as 20.12 fixed point (multiplies by 4096.0, truncates with _dfix).
void func_02007f24(s32 *, f64);
s32 func_0200aef8(Vec3 *, Vec3 *, Fx);
void func_020063d4(void *, s32, s32);
void func_0209bd50(Sys *, s32);
}

static inline void sys_set(Sys *d, const Vec3 *s) {
    if (&d->pos != s) {
        d->pos.x = s->x;
        d->pos.y = s->y;
        d->pos.z = s->z;
    }
}

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

// FAKE: sysp is an unused second parameter (every slot-3 override in the sibling
// vtables takes only the object) reused as the holder of &data_02107860; parameters
// are coloured after the loop counter, which gives i r7 and sysp r8.
extern "C" void func_02033b30(Owner *self, Sys **sysp) {
    Vec3 pos;
    Fx rad;
    s32 b, a;
    s32 *pb;
    u8 i;

    func_02007f24(&a, 12.0);
    func_02007f24(&b, 4.0);
    pb = &b;
    if (pb) { // FAKE: always-true local address test steers the sp+8 load through a register.
        rad.v = *pb + a;
    } else {
        rad.v = a;
    }
    pos  = data_0210930c.slots[0]->pos;
    sysp = &data_02107860; // FAKE: overwrites the dummy parameter (register colouring).
    if ((*sysp)->active != 0) {
        for (i = 0; i < data_021027f8[0x29]; i++) {
            vec_set(&pos, &data_0210930c.slots[i]->pos);
            if (func_0200aef8(&pos, &self->obj->pos, rad) != 0) {
                func_020063d4(data_02109358[i], 0xea, 0);
                self->obj->flags |= 2;
                (*sysp)->f8 = 0;
                func_0209bd50((*sysp), (s8) i);
                return;
            }
        }
    }
    switch (self->state) {
        case 0:
            self->obj->f9(0, 1, 1, 0);
            vec_set(&self->pos, &pos);
            self->state = 1;
            sys_set((*sysp), &self->obj->pos);
            break;
        case 1:
        case 2:
            break;
    }
}
