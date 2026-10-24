#include "m2c.h"

typedef struct Obj {
    void (**vtbl)(struct Obj *);
} Obj;

typedef struct Alloc {
    void (**vtbl)(struct Alloc *, Obj *);
    u8 pad[0x34];
} Alloc;

typedef struct Slot {
    Obj *obj;
    u8 pad04[0x1C];
    u8 owned;
    u8 pad21[3];
} Slot;

typedef struct Holder {
    Slot slots[25];
    void *p384;
    void *p388;
    void *p38c;
    u8 pad390[0x38];
    u32 count;
    s32 mask;
    u8 pad3d0[0x479 - 0x3d0];
    u8 index;
} Holder;

extern u8 data_021019f4;
extern s32 data_027e02a4;

extern void func_02090228(Holder *);
extern u16 func_02093ffc(u8);
extern void func_0205ab6c(u32);
extern void func_0204c864(void *);
extern void func_0205e84c(void *);
extern void func_02066db8(void *);
extern u16 func_02093f58(u8);
extern void G3X_SetFog(s32, s32, s32, s32);
extern void func_0205d72c(void *);

Holder *func_0208f890(Holder *self) {
    u8 owned;
    u32 i;
    u32 j;
    Slot *s;
    u32 id;
    void *p;

    for (i = 0; i < self->count; i++) {
        Obj *o = self->slots[i].obj;
        if (o != 0) {
            s8 *t = (s8 *) &data_021019f4 + data_027e02a4 * 0x38;
            (*(void (***)(s8 *, Obj *))((s8 *) (t - (s8 *) 0)))[2](t, o);
        }
    }
    self->mask = -1;
    func_02090228(self);
    id = func_02093ffc(self->index);
    j  = 0;
    if (j < self->count) {
        s = self->slots;
        do {
            Obj *o = s->obj;
            owned  = s->owned;
            if (o != 0) {
                o->vtbl[1](o);
            }
            s->obj = 0;
            if (id != 0xffff && owned != 0) {
                func_0205ab6c(id);
            }
            j++;
            s++;
        } while (j < self->count);
    }
    p = self->p388;
    if (p != 0) {
        func_0204c864(p);
        func_0205e84c(p);
    }
    self->p388 = 0;
    p          = self->p384;
    if (p != 0) {
        func_0204c864(p);
        func_0205e84c(p);
    }
    self->p384 = 0;
    p          = self->p38c;
    if (p != 0) {
        func_02066db8(p);
        func_0205e84c(p);
    }
    self->p38c = 0;
    func_0205ab6c(func_02093f58(self->index));
    self->index = 74;
    G3X_SetFog(0, 0, 2, 0);
    func_0205d72c((u8 *) self + 912);
    return self;
}
