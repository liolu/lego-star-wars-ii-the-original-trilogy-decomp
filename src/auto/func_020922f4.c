#include "m2c.h"

typedef struct {
    s32 x, y, z;
} Vec3;

typedef struct {
    u16 v;
} Ang;

typedef struct {
    Ang x, y, z, w;
} Rot;

typedef struct Entity {
    u8 pad00[0xb0];
    Vec3 pos;
    u8 padbc[0xc8 - 0xbc];
    Rot rot;
    u8 rot_dirty;
    u8 padd1[0x69c - 0xd1];
    u8 f69c[4];
    Vec3 v6a0;
    u8 pad6ac[0x6b4 - 0x6ac];
    Ang f6b4;
    s16 f6b6;
    u8 pad6b8[3];
    u8 selector; /* entity_selector passed to func_02091b88 */
    u8 pad6bc[0x6e8 - 0x6bc];
    u16 items[(0x7fc - 0x6e8) / 2];
    s32 f7fc;
    u8 pad800[4];
    s32 f804;
} Entity;

typedef struct Holder {
    u8 pad[0x4c];
    Entity *slots[4];
    u8 pad5c[0x92c - 0x5c];
    u8 active;
} Holder;

typedef struct {
    u8 pad[0x29];
    u8 count;
} Config;

extern Config data_021027f8;
extern u8 data_02102900[];
extern u8 data_020eebb8[][5];

extern s32 func_0208da74(u16 id);
extern void func_020063d4(Entity *e, u16 id, s32 arg2);
extern void func_02004800(Entity *e);
extern void func_0200ee88(Entity *e);
extern void func_02060158(void *a0, s32 a1, s32 a2, s32 a3, ...);
extern void *func_02091b88(Holder *holder, s32 slot_index, s32 entity_selector, Vec3 *source_position, u8 replace_existing);
extern void func_0200b938(void *arg0, s32 arg1);

static inline int ang_eq(const Ang *a, Ang b) {
    return a->v == b.v;
}
static inline void ang_set(Ang *d, u16 v) {
    Ang a;
    a.v = v;
    *d  = a;
}
static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

static inline void ent_set_f6b4(Entity *e, u16 v) {
    ang_set(&e->f6b4, v);
}

static inline void ent_set_rot(Entity *e, const Rot *r) {
    int same = ang_eq(&r->x, e->rot.x) && ang_eq(&r->y, e->rot.y) && ang_eq(&r->z, e->rot.z);
    if (!same) {
        ang_set(&e->rot.x, r->x.v);
        ang_set(&e->rot.y, r->y.v);
        ang_set(&e->rot.z, r->z.v);
        e->rot_dirty = 1;
    }
}

enum {
    ID_NONE = 0xfa, /* empty entry in data_020eebb8 rows and the saved-id lists */
};

/*
 * Swaps the entities in holder slots a and b: saves each entity's position,
 * v6a0, rotation, selector, f6b6 and the ids in its items[] that differ from
 * its selector's data_020eebb8 row, recreates slot a with b's selector and
 * position and slot b with a's (func_02091b88, replacing the occupant), then
 * restores the swapped v6a0/rotation, each slot's own f6b6, re-applies the
 * saved ids crosswise via func_020063d4, restores f804 (slot b takes the
 * active slot's value), and clears f7fc on the first data_021027f8.count slots.
 */
void func_020922f4(Holder *self, s32 a, s32 b) {
    Vec3 posA;
    Vec3 v6a0A;
    s32 itemsA[5];
    Vec3 posB;
    Vec3 v6a0B;
    s32 itemsB[5];
    Rot rotA;
    Rot rotB;
    s32 f6b6A;
    s32 f6b6B;
    s32 a804;
    s32 c804;
    s32 selA;
    s32 selB;
    Entity *e;
    s32 i;
    u8 j;
    u32 k;
    s32 id;
    s32 slot;
    u16 cur;

    e       = self->slots[a];
    a804    = e->f804;
    c804    = self->slots[self->active]->f804;
    posA.x  = e->pos.x;
    posA.y  = e->pos.y;
    posA.z  = e->pos.z;
    v6a0A.x = self->slots[a]->v6a0.x;
    v6a0A.y = self->slots[a]->v6a0.y;
    v6a0A.z = self->slots[a]->v6a0.z;
    rotA    = self->slots[a]->rot;
    selA    = self->slots[a]->selector;
    f6b6A   = self->slots[a]->f6b6;
    e       = self->slots[b];
    posB.x  = e->pos.x;
    posB.y  = e->pos.y;
    posB.z  = e->pos.z;
    v6a0B.x = self->slots[b]->v6a0.x;
    v6a0B.y = self->slots[b]->v6a0.y;
    v6a0B.z = self->slots[b]->v6a0.z;
    rotB    = self->slots[b]->rot;
    selB    = self->slots[b]->selector;
    f6b6B   = self->slots[b]->f6b6;

    for (i = 0; i < 5; i++) {
        itemsA[i] = itemsB[i] = ID_NONE;
    }
    for (i = 0; i < 5; i++) {
        id = data_020eebb8[self->slots[a]->selector][i];
        if (id != ID_NONE) {
            slot = func_0208da74(id);
            cur  = self->slots[a]->items[(u8) slot];
            if (cur != id) {
                itemsA[slot] = cur;
            }
        }
        id = data_020eebb8[self->slots[b]->selector][i];
        if (id != ID_NONE) {
            slot = func_0208da74(id);
            cur  = self->slots[b]->items[(u8) slot];
            if (cur != id) {
                itemsB[slot] = cur;
            }
        }
    }

    func_02091b88(self, a, selB, &posB, 1);
    func_02091b88(self, b, selA, &posA, 1);
    func_0200b938(self->slots[a]->f69c, 3);
    vec_set(&self->slots[a]->v6a0, &v6a0B);
    vec_set(&self->slots[b]->v6a0, &v6a0A);
    ent_set_rot(self->slots[a], &rotB);
    ent_set_rot(self->slots[b], &rotA);
    ent_set_f6b4(self->slots[a], rotB.y.v);
    ent_set_f6b4(self->slots[b], rotA.y.v);
    self->slots[a]->f6b6 = f6b6A;
    self->slots[b]->f6b6 = f6b6B;

    for (j = 0; j < 5; j++) {
        if (itemsB[j] != ID_NONE) {
            func_020063d4(self->slots[a], itemsB[j], 0);
        }
        if (itemsA[j] != ID_NONE) {
            func_020063d4(self->slots[b], itemsA[j], 0);
        }
    }
    func_02004800(self->slots[a]);
    func_02004800(self->slots[b]);
    self->slots[a]->f804 = a804;
    self->slots[b]->f804 = c804;
    func_02060158(data_02102900, 0, 0x11, 0);
    func_0200ee88(self->slots[a]);
    func_0200ee88(self->slots[b]);
    for (k = 0; k < data_021027f8.count; k++) {
        self->slots[k]->f7fc = 0;
    }
}
