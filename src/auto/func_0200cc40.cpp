#include "m2c.h"

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

// Partial interface: unknown virtuals retain their observed vtable slots.
// Declarations only: the original vtable remains in the ROM data.
class Ent {
public:
    virtual void unknown00();
    virtual void unknown04();
    virtual s32 getId(); // Slot 0x08.

    u8 pad4[4];
    u32 flags;
    u16 state;
    u8 pade[0xb0 - 0xe];
    Vec3 pos;
    u8 padbc[0xca - 0xbc];
    u16 ang;
    u8 padcc[0x1bc - 0xcc];
    s32 id;
    u8 has_slot;
    u8 pad1c1;
    u8 busy;
    u8 pad1c3[5];
    s32 mode;
    Vec3 rel;
    u16 rel_ang;
    u8 pad1da[2];
    Ent *target;
};

extern "C" void func_0200cef4(Ent *);
extern "C" Ent *data_02109358[];
// Interleaved fx16 sine/cosine pairs indexed by a 12-bit angle index.
extern "C" const s16 data_020f88c4[];

enum {
    MODE_NO_TARGET = 1,
    MODE_TARGET    = 2,
};

static inline s16 sin_idx(int idx) {
    return data_020f88c4[idx * 2];
}

static inline s16 cos_idx(int idx) {
    return data_020f88c4[idx * 2 + 1];
}

static inline void vec_sub(Vec3 *d, const Vec3 *a, const Vec3 *b) {
    s32 z = a->z - b->z;
    s32 y = a->y - b->y;
    s32 x = a->x - b->x;
    d->x  = x;
    d->y  = y;
    d->z  = z;
}

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

typedef struct {
    u16 v;
} Angle;

static inline void set_rel_ang(Ent *self, Angle a) {
    self->rel_ang = a.v;
}

static inline Angle angle_of(u16 v) {
    Angle a;
    a.v = v;
    return a;
}

static inline s32 get_id(Ent *o) {
    return o->getId();
}

// Callback installed at +0x10 by func_0200cb48 (also reached from func_0200ce88).
// Does nothing when self is null or mode is already set. With a target in
// *arg1: returns early if the target (state 1, has_slot set) is
// data_02109358[id] or reports the same id via virtual slot 0x08; otherwise records the target, stores
// self->pos - target->pos in rel, copies the target angle to rel_ang and
// rotates rel.x/rel.z by -rel_ang (fx 20.12). Without a target, rel = pos and
// rel_ang = 0. If busy is set, func_0200cef4 runs instead. Flag 0x20 is
// cleared on every path that gets past the mode check.
extern "C" void func_0200cc40(Ent *self, Ent **arg1) {
    Vec3 diff;
    Ent *other;
    Vec3 *p;
    u16 ang;
    s32 idx;
    s32 sin_v;
    s32 cos_v;
    s32 x;
    s32 z;
    s32 nx;
    s32 t;

    if (self == (Ent *) 0) {
        return;
    }
    if (self->mode != 0) {
        return;
    }
    other = *arg1;
    if (other != (Ent *) 0) {
        if (other->state == 1 && self->has_slot != 0 && other == data_02109358[self->id]) {
            return;
        }
        if (self->id == get_id(other)) {
            return;
        }
        if (self->busy != 0) {
            self->flags &= ~0x20;
            func_0200cef4(self);
            return;
        }
        self->mode   = MODE_TARGET;
        self->target = *arg1;
        vec_sub(&diff, &self->pos, &(*arg1)->pos);
        vec_set(&self->rel, &diff);
        ang = (*arg1)->ang;
        set_rel_ang(self, angle_of(ang));
        idx   = (u16) -self->rel_ang >> 4;
        sin_v = sin_idx(idx);
        cos_v = cos_idx(idx);
        x     = self->rel.x;
        z     = self->rel.z;
        t     = (s32) (((s64) z * sin_v + 0x800) >> 12);
        nx    = (s32) (((s64) x * cos_v + 0x800) >> 12);
        nx += t;
        t           = (s32) (((s64) x * sin_v + 0x800) >> 12);
        t           = (s32) (((s64) z * cos_v + 0x800) >> 12) - t;
        self->rel.x = nx;
        self->rel.z = t;
    } else {
        if (self->busy != 0) {
            self->flags &= ~0x20;
            func_0200cef4(self);
            return;
        }
        self->mode = MODE_NO_TARGET;
        p          = &self->pos;
        vec_set(&self->rel, p);
        ang = 0;
        set_rel_ang(self, (Angle) {ang});
    }
    self->flags &= ~0x20;
}
