#include "m2c.h"

typedef struct Vec3 {
    s32 x, y, z;
} Vec3;
typedef struct Rot3 {
    s16 x, y, z, w;
} Rot3;
typedef struct Mtx {
    s32 m[18];
} Mtx;

typedef struct Obj {
    u8 pad0[8];
    u32 flags;
    u8 pad1[0x78];
    s32 state;
    u8 pad2[0x28];
    Vec3 pos;
    u8 pad3[0x15];
    u8 dirty;
} Obj;

typedef struct Parent {
    u8 pad0[4];
    Vec3 pos;
} Parent;

typedef struct Owner {
    u8 pad0[0x3c];
    Parent *parent;
    u8 pad1[0x70];
    Vec3 pos;      /* 0xb0 */
    u8 pad2[0xe];
    u16 yaw;       /* 0xca */
    u8 pad3[0x146];
    u16 kind;      /* 0x212 */
    u8 pad4[8];
    Obj *obj;      /* 0x21c */
} Owner;

enum {
    FX32_ONE = 0x1000
}; /* 1.0 in 20.12 fixed point */

extern void func_02001990(void *self, s32 mode);
extern Obj *func_02015138(u32 kind, u32 flags, void *transform);
extern void func_020accac(void *out, void *pos, void *rot, void *scale);

static inline int vec_ne(const Vec3 *a, const Vec3 *b) {
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
/* Copies s into the object's position; marks it dirty only if it changed. */
static inline void set_pos(Obj *o, const Vec3 *s) {
    if (vec_ne(&o->pos, s)) {
        vec_set(&o->pos, s);
        o->dirty = 1;
    }
}
/* Fills a transform block from self's position, a yaw-only rotation and a
   unit scale (func_020accac copies pos, rot and scale into it), then creates
   an object of the given kind from it. */
static inline Obj *make_obj(Owner *self, u32 kind, Vec3 *scale) {
    Rot3 rot;
    Mtx mtx;

    scale->x = FX32_ONE;
    scale->y = FX32_ONE;
    scale->z = FX32_ONE;
    rot.x    = 0;
    rot.y    = self->yaw;
    rot.z    = 0;
    func_020accac(&mtx, &self->pos, &rot, scale);
    return func_02015138(kind, 0, &mtx);
}

static inline s32 has_parent(Parent *p) {
    return p != NULL;
}
static inline Vec3 *owner_pos(Owner *o) {
    return &o->pos;
}
/* Creates self->obj with an object kind chosen by self->kind, moves it to the
   parent's position (self's own position when there is no parent), then sets
   its state to 0xd, clears flags 0x221, sets flag 0x1000000 and calls
   func_02001990(obj, 1). Kind 0x14d also calls func_02001990(self, 0). */
void func_0202532c(Owner *self) {
    Vec3 scale;

    if (self->kind == 0x14d) {
        self->obj = make_obj(self, 0x19c, &scale);
        func_02001990(self, 0);
    } else if (self->kind == 0xda) {
        self->obj = make_obj(self, 0x19e, &scale);
    } else if (self->kind == 0x161) {
        self->obj = make_obj(self, 0x199, &scale);
    } else {
        self->obj = make_obj(self, 0x19b, &scale);
    }
    if (has_parent(self->parent)) {
        Parent *parent = self->parent;
        set_pos(self->obj, &parent->pos);
    } else {
        set_pos(self->obj, owner_pos(self));
    }
    self->obj->state = 0xd;
    self->obj->flags &= ~0x221;
    self->obj->flags |= 0x1000000;
    func_02001990(self->obj, 1);
}
