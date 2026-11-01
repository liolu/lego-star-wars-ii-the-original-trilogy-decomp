/* Initialises an object that embeds a transform (func_020acc48 layout: position at
 * +0x24, scale at +0x30, rotation halfwords at +0x3C, dirty flag at +0x45):
 *  - transform = position data_020ff9b4 (zero vector), zero rotation, scale 0x1000
 *    (fixed-point 1.0) on each axis;
 *  - clears +0x48 and +0x64, chains the 64 embedded 12-byte nodes at +0x68 through
 *    their +4 links (node 63's link is not written) and stores the array end and
 *    start at +0x37C and +0x380;
 *  - clears +0x360, +0x36C..+0x378, +0x384, +0x388 and +0x398, sets +0x38C to the
 *    largest s32 and the byte at +0x39C to 1;
 *  - pops a node from the free list of data_0210e1ec (+0x504), links it after the
 *    node at +0x500, makes it the head (+0x4FC) when that node was the head or the
 *    count (+0x4F8) was 0, bumps the count and stores this object as the node owner;
 *  - zeroes the six words at +0x4C, then rewrites the position from data_020ff9b4 and
 *    sets the dirty flag only if the position differs.
 * Returns the object.
 */
#include "m2c.h"

enum {
    FX_ONE = 0x1000
};
enum {
    NODE_COUNT = 0x40
};
enum {
    S32_MAX = 0x7FFFFFFF
};

typedef struct Node {
    void *owner;
    struct Node *prev;
    struct Node *next;
} Node;

typedef struct Pool {
    u8 pad[0x4F8];
    s32 count;
    Node *head;
    Node *tail;
    Node *free_list;
} Pool;

extern Pool data_0210e1ec;
extern const s32 data_020ff9b4[3];
void func_020acc48(void *, void *, void *, void *);

static inline s32 vec_ne(const s32 *a, const s32 *b) {
    if (a[0] != b[0] || a[1] != b[1] || a[2] != b[2]) {
        return 1;
    }
    return 0;
}

static inline void zero_words(s32 *p, s32 n) {
    s32 j;
    for (j = 0; j < n; j++) {
        p[j] = 0;
    }
}

static inline Node *pool_push(Pool *pool) {
    Node *node;
    Node *tail;

    node            = pool->free_list;
    tail            = pool->tail;
    pool->free_list = node->prev;
    if (tail->next != NULL) {
        tail->next->prev = node;
    }
    node->next = tail->next;
    node->prev = tail;
    tail->next = node;
    if (tail == pool->head || pool->count == 0) {
        node->next = pool->tail;
        pool->head = node;
    }
    pool->count = pool->count + 1;
    return node;
}

static inline void vec_set(s32 *d, const s32 *s) {
    if (d != s) {
        d[0] = s[0];
        d[1] = s[1];
        d[2] = s[2];
    }
}

typedef struct Obj {
    u8 pad[0x68];
    Node nodes[NODE_COUNT];
} Obj;

void *func_020bbfe8(void *arg0) {
    u8 *self = arg0;
    u16 rot[3];
    s32 scale[3];
    s32 i;
    Node *node;

    scale[0] = FX_ONE;
    scale[1] = FX_ONE;
    scale[2] = FX_ONE;
    rot[0]   = 0;
    rot[1]   = 0;
    rot[2]   = 0;
    func_020acc48(self, data_020ff9b4, rot, scale);
    M2C_FIELD(self, s32 *, 0x48) = 0;
    M2C_FIELD(self, s32 *, 0x64) = 0;
    for (i = 0; i < NODE_COUNT - 1; i++) {
        ((Obj *) self)->nodes[i].prev = (Node *) ((u8 *) ((Obj *) self)->nodes + (i + 1) * sizeof(Node));
    }
    M2C_FIELD(self, s32 *, 0x360) = 0;
    M2C_FIELD(self, s32 *, 0x36C) = 0;
    M2C_FIELD(self, s32 *, 0x370) = 0;
    M2C_FIELD(self, s32 *, 0x374) = 0;
    M2C_FIELD(self, s32 *, 0x378) = 0;
    M2C_FIELD(self, u8 **, 0x37C) = (u8 *) ((Obj *) self)->nodes + NODE_COUNT * sizeof(Node);
    M2C_FIELD(self, u8 **, 0x380) = self + 0x68;
    M2C_FIELD(self, s32 *, 0x384) = 0;
    M2C_FIELD(self, s32 *, 0x388) = 0;
    M2C_FIELD(self, s32 *, 0x38C) = S32_MAX;
    M2C_FIELD(self, s32 *, 0x398) = 0;
    M2C_FIELD(self, u8 *, 0x39C)  = 1;

    node        = pool_push(&data_0210e1ec);
    node->owner = self;
    zero_words((s32 *) (self + 0x4C), 6);
    if (vec_ne((s32 *) (self + 0x24), data_020ff9b4)) {
        vec_set((s32 *) (self + 0x24), data_020ff9b4);
        M2C_FIELD(self, u8 *, 0x45) = 1;
    }
    return self;
}
