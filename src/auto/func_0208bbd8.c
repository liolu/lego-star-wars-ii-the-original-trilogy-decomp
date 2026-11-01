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
    u8 pad_00[0x8];
    u32 flags;
    u8 pad_0c[0xb0 - 0xc];
    Vec3 pos;
    s32 sx;
    s32 sy;
    u8 pad_c4[0xd1 - 0xc4];
    u8 dirty;
    u8 pad_d2[0xdc - 0xd2];
    s32 dc;
    s32 e0;
    s32 e4;
    s32 e8;
    u8 pad_ec[0xf5 - 0xec];
    u8 f5;
} Node;

typedef struct {
    u8 name[4];
    u16 id;
} Entry;

typedef struct Obj {
    u8 pad_00[0x64];
    Node *a;
    Node *b;
} Obj;

extern void *func_0205e80c(s32, s32, s32);
extern void *func_02059fb4(void *, s16, s16, s32);
extern void *func_0205a018(void *, void *, s32);
extern void func_0205a060(void *);
extern void func_0205a3a0(void *, s16, s16, s8);
extern void *func_0205a96c(void *, s16);
extern void func_0205a9ec(void *);
extern void func_0205aa74(void *, const void *);
extern const void *func_0205ab44(u16);
extern void *func_0205ab1c(u32);
extern void func_0205ab6c(u32);
extern u32 func_02059efc(void *, void *);
extern void *func_020199d0(void *, s32, void *, void *);
extern void func_0201a38c(void *, s16);
extern Entry data_020f024c;

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

static inline void node_sx_pp(Node **pp, Fx x) {
    node_set_sx_fx(*pp, x);
}

static inline void node_sy_pp(Node **pp, Fx y) {
    node_set_sy_fx(*pp, y);
}

static inline void node_sx(Node **pp, s32 v) {
    node_sx_pp(pp, (Fx) {v});
}

static inline void node_sy(Node **pp, s32 v) {
    node_sy_pp(pp, (Fx) {v});
}

/*
 * Fills self's two child nodes (0x64 and 0x68). Each node gets a freshly
 * allocated mesh (MESH_SIZE) and material (MAT_SIZE, PAL_COUNT colours
 * copied from palette data by func_0205aa74), then is built by
 * func_020199d0. Node 1 uses the name and palette id of data_020f024c;
 * node 2 uses a generated mesh fed by resource NODE2_RES_ID and palette
 * NODE2_PAL_ID. The setters mark a node dirty (0xd1) only when a value
 * changes. Values are 20.12 fixed point: node 1 gets 32.0 / 32.0 in
 * sx/sy and e4/e8 at (60, 106), node 2 gets 128.0 / 32.0 at (142, 122).
 * Both nodes then get func_0201a38c(node, 3) and lose flags bit 0.
 */
enum {
    MESH_SIZE    = 0x2c,
    MAT_SIZE     = 0xc,
    NODE_SIZE    = 0xf8,
    PAL_COUNT    = 0x10,
    FX_32        = 0x20000,
    FX_128       = 0x80000,
    NODE1_X      = 0x3c000,
    NODE1_Y      = 0x6a000,
    NODE2_X      = 0x8e000,
    NODE2_Y      = 0x7a000,
    NODE2_RES_ID = 0x1273,
    NODE2_PAL_ID = 0x1276,
};

void func_0208bbd8(Obj *self) {
    void *mesh;
    void *mat;
    Node *n;
    Vec3 pos;
    Vec3 pos2;
    const Vec3 *src;

    mesh = func_0205e80c(MESH_SIZE, 3, 4);
    if (mesh) {
        mesh = func_0205a018(mesh, data_020f024c.name, 1);
    }
    func_0205a060(mesh);
    mat = func_0205e80c(MAT_SIZE, 3, 4);
    if (mat) {
        mat = func_0205a96c(mat, PAL_COUNT);
    }
    src = func_0205ab44(data_020f024c.id); // FAKE: palette data parked in the pos pointer so src = &pos is a later web
                                           // (colouring zero=r3 node=r2 pos=r5)
    func_0205aa74(mat, src);
    func_0205a9ec(mat);
    n = func_0205e80c(NODE_SIZE, 3, 4);
    if (n) {
        n = func_020199d0(n, 0, mesh, mat);
    }
    self->a = n;
    node_sx(&self->a, FX_32);
    node_sy(&self->a, FX_32);
    n           = self->a;
    n->dc       = 0;
    n->e0       = 0;
    n->e4       = FX_32;
    n->e8       = FX_32;
    self->a->f5 = 1;
    pos.x       = NODE1_X;
    pos.y       = NODE1_Y;
    pos.z       = 0;
    src         = &pos;
    node_set_pos(self->a, src);
    func_0201a38c(self->a, 3);
    self->a->flags &= ~1;

    mesh = func_0205e80c(MESH_SIZE, 3, 4);
    if (mesh) {
        mesh = func_02059fb4(mesh, 0x80, 0x140, 3);
    }
    func_02059efc(func_0205ab1c(NODE2_RES_ID), *(void **) mesh);
    func_0205ab6c(NODE2_RES_ID);
    func_0205a3a0(mesh, 0x80, 0x20, 10);
    func_0205a060(mesh);
    mat = func_0205e80c(MAT_SIZE, 3, 4);
    if (mat) {
        mat = func_0205a96c(mat, PAL_COUNT);
    }
    func_0205aa74(mat, func_0205ab44(NODE2_PAL_ID));
    func_0205a9ec(mat);
    n = func_0205e80c(NODE_SIZE, 3, 4);
    if (n) {
        n = func_020199d0(n, 0, mesh, mat);
    }
    self->b = n;
    node_sx(&self->b, FX_128);
    node_sy(&self->b, FX_32);
    n           = self->b;
    n->dc       = 0;
    n->e0       = 0;
    n->e4       = FX_128;
    n->e8       = FX_32;
    self->b->f5 = 1;
    pos2.x      = NODE2_X;
    pos2.y      = NODE2_Y;
    pos2.z      = 0;
    src         = &pos2;
    node_set_pos(self->b, src);
    func_0201a38c(self->b, 3);
    self->b->flags &= ~1;
}
