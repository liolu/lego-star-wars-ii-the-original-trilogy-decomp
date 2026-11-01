/* Reconstructed from USA ARM9 ITCM at 0x01ff9f20 (616 bytes).
 *
 * Swept contact query. node->pos is moved by *delta for the test, then the
 * grid cells of data_021094ac visited by the func_02055530 iterator around
 * self are walked. Each entry object other qualifies when it has flag
 * FLAG_REQUIRED, none of FLAG_EXCLUDED, is not self, owns an active node and
 * its group bit is set in data_027e007c[self->group]; func_01ff84bc(node,
 * other_node) then decides the hit. On a hit node->pos is put back,
 * func_02008c44 yields a vector v, and when v . delta (20.12 fixed point) is
 * negative v and other are written to *out with dist 0 and the search stops.
 * node->pos always ends at its original value.
 */
#include "m2c.h"

enum {
    FLAG_REQUIRED = 0x4,
    FLAG_EXCLUDED = 0x200002,
    FX_SHIFT      = 12,
    FX_HALF       = 0x800,
};

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct {
    s32 cur;
    s32 col;
    s32 row;
    s32 col_min;
    s32 col_max;
    s32 row_max;
} GridIter;

typedef struct Node {
    s32 f0;
    Vec3 pos;
    u8 pad[0x26 - 0x10];
    u8 active;
} Node;

typedef struct Obj {
    s32 f0;
    s32 f4;
    u32 flags;
    u8 pad[0x20 - 0xC];
    Node *node;
    u8 pad2[0x84 - 0x24];
    s32 group;
} Obj;

typedef struct Entry {
    Obj *obj;
    struct Entry *next;
} Entry;

typedef struct {
    s32 w;
    Vec3 v;
    s32 dist;
    s32 f14;
    Obj *hit;
} Out;

extern M2C_UNK data_021094ac;
extern u32 data_027e007c[];

s32 func_01ff84bc(Node *a, Node *b);
void func_02008c44(Vec3 *out, Node *a, Node *b, Vec3 *dir);
void *func_02055530(GridIter *it, M2C_UNK *grid, Obj *obj);
void func_02055630(GridIter *it);
Entry *func_02055660(GridIter *it);
s32 func_02055684(GridIter *it);

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

static inline s32 vec_dot(const Vec3 *a, const Vec3 *b) {
    return ((s64) a->x * b->x + (s64) a->y * b->y + (s64) a->z * b->z + FX_HALF) >> FX_SHIFT;
}

static inline s32 can_collide(Obj *self, Node *node, Obj *other, Node *other_node) {
    if ((other->flags & FLAG_REQUIRED) != FLAG_REQUIRED) {
        return 0;
    }
    if (other->flags & FLAG_EXCLUDED) {
        return 0;
    }
    if (self == other) {
        return 0;
    }
    if (other_node == NULL || other_node->active == 0) {
        return 0;
    }
    if (!(data_027e007c[self->group] & (1 << other->group))) {
        return 0;
    }
    return func_01ff84bc(node, other_node);
}

void func_01ff9f20(Obj *self, Node *node, Vec3 *delta, Out *out) {
    Vec3 orig;
    Vec3 moved;
    GridIter it;
    Vec3 v;
    Entry *e;
    Obj *other;
    Node *other_node;
    s32 dot;

    orig.x = node->pos.x;
    orig.y = node->pos.y;
    orig.z = node->pos.z;
    vec_add(&moved, &orig, delta);
    vec_set(&node->pos, &moved);

    func_02055530(&it, &data_021094ac, self);
    while (!func_02055684(&it)) {
        for (e = func_02055660(&it); e != NULL; e = e->next) {
            other      = e->obj;
            other_node = other->node;
            if (can_collide(self, node, other, other_node)) {
                vec_set(&node->pos, &orig);
                func_02008c44(&v, node, other_node, delta);
                dot = vec_dot(&v, delta);
                if (dot < 0) {
                    vec_set(&out->v, &v);
                    out->hit  = other;
                    out->dist = 0;
                    return;
                }
                vec_set(&node->pos, &moved);
            }
        }
        func_02055630(&it);
    }
    vec_set(&node->pos, &orig);
}
