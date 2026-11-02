/* func_0204ff60: builds the Big result for self/obj (the self->f0 == 0 path of
 * func_0204fa60) and notifies the objects involved.
 *
 * Steps: fetch a placement from obj->f20 (+4, +0x14) via func_0204c5fc; clear a
 * 0x48-byte Hit block; when dir is non-zero, fill it from func_0204dbd8 (obj->f8
 * has both bits 0x80 and 0x100) or func_0204f1ec (only bit 0x100). The Hit block
 * is handed to func_0204e66c, which writes out. out goes to func_020502c8. If
 * the Hit has b or an object in c, obj's f10 callback gets a Msg (who = that
 * object; kind 0 when present, else 1 if out->c was set while self->f0 == 0,
 * else 2), and the object itself is called back with who = obj.
 */
#include "m2c.h"

typedef struct Obj Obj;

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} V3;

typedef struct {
    V3 a;
    s32 b;
    s32 c;
    s32 d;
    V3 e;
    s32 f;
} Big;

typedef struct {
    s32 s;
    V3 v;
} Pair;

typedef struct {
    V3 a;
    s32 b;
    union {
        s32 c;
        Obj *obj;
    } u;
    s32 d;
    Pair p[3];
} Hit;

typedef struct {
    Obj *who;
    u8 kind;
} Msg;

typedef struct {
    u8 pad[0x26];
    u8 f26;
} Node;

struct Obj {
    u8 pad0[0x8];
    u32 f8;
    u8 pad1[0x4];
    void (*f10)(Obj *, Msg *);
    u8 pad2[0xC];
    Node *f20;
};

typedef struct {
    V3 pos;
    V3 rot;
} Place;

typedef struct {
    u8 f0;
    u8 f1;
    u8 f2;
    u8 pad;
    s32 f4;
} Self;

extern Place *func_0204c5fc(Place *out, void *arg1, void *arg2);
extern void func_0204dbd8(Hit *, Obj *, Node *, Place *, V3 *, void *);
extern void func_0204f1ec(Hit *, Obj *, Place *, V3 *, void *);
extern void func_0204e66c(Big *, Obj *, Place *, void *, s32, s32, s32, Hit *);
extern void func_020502c8(void *arg0, void *arg1);

enum {
    OBJ_BIT_80  = 0x80,
    OBJ_BIT_100 = 0x100,
};

static inline void vec_set(V3 *d, const V3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

static inline void hit_set(Hit *d, const Hit *s) {
    vec_set(&d->a, &s->a);
    d->b      = s->b;
    d->u.c    = s->u.c;
    d->d      = s->d;
    d->p[0].s = s->p[0].s;
    vec_set(&d->p[0].v, &s->p[0].v);
    d->p[1].s = s->p[1].s;
    vec_set(&d->p[1].v, &s->p[1].v);
    d->p[2].s = s->p[2].s;
    vec_set(&d->p[2].v, &s->p[2].v);
}

void func_0204ff60(Big *out, Self *self, Obj *obj, V3 *dir, void *arg3, s32 a, s32 b, s32 c) {
    Msg msg;
    Place pos;
    Hit hit;
    s32 flag;
    Node *node;
    Pair *p;

    node = obj->f20;
    func_0204c5fc(&pos, (u8 *) node + 4, (u8 *) node + 0x14);
    hit.a.x = 0;
    hit.a.y = 0;
    hit.a.z = 0;
    hit.b   = 0;
    hit.u.c = 0;
    hit.d   = 0;
    p       = hit.p;
    do {
        p->s   = 0;
        p->v.x = 0;
        p->v.y = 0;
        p->v.z = 0;
        p++;
    } while (p < hit.p + 3);
    flag = 0;

    if (dir->x != 0 || dir->y != 0 || dir->z != 0) {
        if ((obj->f8 & OBJ_BIT_80) == OBJ_BIT_80 && (obj->f8 & OBJ_BIT_100) == OBJ_BIT_100) {
            Hit t;
            func_0204dbd8(&t, obj, node, &pos, dir, arg3);
            hit_set(&hit, &t);
        } else if ((obj->f8 & OBJ_BIT_100) == OBJ_BIT_100) {
            Hit t;
            func_0204f1ec(&t, obj, &pos, dir, arg3);
            hit_set(&hit, &t);
        }
    }

    func_0204e66c(out, obj, &pos, arg3, a, b, c, &hit);
    if (out->c != 0 && self->f0 == 0) {
        flag = 1;
    }
    func_020502c8(self, out);

    if (hit.b != 0 || hit.u.c != 0 || hit.u.obj != 0) { // FAKE: re-tests the same word as a pointer; the original reloads it
        msg.who  = hit.u.obj;
        msg.kind = 0;
        if (msg.who == 0) {
            if (flag) {
                msg.kind = 1;
            } else {
                msg.kind = 2;
            }
        }
        obj->f10(obj, &msg);
        if (hit.u.obj != 0) {
            msg.who = obj;
            hit.u.obj->f10(hit.u.obj, &msg);
        }
    }
}
