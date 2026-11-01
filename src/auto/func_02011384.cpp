#include "m2c.h"

// Base constructor of the entity class whose vtable is data_020fcfb8: the derived
// constructor func_0201e754 (src/auto/func_0201e754.cpp) calls it first and then
// installs its own vtable. `type` indexes the 16-byte per-type tables at
// data_020f03ac..data_020f03b4; most of the body applies per-type overrides of
// the state, callbacks and flags set up by the common prologue.

// Partial interface: unknown virtuals retain their observed vtable slots.
// Declarations only: the original vtable remains in the ROM data.
class Entity;

typedef void (*EntityCallback)(Entity *);

typedef struct Link {
    struct Link **head;
    struct Link *next;
} Link;

typedef struct Owner {
    u32 f0;
    Link *head;
} Owner;

typedef struct {
    u8 pad[4];
    u16 frames;
} AnimDef;

typedef struct {
    u8 pad[8];
    AnimDef *def;
} Anim;

typedef struct {
    u8 a;
    u8 b;
} Pair;

typedef struct {
    s32 v;
} UnkVal;

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

    u8 pad04[4];
    u32 flags;
    u8 pad0c[4];
    EntityCallback cb10;
    EntityCallback cb14;
    EntityCallback cb18;
    u8 pad1c[0x68];
    u32 state;
    u8 pad88[0x4c];
    u8 animctl[0x74];
    Anim *anim;
    u8 pad14c[2];
    u16 h14e;
    u8 pad150[0x36];
    u16 h186;
    u8 pad188[0xc];
    u32 f194;
    u8 pad198[0x12];
    u16 h1aa;
    u8 b1ac;
    u8 pad1ad[0xf];
    u8 b1bc;
    u8 pad1bd;
    u16 h1be;
    u8 b1c0;
    u8 b1c1;
    u8 pad1c2[2];
    u32 f1c4;
    u32 f1c8;
    u32 f1cc;
    Link link;
    u8 b1d8;
    u8 b1d9;
    u8 b1da;
    u8 b1db;
    u8 b1dc;
    u8 pad1dd;
    Pair slots[25];
    u16 h210;
    u16 type_id;
    u8 b214;
    u8 b215;
};

// One column of the 16-byte per-type rows (the other columns are not read here).
typedef struct {
    u16 v;
    u8 pad[14];
} ColU16;

typedef struct {
    u8 v;
    u8 pad[15];
} ColU8;

typedef struct {
    u32 v;
    u8 pad[12];
} ColU32;

extern "C" void *func_0200fe94(Entity *, int, int, int, int);
extern "C" void func_020101cc(Entity *, s32);
extern "C" void func_02001990(Entity *, s32);
extern "C" s32 func_02062a7c(s32, s32);
extern "C" void func_0209f948(void *, UnkVal);
extern "C" void func_0209f9d8(void *);
extern "C" void func_020140f8(Entity *);
extern "C" void func_02016090(Entity *);
extern "C" void func_020160f4(Entity *);
extern "C" void func_02016154(Entity *);
extern "C" void func_020161a8(Entity *);
extern "C" void func_0201621c(Entity *);

extern "C" M2C_UNK data_020fcfb8;
extern "C" const ColU16 data_020f03ac[];
extern "C" const ColU8 data_020f03ae[];
extern "C" const ColU8 data_020f03af[];
extern "C" const ColU16 data_020f03b0[];
extern "C" const ColU32 data_020f03b4[];

// Same list helper as src/auto/func_020458a0.c (C): moves `l` to `owner`'s list,
// or just detaches it when `owner` is null. The `void *` intermediate stands in
// for that file's NULL and keeps the C++ front end from propagating the null
// head into the second test (the original tests it at run time).
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

static inline Anim *entity_anim(Entity *self) {
    return self->anim != 0 ? self->anim : 0;
}

static inline s32 anim_frame_count(Entity *self) {
    Anim *anim = self->anim;
    return (anim != 0 ? (s32) anim->def->frames << 12 : 0) >> 12;
}

