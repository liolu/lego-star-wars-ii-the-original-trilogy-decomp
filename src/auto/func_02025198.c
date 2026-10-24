#include "m2c.h"

typedef struct Node {
    s32 x0;
    struct Node *next;
    u32 flags;
} Node;

typedef struct Entry {
    Node **first;
    Node *second;
} Entry;

typedef struct Owner {
    u8 pad[0x220];
    Entry entries[2];
    u8 count;
} Owner;

void func_02025198(Owner *self) {
    int i;
    for (i = 0; i < self->count; i++) {
        Node **p;
        Node **prev;
        u8 *a = (u8 *) self->entries[i].first;
        u8 *n = a;
        if (a != 0) {
            n = a - 4;
        }
        if (n == 0) {
            continue;
        }
        if (a != 0) {
            a = a - 4;
        }
        ((Node *) a)->flags |= 2;
        prev = 0;
        p    = self->entries[i].first;
        if (p != 0) {
            while (*p != (Node *) (self->entries + i)) {
                p    = &(*p)->next;
                prev = 0;
            }
            *p = self->entries[i].second;
        }
        if (prev != 0) {
            self->entries[i].first  = prev;
            self->entries[i].second = *prev;
            *prev                   = (Node *) (self->entries + i);
        } else {
            self->entries[i].first  = 0;
            self->entries[i].second = 0;
        }
    }
}
