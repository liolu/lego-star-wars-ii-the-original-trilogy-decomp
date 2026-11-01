#include "m2c.h"

// Partial interfaces: unknown virtuals retain their observed vtable slots.
// Declarations only: the original vtables remain in the ROM data.
class Item {
public:
    virtual void unknown00();
    virtual void unknown04(); // Slot 0x04: called last, only on a non-null item.
    virtual void unknown08();
    virtual void unknown0c();
    virtual void unknown10(); // Slot 0x10.

    u8 pad4[8];
    u8 flags;
};

struct NodeHead {
    void *head0;
};

// Second base of Node (at +4): the converted pointer is what the owner receives.
struct Link {
    Item *item;
};

struct Node : NodeHead, Link {};

class Owner {
public:
    virtual void unknown00();
    virtual void unknown04();
    virtual void unknown08();
    virtual void unknown0c();
    virtual void unknown10();
    virtual void unknown14();
    virtual void unknown18();
    virtual void unknown1c();
    virtual void unknown20();
    virtual void unknown24();
    virtual void unknown28(Link *link); // Slot 0x28.

    u8 pad4[8];
    u16 count; // Decremented once per handled node.
};

extern "C" Node *func_0202c710(Owner *self, Node *first, Node *node, s32 arg3);

// Recursive walk: the node returned by func_0202c710(self, first, node, 0) is
// handled first, then `node` itself, so the chain is processed back to front.
// `first` is only forwarded. For a node whose item has flag bit 0 set: pass
// the node's Link base to the owner's slot 0x28, decrement the owner's count,
// then call the item's slot 0x10 and (if non-null) slot 0x04.
extern "C" void func_01ff90f8(Owner *self, Node *first, Node *node) {
    Item *item;
    if (node == 0) {
        return;
    }
    func_01ff90f8(self, first, func_0202c710(self, first, node, 0));
    item = node->item;
    if (!(item->flags & 1)) {
        return;
    }
    self->unknown28(node);
    self->count--;
    item->unknown10();
    if (item != 0) {
        item->unknown04();
    }
}
