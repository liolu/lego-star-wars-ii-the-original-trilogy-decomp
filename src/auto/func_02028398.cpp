#include "m2c.h"

// Partial interface: unknown virtuals retain their observed vtable slots.
// Declarations only: the original vtables remain in the ROM data.
class Entity {
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
    virtual void action(int, int, int, int); // Slot 0x24.
    virtual void unknown28();
    virtual void interact(Entity *, int, int); // Slot 0x2c.
    virtual void unknown30();
    virtual void unknown34();
    virtual void unknown38();
    virtual void unknown3c();
    virtual void unknown40();
    virtual int query(); // Slot 0x44.
};

extern "C" void func_02029428(void *);
extern "C" void func_02029488(void *, s32);

struct Node {
    u32 *f0;
    u32 *f4;
    s32 f8;
    u32 fC;
    u8 pad0[0x180 - 0x10];
    u16 f180;
    u8 pad2[0x5D6 - 0x182];
    u8 f5D6;
};

extern "C" void func_02028398(Node *arg0, Entity *arg1, Node *arg2) {
    u32 *link;
    u32 *cur;
    u32 *var_r2;
    Node *base;

    if ((arg2 == 0) || ((arg2->f8 & 0x800) == 0x800)) {
        link = arg0->f0;
        if (link != 0) {
            var_r2 = 0;
            cur    = (u32 *) link[0];
            if (cur != (u32 *) arg0) {
                do {
                    link = cur + 1;
                    cur  = (u32 *) cur[1];
                } while (cur != (u32 *) arg0);
            }
            link[0] = (u32) arg0->f4;
        } else {
            var_r2 = 0;
        }
        if (var_r2 != 0) {
            arg0->f0  = var_r2;
            arg0->f4  = (u32 *) var_r2[0];
            var_r2[0] = (u32) arg0;
        } else {
            arg0->f0 = 0;
            arg0->f4 = 0;
        }
        return;
    }
    var_r2 = (u32 *) arg2;
    if (var_r2 != 0) {
        var_r2 = (u32 *) ((u8 *) var_r2 + 4);
    }
    link = arg0->f0;
    if (link != 0) {
        cur = (u32 *) link[0];
        if (cur != (u32 *) arg0) {
            do {
                link = cur + 1;
                cur  = (u32 *) cur[1];
            } while (cur != (u32 *) arg0);
        }
        link[0] = (u32) arg0->f4;
    }
    if (var_r2 != 0) {
        arg0->f0  = var_r2;
        arg0->f4  = (u32 *) var_r2[0];
        var_r2[0] = (u32) arg0;
    } else {
        arg0->f0 = 0;
        arg0->f4 = 0;
    }
    base = (Node *) arg0->f0;
    if (arg0->f0 != 0) {
        base = (Node *) ((u8 *) arg0->f0 - 4);
    }
    func_02029428(base);
    base = (Node *) arg0->f0;
    if (arg0->f0 != 0) {
        base = (Node *) ((u8 *) arg0->f0 - 4);
    }
    base->f5D6 = 1;
    base       = (Node *) arg0->f0;
    if (arg0->f0 != 0) {
        base = (Node *) ((u8 *) arg0->f0 - 4);
    }
    ((Entity *) base)->action(25, 0, 1, 0);
    {
        s32 v = arg1 ? arg1->query() : 1;
        base  = (Node *) arg0->f0;
        if (arg0->f0 != 0) {
            base = (Node *) ((u8 *) arg0->f0 - 4);
        }
        ((Entity *) base)->interact(arg1, v, 0);
    }
    base = (Node *) arg0->f0;
    if (arg0->f0 != 0) {
        base = (Node *) ((u8 *) arg0->f0 - 4);
    }
    func_02029488(base, 6);
    base = (Node *) arg0->f0;
    if (arg0->f0 != 0) {
        base = (Node *) ((u8 *) arg0->f0 - 4);
    }
    arg0->fC = base->f180;
    arg0->f8 = 2;
}
