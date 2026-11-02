#include "m2c.h"

// Virtual of the entity class built by func_02011384 (vtable data_020fcfb8 and the
// per-type vtables data_020fd260..data_020fd468 hold it). Same object layout as
// src/auto/func_02011384.cpp: +0x194 is the per-type word set there (f194), +0x212
// the type id passed to that constructor, +0x214 a state byte and +0x1d0 a list link.

struct Link {
    Link **head;
    Link *next;
};
struct Owner {
    u32 f0;
    Link *head;
};

// Partial interface: unknown virtuals keep their observed vtable slots.
class Obj {
public:
    virtual void u00();
    virtual void u04();
    virtual void u08();
    virtual void u0c();
    virtual void u10();
    virtual void u14();
    virtual void u18();
    virtual void u1c();
    virtual void u20();
    virtual void action(int, int, int, int); // Slot 0x24.
};

// Same list helper as src/auto/func_02011384.cpp and func_020458a0.c: unlinks `l`
// from its current list, then puts it at the head of `owner`'s list, or clears
// the link when `owner` is null. The `void *` intermediate keeps the front end
// from propagating the null head into the second test (it is tested at run time).
static inline void link_set(Link *l, Owner *owner) {
    Link *cur;
    Link **new_head;
    Link **p;
    p        = l->head;
    new_head = (Link **) (owner ? (void *) &owner->head : (void *) 0);
    if (p != 0) {
        cur = *p;
        if (cur != l) {
            do {
                p   = &cur->next;
                cur = cur->next;
            } while (cur != l);
        }
        *p = l->next;
    }
    if (new_head != 0) {
        l->head   = new_head;
        l->next   = *new_head;
        *new_head = l;
    } else {
        l->head = 0;
        l->next = 0;
    }
}
extern "C" void func_020122bc(Obj *obj) {
    switch (M2C_FIELD(obj, u32 *, 0x194)) {
        case 0x800:
            // Type ids 0xe0 and 0xe1 ((u16)(id - 0xe0) <= 1) get action 0x35.
            if ((u16) (M2C_FIELD(obj, u16 *, 0x212) + 0xff20) <= 1) {
                obj->action(0x35, 0, 1, 0);
            } else {
                obj->action(0, 1, 1, 0);
            }
            M2C_FIELD(obj, s8 *, 0x214) = 0x10;
            M2C_FIELD(obj, s8 *, 0x1ad) = 0;
            link_set((Link *) ((u8 *) obj + 0x1d0), 0); // detach from its list
            break;
        case 0x200:
        case 0x400:
        case 0x600:
            M2C_FIELD(obj, s8 *, 0x214) = 0xb;
            M2C_FIELD(obj, s8 *, 0x1ad) = 0;
            link_set((Link *) ((u8 *) obj + 0x1d0), 0); // detach from its list
            return;
    }
}
