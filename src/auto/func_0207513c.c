#include "m2c.h"

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct {
    u8 target;
    u8 flags;
} Edge;

typedef struct Node {
    u8 edgeCount;
    Edge edges[7];
    u8 *via;
    Vec3 pos;
    u8 pad20[4];
    s32 cost;
    s32 estimate;
    struct Node *parent;
    struct Node *prev;
    struct Node *listPrev;
    struct Node *listNext;
} Node;

typedef struct {
    Node **nodes;
    s32 count;
    s32 pad08;
    Node *open;
    Node *path;
} Graph;

typedef struct {
    u16 mode;
    u16 pad02;
    u16 result;
    u16 pad06;
    u32 input;
} SqrtRegs;

typedef struct {
    s32 v;
} Fx;

enum {
    FX_FRAC_MASK = 0xfff
};
enum {
    SQRT_REGS_ADDR = 0x40002b0
};
enum {
    EDGE_FLAG_SPECIAL = 0x40
};

Node *func_02074f30(Node **list);
void func_02074f50(Node **list, Node *node);
void func_02074f7c(Node **list, Node *node);

static inline const Fx *fx_trunc(Fx *out, s32 v) {
    if (v >= 0) {
        out->v = v & ~FX_FRAC_MASK;
        return out;
    }
    out->v = (v + FX_FRAC_MASK) & ~FX_FRAC_MASK;
    return out;
}

static inline s32 whole_sq(const Fx *f) {
    return (f->v >> 12) * (f->v >> 12);
}

static inline void vec_sub(Vec3 *d, const Vec3 *a, const Vec3 *b) {
    s32 z = a->z - b->z;
    s32 y = a->y - b->y;
    s32 x = a->x - b->x;
    d->x  = x;
    d->y  = y;
    d->z  = z;
}

static inline s32 vec_dist(SqrtRegs *sq, const Vec3 *a, const Vec3 *b) {
    Vec3 d;
    const Vec3 *v = &d;
    Fx ty, tx, tz;
    s32 sum;

    vec_sub(&d, a, b);
    sum = whole_sq(fx_trunc(&tx, v->x)) + whole_sq(fx_trunc(&ty, v->y)) + whole_sq(fx_trunc(&tz, v->z));
    *(volatile u16 *) &sq->mode  = 0; // FAKE: MMIO SQRTCNT write must not be merged
    *(volatile u32 *) &sq->input = sum; // FAKE: MMIO SQRT_PARAM write must stay after SQRTCNT
    while (*(volatile u16 *) &sq->mode & 0x8000) { // FAKE: MMIO busy poll must reload
    }
    return sq->result << 12;
}

/*
 * Best-first (A*-style) path search over g->nodes from start to goal.
 * Resets every node, then repeatedly pops the open list head (func_02074f30;
 * the list is kept in ascending estimate order by func_02074f7c). Each edge
 * whose flags pass mask (flag 0x40 edges only when mask has 0x40, which also
 * admits every edge) is relaxed: cost = parent cost + whole-unit distance
 * (hardware SQRT), estimate = cost + distance to goal; a cheaper path to a
 * queued node re-sorts it (func_02074f50 unlink, then reinsert). Once goal is
 * popped the back chain is walked to start, storing forward links in
 * parent, and g->path = start. Returns 1 on success, 0 if the open list
 * empties first.
 */
s32 func_0207513c(Graph *g, Node *start, Node *goal, u32 mask) {
    Node *const *it;
    Node *node;
    Node *next;
    s32 i;
    s32 cost;
    u32 any;
    SqrtRegs *sq;

    for (it = g->nodes; it != g->nodes + g->count; it++) {
        (*it)->cost     = 0;
        (*it)->estimate = 0;
        (*it)->parent   = 0;
        (*it)->prev     = 0;
        (*it)->via      = 0;
    }
    g->path = 0;
    g->open = 0;
    func_02074f7c(&g->open, start);
    start->via = 0;
    any        = mask & EDGE_FLAG_SPECIAL;
    for (;;) {
        if (g->open == 0) {
            return 0;
        }
        node       = func_02074f30(&g->open);
        node->prev = node->parent;
        if (node == goal) {
            break;
        }
        sq = (SqrtRegs *) SQRT_REGS_ADDR;
        for (i = 0; i < node->edgeCount; i++) {
            u32 flags;
            flags = node->edges[i].flags;
            next  = g->nodes[node->edges[i].target];
            if (!(flags & mask) || (flags & EDGE_FLAG_SPECIAL)) {
                if (!any) {
                    continue;
                }
            }
            cost = vec_dist(sq, &next->pos, &node->pos) + node->cost;
            if (next->parent != 0) {
                if (cost >= next->cost) {
                    continue;
                }
                if (next->prev != 0) {
                    continue;
                }
            }
            next->cost     = cost;
            next->estimate = cost + vec_dist(sq, &next->pos, &goal->pos);
            if (next->parent != 0) {
                func_02074f50(&g->open, next);
            }
            func_02074f7c(&g->open, next);
            next->parent = node;
            next->via    = &node->edges[i].flags;
        }
    }
    goal->parent = 0;
    for (; goal != start; goal = goal->prev) {
        goal->prev->parent = goal;
    }
    g->path = start;
    return 1;
}
