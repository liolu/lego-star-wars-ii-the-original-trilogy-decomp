#include "m2c.h"

extern M2C_UNK data_020ece84[];
u8 func_020754e8(void *graph, void *node);

typedef struct Node {
    u8 pad0[0x10];
    void *value;
    u8 pad14[0x18];
    struct Node *next;
} Node;

typedef struct {
    u8 pad0[0x10];
    Node *head;
} Graph;

typedef struct {
    int a;
    Node *node;
} ListPos;

typedef struct {
    Node *x;
    ListPos first;
    ListPos cur;
} ListIter;

typedef struct {
    u8 id;
    int value;
} Entry;

enum {
    MAX_ENTRIES = 10
};

typedef struct {
    Graph *graph;
    Entry entries[MAX_ENTRIES];
} Obj;

static inline Node *iter_begin(ListIter *it, Node *head) {
    it->first.a    = 0;
    it->first.node = head;
    it->cur        = it->first;
    it->cur.a      = 0; // FAKE: redundant reset (first.a is already 0) orders the cur.node store before cur.a
    return it->cur.node;
}

static inline Node *iter_next(ListIter *it, Node *n) {
    it->x        = 0;
    it->cur.node = n->next;
    return it->cur.node;
}

/* Walks the graph's node list (head at graph+0x10, next at node+0x2c) and fills
 * up to MAX_ENTRIES entries with func_020754e8(graph, node) (stored as a byte)
 * and the node's value word. If any entry was filled, entries[0].value is
 * overwritten with data_020ece84. Returns the entry count (the caller,
 * func_02027098, stores it as a u8 at obj+0x55). */
int func_02027240(Obj *obj) {
    Node *cur;
    ListIter it;
    u32 i = 0;
    cur   = iter_begin(&it, obj->graph->head);
    if (cur != 0) {
        do {
            if (i >= MAX_ENTRIES) {
                break;
            }
            obj->entries[i].id    = func_020754e8(obj->graph, cur);
            obj->entries[i].value = (int) cur->value;
            i++;
            cur = iter_next(&it, cur);
        } while (cur != it.x);
    }
    if (i != 0) {
        obj->entries[0].value = (int) data_020ece84;
    }
    return i;
}
