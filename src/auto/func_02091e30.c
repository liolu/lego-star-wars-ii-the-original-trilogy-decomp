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
    u8 pad00[0x8];
    u32 flags;
    u16 mode;
    u8 pad0e[0xb0 - 0xe];
    Vec3 pos;
    u8 padbc[0xc8 - 0xbc];
    Rot rot;
    u8 rot_dirty;
    u8 padd1[0x5da - 0xd1];
    u8 f5da;
    u8 pad5db[0x69c - 0x5db];
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
extern void func_020922f4(Holder *self, s32 a, s32 b);
extern void *func_0205e80c(s32 size, s32 a1, s32 a2);
extern void func_020accac(void *mtx, const Vec3 *pos, const Rot *rot, const Vec3 *scale);
extern Entity *func_020029c4(void *mem, s32 a1, s32 selector, s32 a3, void *mtx);

static inline int ang_eq(const Ang *a, Ang b) {
    return a->v == b.v;
}
static inline u16 cp(Ang h) {
    return h.v;
}
static inline void ang_set(Ang *d, u16 v) {
    Ang a;
    a.v = v;
    *d  = a;
}
static inline void ang_put(Ang *d, u16 v) {
    d->v = v;
}
static inline void ang_cpy(Ang *d, Ang a) {
    *d = a;
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
        ang_put(&e->rot.x, cp(r->x));
        ang_put(&e->rot.y, cp(r->y));
        ang_put(&e->rot.z, cp(r->z));
        e->rot_dirty = 1;
    }
}

enum {
    ID_NONE  = 0xfa,   /* empty entry in data_020eebb8 rows and the saved-id lists */
    OBJ_SIZE = 0x6f8,  /* allocation size passed to func_0205e80c for the new object */
    FX_ONE   = 0x1000, /* 1.0 in 20.12 fixed point (unit scale) */
};

/*
 * Replaces the entity in holder slot idx with target. If target->mode is 1,
 * defers to the slot-swap func_020922f4 (other slot = target->f5da).
 * Otherwise: saves the slot entity's f804, position, selector, v6a0, rotation
 * and f6b6, and target's position, rotation, selector and f6b6, plus the ids
 * in each one's items[] that differ from its selector's data_020eebb8 row;
 * recreates slot idx with target's selector at target's position
 * (func_02091b88, replacing the occupant) and sets flag 2 on target; builds a
 * new object (func_020029c4, second arg 2) with the old selector from a matrix of
 * the old position/rotation at unit scale; restores v6a0 on the slot entity,
 * gives it target's rotation and the new object the old rotation (f6b4 =
 * rotation y), keeps each one's own f6b6, re-applies the saved ids crosswise
 * via func_020063d4, restores f804, and clears f7fc on the first
 * data_021027f8.count slots.
 */

void func_02091e30(Holder *self, s32 idx, Entity *target) {
    Vec3 posA;
    Vec3 v6a0A;
    s32 itemsA[5];
    Vec3 posT;
    s32 itemsT[5];
    Vec3 scale;
    Rot rotA;
    Rot rotT;
    s32 mtx[18];
    s32 f6b6A;
    s32 f6b6T;
    s32 a804;
    s32 selA;
    s32 selT;
    Entity *e;
    Entity *obj;
    s32 i;
    u8 j;
    u32 k;
    s32 id;
    s32 slot;
    u16 cur;

    if (target->mode == 1) {
        func_020922f4(self, idx, target->f5da);
        return;
    }
    e      = self->slots[idx];
    a804   = e->f804;
    posA.x = e->pos.x;
    posA.y = e->pos.y;
    posA.z = e->pos.z;
    {
        Entity *slot_ent = self->slots[idx];
        selA             = slot_ent->selector;
        v6a0A.x          = slot_ent->v6a0.x;
        v6a0A.y          = slot_ent->v6a0.y;
        v6a0A.z          = slot_ent->v6a0.z;
        rotA.x           = slot_ent->rot.x;
        rotA.y           = slot_ent->rot.y;
        rotA.z           = slot_ent->rot.z;
        rotA.w           = slot_ent->rot.w;
    }
    f6b6A  = self->slots[idx]->f6b6;
    posT.x = target->pos.x;
    posT.y = target->pos.y;
    posT.z = target->pos.z;
    rotT   = target->rot;
    selT   = target->selector;
    f6b6T  = target->f6b6;

    for (i = 0; i < 5; i++) {
        itemsA[i] = itemsT[i] = ID_NONE;
    }
    for (i = 0; i < 5; i++) {
        id = data_020eebb8[self->slots[idx]->selector][i];
        if (id != ID_NONE) {
            slot = func_0208da74(id);
            cur  = self->slots[idx]->items[(u8) slot];
            if (cur != id) {
                itemsA[slot] = cur;
            }
        }
        id = data_020eebb8[target->selector][i];
        if (id != ID_NONE) {
            slot = func_0208da74(id);
            cur  = target->items[(u8) slot];
            if (cur != id) {
                itemsT[slot] = cur;
            }
        }
    }

    func_02091b88(self, idx, selT, &posT, 1);
    target->flags |= 2;
    obj = func_0205e80c(OBJ_SIZE, 0, 4);
    if (obj != NULL) {
        scale.x = FX_ONE;
        scale.y = FX_ONE;
        scale.z = FX_ONE;
        func_020accac(mtx, &posA, &rotA, &scale);
        obj = func_020029c4(obj, 2, selA, 0, mtx);
    }
    func_0200b938(self->slots[idx]->f69c, 3);
    vec_set(&self->slots[idx]->v6a0, &v6a0A);
    ent_set_rot(self->slots[idx], &rotT);
    ent_set_rot(obj, &rotA);
    ent_set_f6b4(self->slots[idx], rotT.y.v);
    ent_set_f6b4(obj, rotA.y.v);
    self->slots[idx]->f6b6 = f6b6A;
    obj->f6b6              = f6b6T;

    for (j = 0; j < 5; j++) {
        if (itemsT[j] != ID_NONE) {
            func_020063d4(self->slots[idx], itemsT[j], 0);
        }
        if (itemsA[j] != ID_NONE) {
            func_020063d4(obj, itemsA[j], 0);
        }
    }
    func_02004800(self->slots[idx]);
    func_02004800(obj);
    self->slots[idx]->f804 = a804;
    func_02060158(data_02102900, 0, 0x11, 0);
    func_0200ee88(self->slots[idx]);
    for (k = 0; k < data_021027f8.count; k++) {
        self->slots[k]->f7fc = 0;
    }
}
