#include "m2c.h"

typedef struct Owner {
    u8 pad0[4];
    u8 *link4;      /* points 4 bytes past the start of the first list node */
    u8 pad8[0x10];
    void *child18;  /* child object; func_0202c218 calls its vtable slot 3 */
} Owner;

enum {
    NODE_LINK_OFFSET = 4,  /* offset of the link field inside a list node */
};

void func_01ff90f8(Owner *self, void *node, void *node2);
void func_0202c698(Owner *self, void *child, int flag);

/* If the owner's list at +4 is non-empty, passes its first node to
 * func_01ff90f8. If the list is empty afterwards and a child exists at
 * +0x18, calls func_0202c698 on that child with flag 1. */
void func_01ff90a8(Owner *self) {
    void *node = self->link4;
    if (node != NULL) {
        node = (u8 *) node - NODE_LINK_OFFSET;
    }
    if (node == NULL) {
        return;
    }
    func_01ff90f8(self, node, node);
    if (self->link4 == NULL && self->child18 != NULL) {
        func_0202c698(self, self->child18, 1);
    }
}
