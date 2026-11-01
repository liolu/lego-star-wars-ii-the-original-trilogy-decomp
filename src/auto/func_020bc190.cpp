#include "m2c.h"

typedef struct Node {
    struct Ent *owner;
    struct Node *next;
    struct Node *prev;
} Node;

class Res {
public:
    virtual void m0();
    virtual void m1();
    virtual void m2();
    s32 pad[2];
    s32 refs;
    s32 users;
    u8 flag;
};

typedef struct Ent {
    u8 pad0[0x48];
    s32 cleared;
    u8 pad1[4];
    Res *res;
    u8 pad2[0x3c];
    struct Ent *next;
    struct Ent *prev;
} Ent;

typedef struct Obj {
    u8 pad0[0x48];
    s32 res48;
    s32 res4c[6];
    Res *res64;
    u8 pad2[0x30c];
    s32 count;
    Node *head;
    Node *sentinel;
    Node *free;
} Obj;

typedef struct Glob {
    u8 pad[0x4f8];
    s32 count;
    Node *head;
    Node *sentinel;
    Node *free;
} Glob;

extern "C" {
void func_0205e84c(s32);
void func_020c099c(void **);
extern Glob data_0210e1ec;
extern Ent *data_02111790[];
extern Ent *data_02111798[];
extern s32 data_0211178c[];
}

static inline void node_release(Obj *o, Node *m) {
    if (m->prev != 0) {
        m->prev->next = m->next;
    }
    if (m->next != 0) {
        m->next->prev = m->prev;
    }
    if (m == o->head) {
        o->head = m->next;
    }
    m->next = o->free;
    o->free = m;
    o->count--;
}

enum {
    HANDLE_SLOTS = 6,
};

/*
 * Teardown for an Obj (callers store a vtable pointer at obj+0 and then call
 * this; it returns self, destructor-style). Steps:
 *  1. release the handle at +0x48 and the leading non-zero handles of the
 *     six-entry array at +0x4c through func_0205e84c, zeroing each slot;
 *  2. find the node owned by self in the global node list (data_0210e1ec),
 *     unlink it and push it onto that list's free chain;
 *  3. drain self's own node list: for each node, clear the owner's +0x48
 *     word, drop one ref on the owner's Res (virtual slot 2 when refs reach 0
 *     and flag is clear; func_020c099c when refs and users are both 0), move
 *     the owner Ent from the data_02111790 list to the data_02111798 free
 *     chain (count data_0211178c), then recycle the node;
 *  4. drop one user on the Res at +0x64, freeing it when unused.
 */
extern "C" Obj *func_020bc190(Obj *self) {
    Node *m;
    Ent *e;
    Res *r;
    Ent **ph;
    Ent **pf;
    s32 *pc;
    s32 i;
    Node *n;
    Res *r64;

    if (self->res48 != 0) {
        func_0205e84c(self->res48);
        self->res48 = 0;
    }
    i = 0;
    do {
        if (self->res4c[i] == 0) {
            break;
        }
        func_0205e84c(self->res4c[i]);
        self->res4c[i] = 0;
        i++;
    } while (i < HANDLE_SLOTS);

    n = data_0210e1ec.count != 0 ? data_0210e1ec.head : data_0210e1ec.sentinel;
    while (n != data_0210e1ec.sentinel) {
        if ((Ent *) self == n->owner) {
            if (n->prev != 0) {
                n->prev->next = n->next;
            }
            if (n->next != 0) {
                n->next->prev = n->prev;
            }
            if (n == data_0210e1ec.head) {
                data_0210e1ec.head = n->next;
            }
            n->next            = data_0210e1ec.free;
            data_0210e1ec.free = n;
            data_0210e1ec.count--;
            break;
        }
        n = n->next;
    }

    if (self->count != 0) {
        ph = data_02111790;
        pf = data_02111798;
        pc = data_0211178c;
        do {
            m          = self->count != 0 ? self->head : self->sentinel;
            e          = m->owner;
            e->cleared = 0;
            r          = m->owner->res;
            r->refs--;
            if (r->refs == 0 && r->flag == 0) {
                r->m2();
            }
            if (r->refs == 0 && r->users == 0) {
                func_020c099c((void **) r);
            }
            e = m->owner;
            if (e->prev != 0) {
                e->prev->next = e->next;
            }
            if (e->next != 0) {
                e->next->prev = e->prev;
            }
            if (e == *ph) {
                *ph = e->next;
            }
            e->next = *pf;
            *pf     = e;
            (*pc)--;
            node_release(self, m);
        } while (self->count != 0);
    }

    r64 = self->res64;
    if (r64 != 0) {
        r64->users--;
        if (r64->refs == 0 && r64->users == 0) {
            func_020c099c((void **) r64);
        }
    }
    return self;
}
