#include "m2c.h"

typedef struct {
    u16 v;
} Id16;

typedef struct Actor {
    u8 pad0[0xca];
    Id16 id;         /* 0xca */
    u8 pad1[0xd0 - 0xcc];
    u8 dirty;        /* 0xd0 */
    u8 pad2[0x6b4 - 0xd1];
    Id16 id2;        /* 0x6b4 */
    u16 b6;          /* 0x6b6 */
    u8 pad3[0x804 - 0x6b8];
    void *link;      /* 0x804 */
} Actor;

typedef struct Other {
    u8 pad0[0x1c];
    u8 kind;   /* 0x1c */
    u8 pad1d;
    u8 f1e;    /* 0x1e */
    u8 pad1f;
    u8 a;      /* 0x20 */
    u8 b;      /* 0x21 */
} Other;

typedef struct Node {
    Other *obj;
    struct Node *next;
} Node;

typedef struct Pair {
    u8 a, b;
} Pair;

typedef struct Slot {
    u8 pad0[0x24];
    s16 kind;       /* 0x24 */
    u16 ids[5];     /* 0x26 */
    Pair pairs[7];  /* 0x30 */
    u8 b6;          /* 0x3e */
    u8 active;      /* 0x3f */
} Slot;

typedef struct Base {
    u8 pad0[0x21c0];
    Slot slots[1];
} Base;

typedef struct Globals {
    u8 pad0[4];
    Node *list;     /* 0x04 */
    u8 pad8[0x4c - 8];
    u8 *f4c;        /* 0x4c */
    u8 pad50[0x68 - 0x50];
    u8 *f68;        /* 0x68 */
} Globals;

typedef struct Holder Holder;
typedef struct Entity04800 Entity04800;
typedef struct Ent Ent;

extern Actor *data_02109358[4];
extern Globals data_0210930c;
extern s32 data_027e02a8;
extern u8 data_021027f8[];

void *func_020aaeb4(void);
u8 func_020917b4(void *arg0);
u8 func_02093fa8(s32 arg0);
Ent *func_020917c8(Globals *arg0, s32 arg1, s32 arg2);
void *func_02091b88(Holder *holder, s32 slot_index, s32 entity_selector, s32 source_position, u8 replace_existing);
void func_020063d4(Actor *self, u16 id, u8 *arg2);
void func_02004800(Entity04800 *obj);
void func_0200e738(void *arg0, Other *arg1);
void func_020675d8(void *arg0, s32 arg1);

static inline void actor_set_id(s32 idx, Id16 id) {
    Id16 t   = id;
    Actor *a = data_02109358[idx];
    if (a->id.v != t.v) {
        a->id    = t;
        a->dirty = 1;
    }
}

static inline void actor_set_id2(s32 idx, Id16 id) {
    Id16 t                    = id;
    data_02109358[idx]->id2.v = t.v;
}

enum {
    SLOT_KIND_SKIP = 100,  /* slot kind that is not (re)created */
    SLOT_ID_SKIP   = 0xfa,   /* ids[] entry that is not applied */
    SLOT_ID_COUNT  = 5,
    STATUS_FLAG_40 = 0x40,
    SEL_NONE       = -1,
};

/*
 * Refresh the actor data_02109358[idx] from slot idx of base (64-byte slots).
 * Does nothing if the slot is inactive, or, when func_02093fa8 is non-zero and
 * idx != 0, unless bit 0x40 of data_027e02a8 is set.
 * With an actor and a non-skip slot kind: calls func_020917c8 with the kind, then
 * restores the actor's saved 0xca id (setting 0xd0 to 1 on change), copies it to
 * 0x6b4 and stores the slot byte 0x3e at 0x6b6. With no actor: calls func_02091b88.
 * Each non-skip ids[] entry is then passed to func_020063d4, then func_02004800.
 * Finally, if sel is valid and its (a, b) byte pair is set, the first listed object
 * of kind 1..4 with the same pair is stored at actor+0x804; when idx equals
 * data_021027f8[0x2a] it is also passed to func_0200e738 and func_020675d8.
 */
void func_0208a31c(Base *base, s32 idx, s32 sel) {
    Actor *actor;
    Node *node;
    u8 *pa;
    u8 *pb;
    u8 i;

    actor = data_02109358[idx];
    if (base->slots[idx].active == 0) {
        return;
    }
    if (func_02093fa8(func_020917b4(&data_0210930c)) != 0 && idx != 0) {
        if ((data_027e02a8 & STATUS_FLAG_40) == 0) {
            return;
        }
    }
    if (actor != NULL && base->slots[idx].kind != SLOT_KIND_SKIP) {
        Id16 id = actor->id;
        func_020917c8(&data_0210930c, idx, base->slots[idx].kind);
        actor_set_id(idx, id);
        actor_set_id2(idx, id);
        data_02109358[idx]->b6 = base->slots[idx].b6;
    } else if (base->slots[idx].kind != SLOT_KIND_SKIP) {
        func_02091b88((Holder *) &data_0210930c, idx, base->slots[idx].kind, (s32) (data_0210930c.f4c + 0xb0), 0);
    }
    actor = data_02109358[idx];
    if (actor == NULL) {
        return;
    }
    for (i = 0; i < SLOT_ID_COUNT; i++) {
        if (base->slots[idx].ids[i] != SLOT_ID_SKIP) {
            func_020063d4(actor, base->slots[idx].ids[i], NULL);
        }
    }
    func_02004800((Entity04800 *) actor);
    if (((u8 *) func_020aaeb4())[0x4a] == 0) {
        return;
    }
    if (((u8 *) func_020aaeb4())[0x25c] != 0) {
        return;
    }
    if (sel == SEL_NONE) {
        return;
    }
    if (base->slots[idx].pairs[sel].a == 0 && base->slots[idx].pairs[sel].b == 0xff) {
        return;
    }
    node = data_0210930c.list;
    if (node == NULL) {
        return;
    }
    pa = (u8 *) base + idx * 64 + sel * 2 + 0x21f0;
    pb = (u8 *) base + idx * 64 + sel * 2 + 0x21f1;
    do {
        Other *obj = node->obj;
        if ((s32) obj->kind >= 1 && (s32) obj->kind <= 4 && obj->a == *pa && obj->b == *pb) {
            actor->link = obj;
            if (idx != data_021027f8[0x2a]) {
                return;
            }
            func_0200e738(actor, obj);
            func_020675d8(*(u8 **) (*(u8 **) (data_0210930c.f68 + 0x38c) + 4) + 0x28, obj->f1e);
            return;
        }
        node = node->next;
    } while (node != NULL);
}
