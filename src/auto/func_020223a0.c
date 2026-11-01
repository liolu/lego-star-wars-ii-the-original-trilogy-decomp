#include "m2c.h"

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct {
    s16 x;
    s16 y;
    s16 z;
} Rot3;

typedef struct Parent {
    u8 pad0[4];
    Vec3 pos;
} Parent;

typedef struct Obj {
    u8 pad0[0x3C];
    Parent *parent;
    u8 pad1[0xB0 - 0x40];
    Vec3 pos;
    u8 pad2[0xCA - 0xBC];
    u16 rotY;
    u8 pad3[0xD1 - 0xCC];
    u8 posDirty;
    u8 pad4[0x1BE - 0xD2];
    s16 field_1BE;
    u8 pad5[0x22C - 0x1C0];
    struct Obj *child;
} Obj;

extern void func_020accac(void *, void *, void *, void *);
extern void *func_02015138(s32, s32, void *);

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

static inline int vec_ne(const Vec3 *a, const Vec3 *b) {
    if (a->x != b->x || a->y != b->y || a->z != b->z) {
        return 1;
    }
    return 0;
}

static inline void sync_pos(Obj *obj, const Vec3 *src) {
    if (vec_ne(&obj->pos, src)) {
        vec_set(&obj->pos, src);
        obj->posDirty = 1;
    }
}

enum {
    FX32_ONE = 0x1000
};   /* 1.0 in fx32 (20.12 fixed point) */
enum {
    CHILD_KIND = 0x19d
};  /* object kind passed to func_02015138 */

/*
 * Builds a transform from this object's position, Y rotation and unit scale,
 * creates a child object (kind 0x19d) with it and stores it at +0x22C. The
 * child's position is then synced to the parent object's position at +0x3C
 * (or to this object's own position when there is no parent); posDirty is set
 * only when the position actually changed. Finally clears child field +0x1BE.
 */
void func_020223a0(Obj *self) {
    u8 transform[0x48];
    Rot3 rot;
    Vec3 scale;
    Parent *parent;
    u32 has;

    scale.x = FX32_ONE;
    scale.y = FX32_ONE;
    scale.z = FX32_ONE;
    rot.x   = 0;
    rot.y   = self->rotY;
    rot.z   = 0;
    func_020accac(transform, &self->pos, &rot, &scale);
    self->child = func_02015138(CHILD_KIND, 0, transform);
    parent      = self->parent;
    has         = self->parent != NULL;
    if (has != 0) {
        sync_pos(self->child, &parent->pos);
    } else {
        Obj *child      = self->child;
        const Vec3 *src = &self->pos;
        sync_pos(child, src);
    }
    self->child->field_1BE = 0;
}