extern "C" Entity *func_02011384(Entity *self, u32 type, u32 attr, int arg3) {
    Pair *p;
    s32 last;
    UnkVal u;
    u8 b;
    u16 h;
    u32 info;

    func_0200fe94(self, 4, 0x276, attr, arg3);
    M2C_FIELD(self, u32 *, 0x0) = (u32) &data_020fcfb8; // vtable
    self->f1c8                  = 0;
    self->f1cc                  = 0;
    self->link.head             = 0;
    self->link.next             = 0;
    p                           = self->slots;
    do {
        p->a = 0xff;
        p->b = 0;
        p++;
    } while (p < self->slots + 25);
    func_020101cc(self, data_020f03ac[type].v);
    link_set(&self->link, 0);
    self->f1c4    = 0;
    self->h1be    = 0;
    self->b1bc    = 0;
    self->b1d9    = 0;
    self->b1c1    = 0;
    self->b1c0    = 0;
    self->b1dc    = 0;
    self->b1d8    = 0;
    self->b1da    = 0;
    self->b1db    = 0;
    info          = data_020f03b4[type].v;
    self->f194    = info;
    self->type_id = type;
    self->b215    = 0x56;
    self->b214    = 0;
    self->h210    = 0;
    self->state   = 4;
    self->flags |= 0x1000000;
    func_02001990(self, 1);
    self->h186 &= ~2;
    self->action(0, 1, 1, 0);
    if (self->type_id == 0xad || self->type_id == 0xb9 || self->type_id == 0x142 || self->type_id == 0x133 ||
        self->type_id == 0x134 || self->type_id == 0x165 || self->type_id == 0xfc || self->type_id == 0x41 ||
        self->type_id == 0x178 || self->type_id == 0xe9 || self->type_id == 0x172 || self->type_id == 0xee ||
        self->type_id == 0xf7) {
        self->action(0x8a, 1, 1, 0);
    }
    if (self->type_id == 0x45 || self->type_id == 0x165) {
        self->h14e &= ~4;
    }
    // Entities with more than three animation frames start on a frame chosen
    // by func_02062a7c(0, last) (Q12 frame index into the animation control block).
    if (entity_anim(self) != 0) {
        last = anim_frame_count(self) - 1;
        if (last > 2) {
            u.v = func_02062a7c(0, last) << 12;
            func_0209f948(self->animctl, u);
        }
    }
    if (self->type_id == 0x5d || self->type_id == 0x68 || self->type_id == 0xd6) {
        self->action(0, 0, 1, 0);
        func_0209f9d8(self->animctl);
    }
    b = data_020f03ae[type].v;
    if (b != 0) {
        self->b1c1 = b;
        if (type != 1) {
            self->h210 |= 1;
        }
        self->cb14  = func_020160f4;
        self->state = 0x10;
        if (self->type_id == 0xd6) {
            self->state = 0xa;
        }
        if (self->type_id == 0x96) {
            self->state = 3;
        }
    } else if (info & 0x20000) {
        self->state = 8;
        self->cb18  = func_02016090;
    }
    if (type - 0x13c <= 1) {
        self->h210 |= 1;
    }
    b = data_020f03af[type].v;
    if (b != 0) {
        self->b1ac = b;
    }
    h = data_020f03b0[type].v;
    if (h != 0) {
        self->h1aa = h;
    }
    if (type == 0x56 || type == 0x7d || type == 0x12f) {
        self->state = 0x14;
    }
    if (type == 0xa9 || type == 0x8d) {
        self->state = 0x15;
    }
    if (type == 0x7d || type == 0x12f) {
        self->flags &= ~1;
    }
    if (type == 0x7b) {
        self->h210 |= 1;
        self->cb10  = func_02016154;
        self->state = 0x17;
    }
    if (type == 0x173 || type == 0xf1 || self->type_id == 0x112) {
        self->h210 |= 1;
        self->cb18  = func_020161a8;
        self->state = 0x1a;
    }
    if (type == 0x9b) {
        self->h210 |= 1;
    }
    if (type == 0xe) {
        self->flags |= 0x20;
    }
    if (type == 0x197 || type == 0x198 || type == 0x19d) {
        self->state = 0xd;
        self->flags &= ~0x221;
        self->flags |= 0x1000000;
        func_02001990(self, 1);
        self->cb10 = func_0201621c;
    }
    if (type == 0x175 || (type >= 0x18c && type <= 0x196)) {
        self->flags &= ~1;
        func_020140f8(self);
    }
    if (type == 0x1b3) {
        func_020140f8(self);
        self->h210 |= 1;
    }
    if (self->type_id == 0x133) {
        self->h1be = 5;
    }
    if (self->type_id == 0xf3) {
        self->h1be = 0x258;
    }
    if (type == 0xd9 || type == 0xaa || type == 0x9c || type == 0x9b || type == 0x1a || type == 0x1c || type == 0x61 ||
        type == 0x6f || type == 0x8c || type == 0x1d || type == 0x8f || type == 0x11d) {
        self->flags |= 0x4000000;
    }
    if (self->type_id == 0x6f) {
        self->flags &= ~0x1000000;
    }
    if (self->type_id == 0xa6) {
        self->state = 0x1c;
    }
    return self;
}
