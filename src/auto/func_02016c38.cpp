#include "m2c.h"

typedef struct Vec3 {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

struct Owner;

struct Link {
    Owner *owner;
    Link *next;
};

struct Owner {
    Link *head;
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
    virtual void unknown28();
    virtual void unknown2c();
    virtual void unknown30();
    virtual void unknown34();
    virtual void unknown38();
    virtual void unknown3c();
    virtual void reset(); // Slot 0x40.
};

class Actor : public Entity, public Owner {
public:
    u32 flags;           /* 0x008 */
    u8 pad0C[8];
    u32 f14;             /* 0x014 */
    u8 pad18[0x6c];
    u32 f84;             /* 0x084 */
    u8 pad88[0x28];
    Vec3 pos;            /* 0x0b0 */
    u8 padBC[0xe];
    u16 rotY;            /* 0x0ca */
    u8 padCC[5];
    u8 fd1;              /* 0x0d1 */
    u8 padD2[2];
    u8 anim[0x7a];       /* 0x0d4 */
    u16 f14e;            /* 0x14e */
    u8 pad150[0x44];
    u32 f194;            /* 0x194 */
    u8 pad198[0x14];
    u8 f1ac;             /* 0x1ac */
    u8 f1ad;             /* 0x1ad */
    u8 pad1AE[0x1a];
    Link link;           /* 0x1c8 */
    u8 pad1D0[0x40];
    u16 f210;            /* 0x210 */
    u16 type;            /* 0x212 */
    u8 state;            /* 0x214 */
};

typedef struct Rot {
    s16 x;
    s16 y;
    s16 z;
    s16 pad;
} Rot;

typedef struct Global {
    u8 pad[0xa6];
    u16 v[6];
} Global;

extern "C" void func_02001990(Actor *self, s32 mode);
extern "C" Actor *func_020029c4(void *mem, s32 arg1, s32 arg2, s32 arg3, void *mtx);
extern "C" void func_02003998(void *arg0, s8 arg1);
extern "C" Actor *func_02015138(u32 kind, u32 flags, void *transform);
extern "C" void func_02016664(void *self);
extern "C" void func_0202c498(void *arg0, s8 arg1);
extern "C" void *func_0205e80c(u32 size, u32 arg1, u32 arg2);
extern "C" void func_0209f9d8(void *arg0);
extern "C" void func_020accac(void *arg0, void *arg1, void *arg2, void *arg3);

extern "C" Global data_02107890;

static inline s32 vec_ne(const Vec3 *a, const Vec3 *b) {
    if (a->x != b->x || a->y != b->y || a->z != b->z) {
        return 1;
    }
    return 0;
}

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

// Unlinks l from its current owner's singly linked list (if any), then pushes it
// onto the head of owner's list; a null owner leaves l detached.
static inline void link_set(Link *l, Owner *owner) {
    Link *cur;
    Link **p;

    if (l->owner != 0) {
        p   = &l->owner->head;
        cur = *p;
        if (cur != l) {
            do {
                p   = &cur->next;
                cur = cur->next;
            } while (cur != l);
        }
        *p = l->next;
    }
    if (owner != 0) {
        l->owner    = owner;
        l->next     = owner->head;
        owner->head = l;
    } else {
        l->owner = 0;
        l->next  = 0;
    }
}

enum {
    SCALE_ONE = 0x1000, // 1.0 in 20.12 fixed point
};

// State-entry handler (the default path of func_020263f4): for the new state,
// records it in self->state, calls the slot-0x24 virtual (action) and
// applies per-type side effects. In state 20, types 0x5c/0x67/0xd5 spawn an
// object (func_02015138) at self's position and link it into self's Owner list.
extern "C" void func_02016c38(Actor *self, s32 state) {
    Vec3 scale;
    u16 type;

    switch (state) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
            break;
        case 8:
            if ((u16) (self->type + 0xff8c) >
                1) { // FAKE: type 0x74..0x75 range check as a u16 add (mwcc range fold of <, > did not fire here)
                return;
            }
            self->f84 = 4;
            self->f14 = 0;
            self->action(0x35, 0, 1, 0);
            func_0209f9d8(self->anim);
            self->f14e &= ~1;
            self->state = 8;
            self->f194  = 0x600;
            break;
        case 16:
            if ((u16) (self->type + 0xff20) <= 1) { // FAKE: type 0xe0..0xe1 range check as a u16 add
                self->action(0x35, 0, 1, 0);
            } else {
                self->action(0, 1, 1, 0);
            }
            self->state = 0x10;
            self->f194  = 0x800;
            break;
        case 7:
            self->state = state;
            self->action(0x45, 1, 1, 0);
            self->f194 = 0;
            func_02016664(self);
            break;
        case 15:
            self->state = state;
            self->action(0x58, 0, 1, 0);
            func_0209f9d8(self->anim);
            if (self->type == 0xa8) {
                self->flags &= ~0x20;
            }
            break;
        case 20:
            self->state = state;
            type        = self->type;
            if (type == 0x5c || type == 0x67 || type == 0xd5) {
                Rot rot;
                u8 mtx[0x48];
                Actor *obj;
                const Vec3 *pos;

                self->f194 = 0;
                self->flags &= ~1;
                scale.x = SCALE_ONE;
                scale.y = SCALE_ONE;
                scale.z = SCALE_ONE;
                rot.x   = 0;
                rot.y   = self->rotY;
                rot.z   = 0;
                func_020accac(mtx, &self->pos, &rot, &scale);
                obj = func_02015138(self->type + 1, 0, mtx);
                pos = &self->pos;
                if (vec_ne(&obj->pos, pos)) {
                    vec_set(&obj->pos, pos);
                    obj->fd1 = 1;
                }
                link_set(&obj->link, self);
            } else if (type == 0x181) {
                Actor *obj;

                data_02107890.v[0] = 0x24;
                data_02107890.v[1] = 0x5e;
                data_02107890.v[2] = 0xa3;
                data_02107890.v[3] = 0xb1;
                data_02107890.v[4] = 0xd4;
                data_02107890.v[5] = 0x28;
                obj                = (Actor *) func_0205e80c(0x6f8, 0, 4);
                if (obj != 0) {
                    Rot rot;
                    u8 mtx[0x48];

                    rot.x   = 0;
                    rot.y   = self->rotY;
                    rot.z   = 0;
                    scale.x = SCALE_ONE;
                    scale.y = SCALE_ONE;
                    scale.z = SCALE_ONE;
                    func_020accac(mtx, &self->pos, &rot, &scale);
                    {
                        s32 visible = 1;

                        if (self->flags & 0x80000000) {
                            visible = 0;
                        }
                        obj = func_020029c4(obj, 2, 0x48, visible, mtx);
                    }
                }
                func_0202c498((u8 *) obj + 0x5e0, 0xb);
                obj->action(0x70, 1, 1, 0);
                func_02003998(obj, 10);
                self->flags &= ~1;
            } else {
                self->action(0x51, 0, 1, 0);
                self->action(0x52, 0, 1, 0);
                func_0209f9d8(self->anim);
                self->f194 = 0;
                func_02016664(self);
                type = self->type;
                if (type == 0x4b) {
                    self->f194  = 0x20;
                    self->f1ad  = 0;
                    self->f1ac  = 7;
                    self->state = 0;
                } else if (type == 0x54 || type == 0x110 || type == 0x12a) {
                    self->f194 = 0x100000;
                    self->f1ad = 0;
                    self->f210 &= ~1;
                    self->reset();
                } else if (type == 0x5e) {
                    self->reset();
                    self->f194 = 0;
                    self->f1ad = 0;
                    self->f210 &= ~1;
                }
            }
            break;
        case 26:
        case 27:
            self->state = state;
            type        = self->type;
            if (type == 0x17 || type == 0x108 || type == 0x138) {
                func_02001990(self, 0);
                self->action(0x35, 0, 1, 0);
                self->f14e &= ~1;
                func_0209f9d8(self->anim);
                self->state = 0x1b;
            } else if (type == 0xa4 || type == 0x96) {
                self->f84 = 4;
                func_02001990(self, 1);
                self->action(0x35, 0, 1, 0);
                self->f14e &= ~1;
                func_0209f9d8(self->anim);
                self->state = 0x1b;
            } else if ((u16) (type + 0xff8c) <= 1) { // FAKE: type 0x74..0x75 range check as a u16 add
                self->f84 = 4;
                self->f14 = 0;
                self->action(0x35, 0, 1, 0);
                func_0209f9d8(self->anim);
                self->f14e &= ~1;
                self->state = 8;
                self->f194  = 0x600;
            } else {
                self->flags |= 2;
            }
            break;
    }
}
