#include "m2c.h"

struct Node;

// Partial interface: the vtables live in ROM data; only slot 0x0c is called.
class NodeIface {
public:
    virtual void unknown00();
    virtual void unknown04();
    virtual void unknown08();
    virtual void load(u16 *src); // Slot 0x0c.
};

struct NodeVtbl {
    void *slot[4];
};

struct Node {
    const NodeVtbl *vt;
    s32 type;
    s32 unk8;
    s32 unkC;
    s32 unk10;
    s32 unk14;
    s32 unk18;
    s32 unk1C;
    s32 unk20;
    s32 unk24;
    s32 unk28;
};

extern "C" Node *func_0205e80c(s32 size, s32 heap, s32 flags);
extern "C" void func_020bdfa8(void *arg0);

extern "C" const NodeVtbl data_021011a4;
extern "C" const NodeVtbl data_021011bc;
extern "C" const NodeVtbl data_021011ec;
extern "C" const NodeVtbl data_02101204;
extern "C" const NodeVtbl data_0210121c;
extern "C" const NodeVtbl data_02101234;
extern "C" const NodeVtbl data_0210124c;
extern "C" const NodeVtbl data_02101264;
extern "C" const NodeVtbl data_0210127c;
extern "C" const NodeVtbl data_02101294;
extern "C" const NodeVtbl data_021012ac;

// Factory: reads the u16 type tag (0..10) at the start of a record, allocates
// the matching object (size per type, func_0205e80c(size, 3, 4)), runs the
// shared base initialiser func_020bdfa8, installs that type's vtable (types 8
// and 9 install a base vtable first, then the derived one) and clears its
// fields. Then it stores the tag at +0x04 and passes the record to virtual
// slot 0x0c. The result is not null-checked before that call.
extern "C" Node *func_020be37c(u16 *src) {
    u16 type = *src;
    Node *node;

    switch (type) {
        case 0:
            node = func_0205e80c(0x18, 3, 4);
            if (node != (Node *) 0) {
                func_020bdfa8(node);
                node->vt    = &data_021011a4;
                node->unk14 = 0;
            }
            break;
        case 1:
            node = func_0205e80c(0x18, 3, 4);
            if (node != (Node *) 0) {
                func_020bdfa8(node);
                node->vt    = &data_021012ac;
                node->unk14 = 0;
            }
            break;
        case 2:
            node = func_0205e80c(0x18, 3, 4);
            if (node != (Node *) 0) {
                func_020bdfa8(node);
                node->vt    = &data_02101264;
                node->unk14 = 0;
            }
            break;
        case 3:
            node = func_0205e80c(0x24, 3, 4);
            if (node != (Node *) 0) {
                func_020bdfa8(node);
                node->vt    = &data_02101204;
                node->unk14 = 0;
                node->unk18 = 0;
                node->unk1C = 0;
                node->unk20 = 0;
            }
            break;
        case 4:
            node = func_0205e80c(0x18, 3, 4);
            if (node != (Node *) 0) {
                func_020bdfa8(node);
                node->vt    = &data_021011ec;
                node->unk14 = 0;
            }
            break;
        case 5:
            node = func_0205e80c(0x24, 3, 4);
            if (node != (Node *) 0) {
                func_020bdfa8(node);
                node->vt    = &data_021011bc;
                node->unk14 = 0;
                node->unk18 = 0;
                node->unk1C = 0;
                node->unk20 = 0;
            }
            break;
        case 6:
            node = func_0205e80c(0x2c, 3, 4);
            if (node != (Node *) 0) {
                func_020bdfa8(node);
                node->vt    = &data_0210121c;
                node->unk20 = 0;
                node->unk24 = 0;
                node->unk28 = 0;
            }
            break;
        case 7:
            node = func_0205e80c(0x28, 3, 4);
            if (node != (Node *) 0) {
                func_020bdfa8(node);
                node->vt    = &data_02101234;
                node->unk14 = 0;
                node->unk18 = 0;
                node->unk1C = 0;
                node->unk20 = 0;
            }
            break;
        case 8:
            node = func_0205e80c(0x2c, 3, 4);
            if (node != (Node *) 0) {
                func_020bdfa8(node);
                node->vt    = &data_02101234;
                node->unk14 = 0;
                node->unk18 = 0;
                node->unk1C = 0;
                node->unk20 = 0;
                node->vt    = &data_0210124c;
            }
            break;
        case 9:
            node = func_0205e80c(0x1c, 3, 4);
            if (node != (Node *) 0) {
                func_020bdfa8(node);
                node->vt    = &data_021012ac;
                node->unk14 = 0;
                node->vt    = &data_02101294;
                node->unk18 = 0;
            }
            break;
        case 10:
            node = func_0205e80c(0x18, 3, 4);
            if (node != (Node *) 0) {
                func_020bdfa8(node);
                node->vt    = &data_0210127c;
                node->unk14 = 0;
            }
            break;
    }
    node->type = type;
    ((NodeIface *) node)->load(src);
    return node;
}
