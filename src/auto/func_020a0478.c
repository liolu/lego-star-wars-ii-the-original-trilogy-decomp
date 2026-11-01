#include "m2c.h"

/* Syncs an object's cached transform (called from the func_020a0630 update).
 *
 * Position: the target is pos (+0x28), or pos + offset (+0x29c) when the object's
 * per-record mode byte (+0x208, indexed by the record byte at data_021027f8 + 0x2a,
 * the same block func_020a41f4 polls) is 1 and the byte at +0x23e is 1. If it differs
 * from curPos (+0x4c), curPos is updated and bit 0x8 of flags (+0x1ac) is set.
 * Rotation: if the angles at +0x40 differ from curRot (+0x58), curRot is updated, the
 * matrix at +0x60 is rebuilt from the angles by func_01ffa9e8, and flags is set to -1.
 */
enum {
    REC_INDEX_OFFSET = 0x2a,
    MODE_ONE         = 1,
    FLAG_POS_CHANGED = 0x8,
};

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct {
    u16 v;
} Angle;

typedef struct {
    Angle x;
    Angle y;
    Angle z;
} Rot;

typedef struct {
    s32 m[4][4];
} Mtx44;

typedef struct {
    u8 pad00[0x28];
    Vec3 pos;
    u8 pad34[0x40 - 0x34];
    Rot rot;
    u8 pad46[0x4c - 0x46];
    Vec3 curPos;
    Rot curRot;
    u8 pad5e[0x60 - 0x5e];
    Mtx44 rotMtx;
    u8 padA0[0x1ac - 0xa0];
    u32 flags;
    u8 pad1b0[0x208 - 0x1b0];
    u8 mode[0x23e - 0x208];
    u8 f23e;
    u8 pad23f[0x29c - 0x23f];
    Vec3 offset;
} Obj;

void func_01ffa9e8(Mtx44 *m, Angle *rot);

extern u8 data_021027f8[];

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

static inline Angle angle_get(const Angle *a) {
    return *a;
}

static inline s32 rot_eq(const Rot *a, const Rot *b) {
    return a->x.v == angle_get(&b->x).v && a->y.v == angle_get(&b->y).v && a->z.v == angle_get(&b->z).v;
}

static inline void obj_set_pos(Obj *o, const Vec3 *p) {
    if (vec_ne(&o->curPos, p)) {
        vec_set(&o->curPos, p);
        o->flags |= FLAG_POS_CHANGED;
    }
}

static inline void obj_update_rot(Obj *o) {
    if (rot_eq(&o->rot, &o->curRot)) {
        return;
    }
    o->curRot.x.v = angle_get(&o->rot.x).v;
    o->curRot.y.v = angle_get(&o->rot.y).v;
    o->curRot.z.v = angle_get(&o->rot.z).v;
    func_01ffa9e8(&o->rotMtx, &o->rot.x);
    o->flags = -1;
}

void func_020a0478(Obj *self) {
    if (self->mode[data_021027f8[REC_INDEX_OFFSET]] == MODE_ONE && self->f23e == 1) {
        Vec3 p;
        s32 z = self->pos.z + self->offset.z;
        s32 y = self->pos.y + self->offset.y;
        s32 x = self->pos.x + self->offset.x;
        p.x   = x;
        p.y   = y;
        p.z   = z;
        obj_set_pos(self, &p);
    } else {
        const Vec3 *p = &self->pos;
        obj_set_pos(self, p);
    }
    obj_update_rot(self);
}
