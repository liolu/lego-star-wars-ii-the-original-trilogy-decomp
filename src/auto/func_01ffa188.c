#include "m2c.h"

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

/* Collision shape hanging off an entity at +0x20; the probe moves its
 * position (+0x04..+0x0c) by the caller's offset during the tests. */
typedef struct {
    u8 pad[0x4];
    Vec3 pos;     /* 0x04 */
    u8 pad10[0x26 - 0x10];
    u8 f26;       /* 0x26: nonzero = shape takes part in overlap tests */
} Shape;

typedef struct Ent {
    u8 pad0[0x8];
    u32 flags;     /* 0x08 */
    u8 padc[0x20 - 0xc];
    Shape *shape;  /* 0x20 */
    u8 pad24[0x84 - 0x24];
    s32 group;     /* 0x84: index into the group mask table */
} Ent;

/* Grid-cell list node returned by func_02055660. */
typedef struct Link {
    Ent *ent;
    struct Link *next;
} Link;

/* Grid-cell iterator state (func_02055530 init, func_02055684 done,
 * func_02055660 current cell list, func_02055630 advance). */
typedef struct {
    u8 data[0x18];
} Iter;

enum {
    REQUIRED_FLAG = 0x4,      /* flags bit that must be set */
    EXCLUDE_FLAGS = 0x200002, /* any of these bits skips the entity */
};

extern M2C_UNK data_021094ac; /* the grid walked by the iterator */
extern u32 data_027e007c[];   /* per-group mask of groups this one may touch */

void *func_02055530(void *arg0, s32 arg1, void *arg2);
void func_02055630(void *arg0);
s32 func_02055660(const void *object);
s32 func_02055684(void *arg0);
/* Pairwise test between two shapes; nonzero means they overlap. */
s32 func_01ff84bc(void *a, void *b);

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

static inline void vec_add(Vec3 *d, const Vec3 *a, const Vec3 *b) {
    s32 z = a->z + b->z;
    s32 y = a->y + b->y;
    s32 x = a->x + b->x;
    d->x  = x;
    d->y  = y;
    d->z  = z;
}

/* Nonzero when `other` passes the flag, self and group filters and its
 * shape overlaps `shape`. */
static inline s32 can_hit(Ent *self, Ent *other, Shape *shape) {
    u32 flags          = other->flags;
    Shape *other_shape = other->shape;
    if ((flags & REQUIRED_FLAG) != REQUIRED_FLAG) {
        return 0;
    }
    if (flags & EXCLUDE_FLAGS) {
        return 0;
    }
    if (self == other) {
        return 0;
    }
    if (other_shape == NULL || other_shape->f26 == 0) {
        return 0;
    }
    if (!(data_027e007c[self->group] & (1 << other->group))) {
        return 0;
    }
    return func_01ff84bc(shape, other_shape);
}

/* Probe: move `shape` by `delta`, walk the grid cells around `self` and
 * return the first entity whose shape overlaps it (NULL if none). The
 * shape's position is restored before returning. */
Ent *func_01ffa188(Ent *self, Shape *shape, Vec3 *delta) {
    Vec3 saved;
    Vec3 moved;
    Iter it;
    Link *link;

    saved.x = shape->pos.x;
    saved.y = shape->pos.y;
    saved.z = shape->pos.z;
    vec_add(&moved, &saved, delta);
    vec_set(&shape->pos, &moved);
    func_02055530(&it, (s32) &data_021094ac, self);
    while (!func_02055684(&it)) {
        for (link = (Link *) func_02055660(&it); link != NULL; link = link->next) {
            Ent *other = link->ent;
            if (can_hit(self, other, shape)) {
                vec_set(&shape->pos, &saved);
                return other;
            }
        }
        func_02055630(&it);
    }
    vec_set(&shape->pos, &saved);
    return NULL;
}
