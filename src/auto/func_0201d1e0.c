#include "m2c.h"

/*
 * Virtual method of the object class whose vtable is data_020fd220
 * (func_0201cf70 installs that vtable and clears +0x1bc and +0x1c4).
 *
 * Follows a tracked object: the primary one at +0x1c4, else the fallback
 * at +0x1bc. When the tracked object passes the flag/type checks, its
 * position (or the position of its anchor at +0x30, unless its kind is
 * TRACK_KIND_DIRECT) is copied into this object's position and into the
 * position of the child object at +0x1c8 (created on demand by
 * func_0201d14c); each copy only happens when the vectors differ and then
 * sets that node's dirty byte. The child's byte at +0x39c is set to 1 while
 * tracking and to 0 otherwise. The fallback path also clears bit 0 of the
 * flags word at +8. Every path ends by calling func_02010384 with the same
 * arguments (arg1 is read back from the variadic register save area).
 */

typedef struct {
    s32 v;
} UnkVal;

typedef struct {
    s32 x, y, z;
} Vec3;

typedef struct {
    Vec3 pos;
    u8 pad0c[0x15];
    u8 dirty;
    u8 pad22[2];
} Node;

typedef struct {
    u8 pad00[4];
    Vec3 pos;
} Anchor;

typedef struct {
    u8 pad000[0xc];
    u16 type;
    u8 pad00e[0x22];
    Anchor *anchor;
    u8 pad034[0x7c];
    Node node;
    u8 pad0d4[0xc0];
    u32 flags;
    u8 pad198[0x7a];
    u16 kind;
} Obj;

typedef struct {
    u8 pad000[0x24];
    Node node;
    u8 pad048[0x354];
    u8 active;
} Child;

typedef struct {
    u8 pad000[8];
    u32 flags;
    u8 pad00c[0xa4];
    Node node;
    u8 pad0d4[0xe8];
    Obj *alt;
    u8 pad1c0[4];
    Obj *target;
    Child *child;
} Self;

void func_02010384(void *, UnkVal, ...);
void func_0201d14c(Self *);

enum {
    TRACK_FLAGS_MASK  = 0x1400700,
    TRACK_TYPE        = 4,
    TRACK_KIND_DIRECT = 0x1a,
    FLAG_BIT0         = 1,
};

static inline int vec_ne(const Vec3 *a, const Vec3 *b) {
    if (a->x != b->x || a->y != b->y || a->z != b->z) {
        return 1;
    }
    return 0;
}

static inline void self_set_pos(Self *self, const Vec3 *v) {
    if (vec_ne(&self->node.pos, v)) {
        if (&self->node.pos != v) {
            self->node.pos.x = v->x;
            self->node.pos.y = v->y;
            self->node.pos.z = v->z;
        }
        self->node.dirty = 1;
    }
}

static inline void child_set_pos(Child *child, const Vec3 *v) {
    if (vec_ne(&child->node.pos, v)) {
        if (&child->node.pos != v) {
            child->node.pos.x = v->x;
            child->node.pos.y = v->y;
            child->node.pos.z = v->z;
        }
        child->node.dirty = 1;
    }
}

static inline s32 anchor_valid(const Anchor *a) {
    s32 r;
    if (a != NULL) {
        r = 1;
    } else {
        r = 0;
    }
    return r;
}

void func_0201d1e0(Self *self, UnkVal arg1, ...) {
    Anchor *anchor;

    if (self->target != NULL) {
        if (self->target->flags & TRACK_FLAGS_MASK) {
            if (self->target->type == TRACK_TYPE) {
                if (self->target->kind == TRACK_KIND_DIRECT) {
                    self_set_pos(self, &self->target->node.pos);
                    func_0201d14c(self);
                    child_set_pos(self->child, &self->target->node.pos);
                    self->child->active = 1;
                } else {
                    anchor = self->target->anchor;
                    if (anchor_valid(anchor)) {
                        self_set_pos(self, &anchor->pos);
                        func_0201d14c(self);
                        child_set_pos(self->child, &anchor->pos);
                    } else {
                        self_set_pos(self, &self->target->node.pos);
                        func_0201d14c(self);
                        child_set_pos(self->child, &self->target->node.pos);
                    }
                    func_0201d14c(self);
                    self->child->active = 1;
                }
            } else {
                self_set_pos(self, &self->target->node.pos);
                if (self->child != NULL) {
                    self->child->active = 0;
                }
            }
        } else {
            if (self->child != NULL) {
                self->child->active = 0;
            }
        }
    } else {
        if (self->alt != NULL && (self->alt->flags & TRACK_FLAGS_MASK) && self->alt->type == TRACK_TYPE) {
            if (self->alt->kind == TRACK_KIND_DIRECT) {
                self_set_pos(self, &self->alt->node.pos);
                func_0201d14c(self);
                child_set_pos(self->child, &self->alt->node.pos);
            } else {
                anchor = self->alt->anchor;
                if (anchor_valid(anchor)) {
                    self_set_pos(self, &anchor->pos);
                    func_0201d14c(self);
                    child_set_pos(self->child, &anchor->pos);
                } else {
                    self_set_pos(self, &self->alt->node.pos);
                    func_0201d14c(self);
                    child_set_pos(self->child, &self->alt->node.pos);
                }
            }
            if (self->child != NULL) {
                self->child->active = 1;
            }
        } else {
            if (self->child != NULL) {
                self->child->active = 0;
            }
        }
        self->flags &= ~FLAG_BIT0;
    }
    func_02010384(self, arg1);
}
