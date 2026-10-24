#include "m2c.h"

// Coordinator seed (class/slot design only; the worker iterates on statement shape).
// Layout follows the verified sibling src/auto/func_02061194.cpp: sub-object at 0x38 with its
// own vtable (declaration only; the ROM keeps the real vtable) and a list at 0x3c.

class EntryA {
public:
    s32 pad0[2];
    s32 key2; // 0x08
    u16 key1; // 0x0c
};

class EntryB {
public:
    s32 key1; // 0x00
    s32 key2; // 0x04
};

class LinkA {
public:
    EntryA *data; // 0x00
    LinkA *next;  // 0x04
    LinkA *prev;  // 0x08
};

class LinkB {
public:
    EntryB *data; // 0x00
    LinkB *next;  // 0x04
    LinkB *prev;  // 0x08
};

class Sink {
public:
    virtual void unknown00();
    virtual void unknown04();
    virtual void unknown08();
    virtual void handle(EntryB *); // Slot 0xc.
};

class Owner {
public:
    virtual void unknown00();
    LinkA *listA;  // 0x04
    u8 pad[0x30];  // 0x08
    Sink sink;     // 0x38
    LinkB *listB;  // 0x3c
};

extern "C" void func_02062048(void *);
extern "C" void func_0205e84c(void *);
extern "C" void func_0205ab6c(u16);

extern "C" void func_020612d4(Owner *arg0, s32 arg1, s32 arg2) {
    LinkA *a;
    EntryB *e;
    LinkB *next;
    LinkB *b;
    s32 removed;
    s32 found;

    a = arg0->listA;
    while (a != NULL) {
        s32 removed = 0;
        EntryA *e   = a->data;
        if (e->key1 == arg1 && e->key2 == arg2) {
            LinkA *next = a->prev;
            if (next == NULL) {
                next    = a->next;
                removed = 1;
            }
            if (e != NULL) {
                func_02062048(e);
                func_0205e84c(e);
            }
            a = next;
        }
        if (a != NULL && removed == 0) {
            a = a->next;
        }
    }

    found = 0;
    b     = arg0->listB;
    while (b != NULL) {
        e       = b->data;
        removed = 0;
        if (e->key1 == arg1 && e->key2 == arg2 && found == 0) {
            func_0205ab6c((u16) e->key1);
            next = b->prev;
            if (next == NULL) {
                next    = b->next;
                removed = 1;
            }
            arg0->sink.handle(b->data);
            func_0205e84c(e);
            found = 1;
            b     = next;
        }
        if (b != NULL && removed == 0) {
            b = b->next;
        }
    }
}
