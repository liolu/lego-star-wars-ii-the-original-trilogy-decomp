#include "m2c.h"

typedef struct {
    u16 v;
} HalfVal;

typedef struct {
    u16 b0 : 1;
    u16 b1 : 1;
    u16 b2 : 1;
    u16 b3 : 1;
    u16 b4 : 1;
    u16 b5 : 1;
    u16 b6 : 1;
    u16 b7 : 1;
    u16 b8 : 1;
} Flags;

struct Link;

struct ListHead {
    Link *first;
};

struct OwnerBase {
    u32 f0;
};

struct Owner : OwnerBase, ListHead {
    u32 f8;
};

struct Link {
    ListHead *head;
    Link *next;

    Owner *owner() {
        return static_cast<Owner *>(head);
    }

    void unlink() {
        Link *cur;
        Link **p;

        p   = *(Link ***) this; // FAKE: re-read head through a cast so it is reloaded, not CSE'd
        cur = *p;
        if (cur != this) {
            do {
                p   = &cur->next;
                cur = cur->next;
            } while (cur != this);
        }
        *p = next;
    }

    void set(Owner *o) {
        ListHead *nh = o;
        if (head != (ListHead *) 0) {
            unlink();
        }
        if (nh != (ListHead *) 0) {
            head      = nh;
            next      = nh->first;
            nh->first = this;
        } else {
            head = (ListHead *) 0;
            next = (Link *) 0;
        }
    }
};

typedef struct {
    u8 pad0[9];
    u8 f9;
} Sub;

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
    virtual void unknown24(u32 anim, s32 a, s32 b, s32 c); // Slot 0x24.

    u8 pad4[0x14a];
    Flags flags14e;
    u8 pad150[0x36];
    Flags flags186;
    u8 pad188[0x20];
    u16 mask1a8;
    u8 pad1aa[0x424];
    Sub sub;
    u8 pad5d8[0xdc];
    HalfVal turn;

    void setTurn(HalfVal a) {
        turn = a;
    }
};

class Obj {
public:
    virtual void unknown00();
    virtual void unknown04();
    virtual void unknown08(); // Slot 0x08.

    Entity *ent;
    u8 pad8[4];
    u8 fc;
    u8 state;
    u8 idx;
    u8 count;
    u8 limit;
    u8 loop;
    u8 f12;
    u8 pad13;
    Link link;
};

typedef struct {
    u32 v;
    u32 pad;
} Cell;

typedef struct {
    Cell c[2];
    u32 pad;
} Row;

extern "C" const Row data_020f6af8[];
extern "C" const Row data_020f6afc[];

extern "C" void func_02006b24(void *object);
extern "C" void func_0201811c(HalfVal *out, Entity *ent, Owner *target, s32 n);
extern "C" s32 func_02018210(Entity *, HalfVal, HalfVal);
extern "C" void func_0204a400(Sub *p, Entity *q, s32 n);
extern "C" s32 func_0204a530(Sub *);
extern "C" void func_0204a544(Sub *, Entity *);
extern "C" s32 func_0204a880(Sub *);

enum {
    OWNER_DETACH_FLAGS = 0x802, // Owner flags that make this object drop its owner link.
    TURN_ARG           = 0xe38, // Constant third argument of func_02018210.
};

enum {
    STATE_INIT   = 0, // Play the start animation, then switch to STATE_ACTIVE.
    STATE_ACTIVE = 1,
};

// Update step, referenced from the pointer table data_020fe2d0. Steps:
// 1. If the linked owner has any OWNER_DETACH_FLAGS bit set, unlink from it.
// 2. STATE_INIT: start the entity animation picked from data_020f6af8 by idx and
//    f12 (virtual slot 0x24), call func_02006b24, take the next idx from
//    data_020f6afc, enter STATE_ACTIVE and clear f12. Other states but
//    STATE_ACTIVE do nothing.
// 3. STATE_ACTIVE: unless func_0204a880(&ent->sub) returns nonzero, store the
//    value func_0201811c computes towards the owner in ent->turn, then pass
//    ent->turn to func_02018210.
// 4. Depending on entity flag bits and sub.f9, call func_0204a400 with mode 1
//    (count == 0) or 2 (idx != 9); then either advance count and call virtual
//    slot 0x08, or set fc bit 0, clear mask1a8 bit 8 and call func_0204a400
//    with mode 3 (and func_0204a544 when func_0204a530 says so).
// 5. If entity flag bit 1 at +0x14e is set, set fc bit 0 and clear mask1a8 bit 8.
extern "C" void func_020454cc(Obj *self) {
    HalfVal res;
    HalfVal step;
    Entity *ent;
    s32 sel;

    if (self->link.owner() != NULL && (self->link.owner()->f8 & OWNER_DETACH_FLAGS)) {
        self->link.set((Owner *) 0);
    }

    switch (self->state) {
        case STATE_INIT:
            sel = 0;
            if (self->f12 != 0) {
                sel = 1;
            }
            self->ent->unknown24(data_020f6af8[self->idx].c[sel].v, 0, 1, 0);
            func_02006b24(self->ent);
            self->idx   = data_020f6afc[self->idx].c[sel].v;
            self->state = STATE_ACTIVE;
            self->f12   = 0;
            return;
        case STATE_ACTIVE:
            break;
        default:
            return;
    }

    if (!func_0204a880(&self->ent->sub) && self->link.owner() != NULL) {
        func_0201811c(&res, self->ent, self->link.owner(), 0);
        self->ent->setTurn(res);
    }
    ent    = self->ent;
    step.v = TURN_ARG;
    func_02018210(ent, ent->turn, step);

    ent = self->ent;
    if (ent->flags186.b8 && ent->sub.f9 != 1) {
        if (self->count == 0) {
            func_0204a400(&ent->sub, ent, 1);
            self->f12 = 1;
        } else if (self->idx != 9) {
            func_0204a400(&ent->sub, ent, 2);
            self->f12 = 1;
        }
    } else if (ent->flags186.b2 && ent->sub.f9 != 1) {
        if (self->count == 0) {
            func_0204a400(&ent->sub, ent, 1);
        } else if (self->idx != 9) {
            func_0204a400(&ent->sub, ent, 2);
        }
    }

    if (self->idx != 9 && self->ent->flags186.b3) {
        if (self->ent->sub.f9 == 1 || self->loop != 0) {
            self->count++;
            if (self->loop != 0 && self->count >= self->limit - 1) {
                self->loop = 0;
            }
            self->unknown08();
        } else {
            self->fc |= 1;
            self->ent->mask1a8 &= ~0x100;
            func_0204a400(&self->ent->sub, self->ent, 3);
            if (func_0204a530(&self->ent->sub)) {
                func_0204a544(&self->ent->sub, self->ent);
            }
        }
    }

    if (self->ent->flags14e.b1 == 1) {
        self->fc |= 1;
        self->ent->mask1a8 &= ~0x100;
    }
}
