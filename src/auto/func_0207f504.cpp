#include "m2c.h"

class C7F504Item;

// Doubly linked node; the payload is an object with a virtual at slot 0x00.
struct C7F504Node {
    C7F504Item *item;
    C7F504Node *next;
    C7F504Node *prev;
};

// Partial interface: only the virtual at slot 0x00 is observed.
// Declarations only: the original vtable remains in the ROM data.
class C7F504Item {
public:
    virtual int unknown00();
};

// Raw view of the owning object: the list header is embedded at offset 0x8
// and its head pointer is at offset 0xc.
struct C7F504List {
    u32 unknown00;
    u32 unknown04;
    u32 base;
    C7F504Node *head;
};

extern "C" void func_0205d4c8(void *list, void *node);
extern "C" void func_0205e84c(void *data);

extern "C" void func_0207f504(C7F504List *obj) {
    C7F504Item *item;
    C7F504Node *node;
    C7F504Node *prev;

    node = obj->head;
    while (node != 0) {
        item = node->item;
        if (item->unknown00() != 0) {
            prev = node->prev;
            func_0205d4c8(&obj->base, node);
            func_0205e84c(item);
            if (prev != 0) {
                node = prev;
            } else {
                node = obj->head;
                continue;
            }
        }
        node = node->next;
    }
}
