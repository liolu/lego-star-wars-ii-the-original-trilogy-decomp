#include "m2c.h"

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct {
    s32 v;
} Fx;

typedef struct Node {
    u8 pad_00[0xb0];
    Vec3 pos;
    s32 sx;
    s32 sy;
    u8 pad_c4[0xd1 - 0xc4];
    u8 dirty;
    u8 pad_d2[2];
    void *mesh;
    void *mat;
    s32 dc;
    s32 e0;
    s32 e4;
    s32 e8;
    u8 pad_ec[0xf5 - 0xec];
    u8 f5;
} Node;

typedef struct {
    u16 id;
    u8 pad_02[6];
    u8 name[0x10];
} Entry;

extern void *func_0205ab58(s32);
extern void *func_0205e80c(s32, s32, s32);
extern void *func_02059fb4(void *, s16, s16, s32);
extern void *func_0205a018(void *, void *, s32);
extern void func_0205a060(void *);
extern void func_0205a63c(void *, void *);
extern void *func_0205a96c(void *, s16);
extern void func_0205a9ec(void *);
extern void func_0205aa74(void *, void *);
extern void *func_0205ab44(u16);
extern void *func_020199d0(void *, s32, s32, s32);
extern s32 func_020917b4(void *);
extern Entry data_020ef57c[];
extern Entry data_020ef588[];
extern u8 data_0210930c[];

typedef struct Obj {
    u8 pad_00[0x54];
    Node *nodes[4];
} Obj;

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

static inline void node_set_pos(Node *n, const Vec3 *v) {
    if (vec_ne(&n->pos, v)) {
        vec_set(&n->pos, v);
        n->dirty = 1;
    }
}

static inline void node_set_sx_fx(Node *n, Fx v) {
    if (n->sx != v.v) {
        n->sx    = v.v;
        n->dirty = 1;
    }
}

static inline void node_set_sy_fx(Node *n, Fx v) {
    if (n->sy != v.v) {
        n->sy    = v.v;
        n->dirty = 1;
    }
}
static inline void obj_set_sx(Obj *o, s32 i, s32 v) {
    node_set_sx_fx(o->nodes[i], (Fx) {v});
}

static inline void obj_set_sy(Obj *o, s32 i, s32 v) {
    node_set_sy_fx(o->nodes[i], (Fx) {v});
}

/*
 * Builds a pair of nodes in self->nodes[slot] and self->nodes[slot + 1]
 * (slot 0, or 2 when flag is set). Each node gets a freshly allocated
 * mesh and material object: the first from resource 0x26c, the second from
 * table entry idx (name and id). The setters only mark a node dirty (0xd1)
 * when a value actually changes. Both nodes are then moved to a shared
 * position; the flag path uses an alternative position unless
 * func_020917b4(data_0210930c) is 0x47..0x48.
 */
void func_0208b76c(Obj *self, u8 idx, u32 flag) {
    s32 slot;
    void *res;
    void *mesh;
    void *mat;
    Node *n;
    Vec3 pos;

    slot = 0;
    if (flag) {
        slot = 2;
    }
    res = func_0205ab58(0x26c);

    mesh = func_0205e80c(0x2c, 3, 4);
    if (mesh) {
        mesh = func_02059fb4(mesh, 0x40, 0x40, 3);
    }
    func_0205a63c(mesh, (u8 *) res + 0x20);
    func_0205a060(mesh);
    mat = func_0205e80c(0xc, 3, 4);
    if (mat) {
        mat = func_0205a96c(mat, 0x10);
    }
    func_0205aa74(mat, res);
    func_0205a9ec(mat);
    n = func_0205e80c(0xf8, 3, 4);
    if (n) {
        n = func_020199d0(n, 0, 0, 0);
    }
    self->nodes[slot]      = n;
    n->mesh                = mesh;
    self->nodes[slot]->mat = mat;
    obj_set_sx(self, slot, 0x26000);
    obj_set_sy(self, slot, 0x26000);
    n                     = self->nodes[slot];
    n->dc                 = 0;
    n->e0                 = 0x800;
    n->e4                 = 0x26000;
    n->e8                 = 0x26000;
    self->nodes[slot]->f5 = 1;

    mesh = func_0205e80c(0x2c, 3, 4);
    if (mesh) {
        mesh = func_0205a018(mesh, data_020ef57c[idx].name, 0);
    }
    func_0205a060(mesh);
    mat = func_0205e80c(0xc, 3, 4);
    if (mat) {
        mat = func_0205a96c(mat, 0x10);
    }
    func_0205aa74(mat, func_0205ab44(data_020ef588[idx].id));
    func_0205a9ec(mat);
    n = func_0205e80c(0xf8, 3, 4);
    if (n) {
        n = func_020199d0(n, 0, 0, 0);
    }
    self->nodes[slot + 1]      = n;
    n->mesh                    = mesh;
    self->nodes[slot + 1]->mat = mat;
    obj_set_sx(self, slot + 1, 0x20000);
    obj_set_sy(self, slot + 1, 0x20000);
    n                         = self->nodes[slot + 1];
    n->dc                     = 0;
    n->e0                     = 0;
    n->e4                     = 0x20000;
    n->e8                     = 0x20000;
    self->nodes[slot + 1]->f5 = 1;

    pos.x = 0x1a000;
    pos.y = 0x18000;
    pos.z = 0;
    if (flag) {
        Vec3 alt;
        const Vec3 *src;
        alt.x = 0x83000;
        alt.y = 0x9a000;
        alt.z = 0;
        src   = &alt;
        vec_set(&pos, src);
        if (func_020917b4(data_0210930c) >= 0x47 && func_020917b4(data_0210930c) <= 0x48) {
            pos.x = 0x1a000;
        }
    }
    node_set_pos(self->nodes[slot], &pos);
    node_set_pos(self->nodes[slot + 1], &pos);
}
