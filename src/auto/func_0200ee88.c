#include "m2c.h"

typedef struct {
    s32 v;
} Fx;

typedef struct Node {
    u8 pad_00[0xbc];
    s32 sx;
    s32 sy;
    u8 pad_c4[0xd1 - 0xc4];
    u8 dirty;
    u8 pad_d2[0xdc - 0xd2];
    s32 dc;
    s32 e0;
    s32 e4;
    s32 e8;
    u8 pad_ec[0xf6 - 0xec];
    u8 f6;
} Node;

typedef struct Ref {
    struct Ref **owner;
    struct Ref *next;
} Ref;

typedef struct {
    u16 id;
    u8 pad_02[6];
    u8 name[0x10];
} Entry;

typedef struct Self {
    u8 pad_000[0x6bb];
    u8 idx;
    u8 pad_6bc[0x820 - 0x6bc];
    Ref ref820;
    Ref ref828;
    s32 f830;
} Self;

extern void func_0200ee24(Self *self);
extern void func_0200f244(Ref *reference, Node *object);
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
extern void *func_020199d0(void *, s32, void *, void *);
extern Entry data_020ef57c[];
extern Entry data_020ef588[];

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

static inline void node_set_sx(Node *n, s32 v) {
    node_set_sx_fx(n, (Fx) {v});
}

static inline void node_set_sy(Node *n, s32 v) {
    node_set_sy_fx(n, (Fx) {v});
}

/*
 * A Ref is a tracked reference to an object: owner points at the chain head
 * stored at object+4, next is the following Ref in that chain.
 */

/* Remove r from its object's chain (no-op when r is unbound). */
static inline void ref_unlink(Ref *r) {
    if (r->owner != NULL) {
        Ref **pp = r->owner;
        while (*pp != r) {
            pp = &(*pp)->next;
        }
        *pp = r->next;
    }
}

/* Rebind d to the object s refers to: chain head -> object -> chain head. */
static inline void ref_assign(Ref *d, Ref *s) {
    char *object = (char *) s->owner;
    if (object != NULL) {
        object -= 4;
    }
    if (object != NULL) {
        object += 4;
    }
    ref_unlink(d);
    if (object != NULL) {
        d->owner         = (Ref **) object;
        d->next          = *(Ref **) object;
        *(Ref **) object = d;
    } else {
        d->owner = NULL;
        d->next  = NULL;
    }
}

/* d = node, through a temporary Ref built by func_0200f244. */
static inline void ref_set(Ref *d, Node *n) {
    Ref tmp;
    func_0200f244(&tmp, n);
    ref_assign(d, &tmp);
    ref_unlink(&tmp);
}

enum {
    MESH_ALLOC_SIZE = 0x2c,
    MAT_ALLOC_SIZE  = 0xc,
    NODE_ALLOC_SIZE = 0xf8,
};

/*
 * Rebuilds the two nodes referenced at self+0x820 and self+0x828.
 * func_0200ee24 first ORs 2 into +8 of the current referents and clears
 * self->f830. Node 1: mesh from resource 0x26c, scale 0x7000.
 * Node 2: mesh from table entry self->idx (name and id), scale 0x5000.
 * The scale setters mark a node dirty (+0xd1) only on change. Each node is
 * bound into its Ref slot, then self->f830 is set to 0x3c.
 */
void func_0200ee88(Self *self) {
    void *res;
    void *mesh;
    void *mat;
    Node *n;

    func_0200ee24(self);
    res = func_0205ab58(0x26c);

    mesh = func_0205e80c(MESH_ALLOC_SIZE, 3, 4);
    if (mesh) {
        mesh = func_02059fb4(mesh, 0x40, 0x40, 3);
    }
    func_0205a63c(mesh, (u8 *) res + 0x20);
    func_0205a060(mesh);
    mat = func_0205e80c(MAT_ALLOC_SIZE, 3, 4);
    if (mat) {
        mat = func_0205a96c(mat, 0x10);
    }
    func_0205aa74(mat, res);
    func_0205a9ec(mat);
    n = func_0205e80c(NODE_ALLOC_SIZE, 0, 4);
    if (n) {
        n = func_020199d0(n, 0, mesh, mat);
    }
    node_set_sx(n, 0x7000);
    node_set_sy(n, 0x7000);
    n->dc = 0;
    n->e0 = 0x800;
    n->e4 = 0x26800;
    n->e8 = 0x26000;
    n->f6 = 1;
    ref_set(&self->ref820, n);

    mesh = func_0205e80c(MESH_ALLOC_SIZE, 3, 4);
    if (mesh) {
        mesh = func_0205a018(mesh, data_020ef57c[self->idx].name, 0);
    }
    func_0205a060(mesh);
    mat = func_0205e80c(MAT_ALLOC_SIZE, 3, 4);
    if (mat) {
        mat = func_0205a96c(mat, 0x10);
    }
    func_0205aa74(mat, func_0205ab44(data_020ef588[self->idx].id));
    func_0205a9ec(mat);
    n = func_0205e80c(NODE_ALLOC_SIZE, 0, 4);
    if (n) {
        n = func_020199d0(n, 0, mesh, mat);
    }
    node_set_sx(n, 0x5000);
    node_set_sy(n, 0x5000);
    n->dc = 0;
    n->e0 = 0;
    n->e4 = 0x20000;
    n->e8 = 0x20000;
    n->f6 = 1;
    ref_set(&self->ref828, n);

    self->f830 = 0x3c;
}
