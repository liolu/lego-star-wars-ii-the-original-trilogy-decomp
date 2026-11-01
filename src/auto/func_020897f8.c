/* func_020897f8: walks the four entity slots of data_0210930c (+0x4c). For each non-NULL slot it
 * copies the slot entity's position (+0xb0), then looks at the matching 64-byte source record of the
 * argument (array at +0x21c0):
 * - mode (+0x24) != MODE_SENTINEL and flag (+0x3f) set: gets an entity from func_02091b88(globals,
 *   slot, mode, record, 1); if the slot is data_021027f8's current index (+0x2a) the entity is passed
 *   to func_020a0370(func_020aaeb4(), entity). The copied position is written to that entity (+0xb0,
 *   dirty flag +0xd1 on change) and the record's three u16 angles (+0x1c) are written to the entity
 *   (+0xc8, dirty flag +0xd0) when any of them differs.
 * - mode != MODE_SENTINEL and flag clear: the position copy is written back to the slot entity.
 * - otherwise: func_02091b68(globals, slot).
 * The walk is bracketed by func_02001688(0) and func_020016c0().
 */
#include "m2c.h"

enum {
    SLOT_COUNT    = 4,
    MODE_SENTINEL = 100,
};

typedef struct {
    u16 v;
} HalfVal;

typedef struct {
    s32 x, y, z;
} Vec3;

typedef struct {
    u8 pad0[0xb0];
    Vec3 pos;
    u8 padbc[0xc8 - 0xbc];
    HalfVal yaw[3];
    u8 padce[2];
    u8 f_d0;
    u8 f_d1;
} Ent;

typedef struct {
    u8 pad0[0x4c];
    Ent *slots[4];
} Globals;

typedef struct {
    u8 pad0[0x1c];
    HalfVal ang[3];
    u8 pad22[2];
    s16 mode;
    u8 pad26[0x3f - 0x26];
    u8 flag;
} Src;

typedef struct {
    u8 pad0[0x21c0];
    Src src[4];
} Owner;

typedef struct {
    u8 pad0[0x2a];
    u8 cur;
} Other;

extern Globals data_0210930c;
extern Other data_021027f8;

void func_02001688(s32);
void func_020016c0(void);
void func_02091b68(Globals *, s32);
Ent *func_02091b88(Globals *, s32, s32, void *, u8);
void func_020a0370(void *, void *);
void *func_020aaeb4(void);

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

static inline int half_eq(const HalfVal *a, HalfVal b) {
    return a->v == b.v;
}

static inline void put_half(HalfVal *d, HalfVal v) {
    d->v = v.v;
}

static inline int vec_ne(const Vec3 *a, const Vec3 *b) {
    if (a->x != b->x || a->y != b->y || a->z != b->z) {
        return 1;
    }
    return 0;
}

static inline void set_pos(Ent *e, const Vec3 *p) {
    if (vec_ne(&e->pos, p)) {
        vec_set(&e->pos, p);
        e->f_d1 = 1;
    }
}

static inline void sync_ang(Ent *o, Owner *s, u8 i) {
    int eq;
    int all;
    all = 0;
    eq  = 0;
    if (half_eq(
            (const HalfVal *) ((u8 *) s + i * 64 + 0x21dc),
            o->yaw[0])) { // FAKE: raw spelling of &s->src[i].ang[0] keeps its address live from this compare to the copy below
        if (half_eq(&s->src[i].ang[1], o->yaw[1])) {
            eq = 1;
        }
    }
    if (eq) {
        if (half_eq(&s->src[i].ang[2], o->yaw[2])) {
            all = 1;
        }
    }
    if (!all) {
        put_half(&o->yaw[0],
                 *(const HalfVal *) ((u8 *) s + i * 64 +
                                     0x21dc)); // FAKE: same raw spelling as the first compare so the address is reused
        put_half(&o->yaw[1], s->src[i].ang[1]);
        put_half(&o->yaw[2], s->src[i].ang[2]);
        o->f_d0 = 1;
    }
}

void func_020897f8(Owner *arg0) {
    u8 i;
    Vec3 pos;
    Ent *cur;
    Ent *e;

    func_02001688(0);
    for (i = 0; i < SLOT_COUNT; i++) {
        cur = data_0210930c.slots[i];
        if (cur == NULL) {
            continue;
        }
        pos.x = cur->pos.x;
        pos.y = cur->pos.y;
        pos.z = cur->pos.z;
        if (arg0->src[i].mode != MODE_SENTINEL && arg0->src[i].flag != 0) {
            e = func_02091b88(&data_0210930c, i, arg0->src[i].mode, &arg0->src[i], 1);
            if (i == data_021027f8.cur) {
                func_020a0370(func_020aaeb4(), e);
            }
            set_pos(e, &pos);
            sync_ang(e, arg0, i);
        } else if (arg0->src[i].mode != MODE_SENTINEL && arg0->src[i].flag == 0) {
            set_pos(cur, &pos);
        } else if (arg0->src[i].mode == MODE_SENTINEL || arg0->src[i].flag == 0) {
            func_02091b68(&data_0210930c, i);
        }
    }
    func_020016c0();
}
