#include "m2c.h"

typedef struct ListOwner ListOwner;

typedef struct Link {
    /* 0x0 */ ListOwner *owner;
    /* 0x4 */ struct Link *next;
} Link;

struct ListOwner {
    /* 0x0 */ Link *head;
};

typedef struct Obj {
    /* 0x00 */ u8 unk0;
    /* 0x01 */ u8 unk1;
    /* 0x02 */ u8 unk2;
    /* 0x03 */ u8 unk3;
    /* 0x04 */ u16 unk4;
    /* 0x08 */ ListOwner *owner;
    /* 0x0C */ Link link;
} Obj;

/*
 * Attach link to owner's intrusive list (push at head) or, for a NULL owner,
 * leave it detached with both fields cleared.
 */
static inline void link_set(Link *link, ListOwner *owner) {
    if (owner != 0) {
        link->owner = owner;
        link->next  = owner->head;
        owner->head = link;
    } else {
        link->owner = 0;
        link->next  = 0;
    }
}

/*
 * Initialise an Obj in place and return it. The only caller allocates 0x14
 * bytes and calls this when the allocation succeeded (new Obj(arg1) shape).
 * arg1 is kept in byte 1, byte 3 starts at 3, and the embedded link is
 * attached to the (just cleared, so NULL) owner field, i.e. left detached.
 */
Obj *func_0202ceec(Obj *self, s32 arg1) {
    self->unk0  = 0;
    self->unk1  = arg1;
    self->unk2  = 0;
    self->unk3  = 3;
    self->unk4  = 0;
    self->owner = 0;
    {
        Link *link = &self->link;
        link_set(link, self->owner);
    }
    return self;
}
