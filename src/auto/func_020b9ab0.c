#include "m2c.h"

typedef struct {
    s32 x, y, z;
} Vec3;
typedef struct {
    u16 v;
} Ang;
typedef struct {
    Ang x, y, z;
} Rot;
typedef struct {
    Vec3 pos;
    Vec3 scale;
    Rot rot;
    u16 pad;
    u8 dirty;
    u8 flag;
} Xform;
typedef struct {
    u8 pad0[0x38];
    Vec3 pos;
    Vec3 out;
} Obj;
typedef struct {
    u8 pad0[0x40];
    Rot rot;
} Src;
typedef struct {
    u8 pad0[0xb0];
    Vec3 pos;
} Ent;

extern u8 data_021027f8[];
extern Ent *data_02109358[];

extern Src *func_020aaeb4(void);
extern void func_01ffa6f8(Vec3 *, Rot *);

static inline Ang ang_of(const Ang *p) {
    return *p;
}

static inline s32 rot_eq0(const Rot *a, const Rot *b) {
    return a->x.v == ang_of(&b->x).v && a->y.v == ang_of(&b->y).v && a->z.v == ang_of(&b->z).v;
}

/* FAKE: rot_eq() only forwards to rot_eq0(); the extra inline depth sets the stack temporary order */
static inline s32 rot_eq(const Rot *a, const Rot *b) {
    return rot_eq0(a, b);
}

static inline void rot_set(Rot *d, const Rot *s) {
    d->x.v = ang_of(&s->x).v;
    d->y.v = ang_of(&s->y).v;
    d->z.v = ang_of(&s->z).v;
}

/* 20.12 fixed point: 0x1000 is 1.0 (identity scale). */
enum {
    FX_ONE   = 0x1000,
    FX_SHIFT = 12
};

/*
 * Copies the Vec3 at +0xb0 of the entity picked by the byte index
 * data_021027f8[0x2a] from the pointer table data_02109358[] into obj->pos
 * (skipped when the slot is NULL). Then builds a local transform (zero pos,
 * identity scale, zero rot), takes the three 16-bit angles at +0x40 of the
 * object returned by func_020aaeb4() and, when they differ or the transform is
 * dirty, has func_01ffa6f8 fill the 3x3 matrix m from them. Row m[0] goes to
 * obj->out through a 64-bit << 12 >> 12 fixed-point round trip.
 */
void func_020b9ab0(Obj *obj) {
    Xform t;
    Vec3 m[3];
    Src *s;
    Ent *e = data_02109358[data_021027f8[0x2a]];

    if (e != 0) {
        Vec3 *q = &e->pos;
        if (&obj->pos != q) {
            obj->pos.x = q->x;
            obj->pos.y = q->y;
            obj->pos.z = q->z;
        }
    }
    t.rot.x.v = 0;
    t.pos.x   = 0;
    t.pos.y   = 0;
    t.pos.z   = 0;
    t.scale.x = FX_ONE;
    t.scale.y = FX_ONE;
    t.scale.z = FX_ONE;
    t.rot.y.v = 0;
    t.rot.z.v = 0;
    t.dirty   = 1;
    t.flag    = 1;
    s         = func_020aaeb4();
    if (!rot_eq(&s->rot, &t.rot)) {
        rot_set(&t.rot, &s->rot);
        t.dirty = 1;
    }
    if (t.dirty) {
        t.flag  = 1;
        t.dirty = 0;
        func_01ffa6f8(m, &t.rot);
    }
    obj->out.x = (s32) (((s64) m[0].x << FX_SHIFT) >> FX_SHIFT);
    obj->out.y = (s32) (((s64) m[0].y << FX_SHIFT) >> FX_SHIFT);
    obj->out.z = (s32) (((s64) m[0].z << FX_SHIFT) >> FX_SHIFT);
}
