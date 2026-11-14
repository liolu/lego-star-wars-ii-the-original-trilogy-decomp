#include "m2c.h"

/* One halfword structure passed by value (see src/auto/func_020534f4.c). */
typedef struct {
    u16 v;
} HalfWord;

typedef struct {
    u8 a;
    u8 b;
    u8 c;
    u8 d;
} Pair8;

typedef struct Node {
    struct Node *first;
    struct Node *second;
} Node;

typedef struct {
    void *vtbl;
    s32 f04;
    s32 pair[2];
    s32 idx;
    u8 pad14[0x2c];
    s32 f40;
    s32 f44;
    s32 f48;
    Pair8 pairs[2];
    u8 buf[0x10];
    s32 f64;
    s32 f68;
    Node node;
    s32 f74;
    s32 f78;
    s32 f7c;
    u8 f80;
    u8 f81;
    u8 f82;
    u8 f83;
    u8 f84;
    u8 f85;
    u8 pad86[3];
    u8 f89;
    u8 f8a;
    u8 f8b;
    u8 f8c;
    u8 f8d;
    u8 f8e;
} Obj;

extern void *func_0206d0d8(void *, s32);
extern void func_020534e0(void *);
extern void func_0206549c(void *);
extern void func_02053f44(void *, s32);
extern void func_020534f4(void *, s32, s32, HalfWord);
extern void func_020536a8(void *, s32);
extern void *func_0205e624(void *, u32, u32);
extern void *data_020ff454;
extern u8 data_02101b78[];

/* Stores owner in link->first; a non-NULL owner also gets link pushed onto its
 * first chain (same node idiom as func_0202ceec). */
static inline void link_set(Node *link, void *owner) {
    Node *o = owner; // FAKE: void * param copied to a Node * local so the NULL test is not constant-folded

    if (o != NULL) { // FAKE: the only caller passes NULL; keeps the target's cmp #0 / block split
        link->first  = o;
        link->second = o->first;
        o->first     = link;
    } else {
        link->first  = o; // FAKE: first stored in both arms so it is hoisted and kept as add r1,#0x6c; str [r1]
        link->second = NULL;
    }
}

/* Base init (func_0206d0d8), vtable data_020ff454, then member defaults; returns self. */
Obj *func_0208a938(Obj *self) {
    u32 i;
    Node *n;

    func_0206d0d8(self, 0);
    self->vtbl = &data_020ff454;
    func_020534e0(self->pair);
    func_0206549c(&self->idx);
    self->f40 = 0;
    self->f44 = 0;
    self->f48 = 0;
    for (i = 0; i < 2; i++) {
        self->pairs[i].a = 0;
        self->pairs[i].b = 0;
    }
    n = &self->node; // FAKE: member address through a local; link_set(&self->node, NULL) folds every store onto r4
    link_set(n, NULL);
    self->f7c = 0x80000000;
    {
        HalfWord h = {0};
        func_02053f44(data_02101b78, 0x1000);
        func_020534f4(self->pair, 0, 1, h);
    }
    func_020536a8(self->pair, 0);
    self->f80 = 0;
    self->f85 = 3;
    self->f78 = 20;
    self->f81 = 2;
    self->f82 = 4;
    self->f83 = 0;
    self->f84 = 1;
    self->f8b = 0xff;
    self->f89 = 0xff;
    self->f8a = 4;
    self->f8e = 0xff;
    self->f8c = 0xff;
    self->f8d = 4;
    self->f74 = 0;
    func_0205e624(self->buf, 0, 0x10);
    self->f64        = 0;
    self->f68        = 0;
    self->pairs[0].c = 0x80;
    self->pairs[0].d = 0x94;
    self->pairs[1].c = 8;
    self->pairs[1].d = 0xa0;
    return self;
}
