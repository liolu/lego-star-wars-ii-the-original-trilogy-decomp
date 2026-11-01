#include "m2c.h"

typedef struct {
    s32 v;
} WordArg;

typedef struct {
    u8 pad0[0x14];
    s32 pos[3];
    s32 radius;
} Node;

typedef struct {
    u8 id;
    u8 pad[7];
} Slot;

typedef struct {
    Node ***nodes;
    Slot slots[10];
    u8 pad54;
    u8 count;
    u8 cur;
} Path;

extern s32 func_0200aef8(void *, void *, WordArg);

/* Node referenced by slot i of the path (i == -1 selects the current slot). */
static inline Node *path_node(Path *path, s32 i) {
    if (i == -1) {
        i = path->cur;
    }
    return (*path->nodes)[path->slots[i].id];
}

/* Runs the position/radius test func_0200aef8 (also used by func_02027124 as a
 * "point within node radius" check) on the current path slot's node and pos.
 * The current slot index is the byte at 0x56 advanced by func_020272cc. */
s32 func_020272f4(Path *path, void *pos) {
    Node *n = path_node(path, path->cur);
    WordArg r;
    r.v = n->radius;
    return func_0200aef8(n->pos, pos, r);
}
