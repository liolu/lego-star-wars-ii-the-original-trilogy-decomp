/* func_0204fa60: computes a Big result for self/obj and copies its vector out.
 *
 * Steps: clear the result; if self->f0 is zero call func_0204ff60, else fetch
 * a 24-byte placement from obj->f20 and pick func_0204ec14 / func_0204f014
 * (obj->f8 bits 0x100 and 0x80) when dir is non-zero or obj has a link, or
 * func_0204e66c with a zeroed scratch block otherwise. The per-kind table
 * entries are indexed by self->f2. The result is passed to func_020502c8,
 * then obj (and the objects in res.f / tagged res.c) are notified through
 * their f10 callbacks, and res.a is copied to out.
 */
#include "m2c.h"

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

typedef struct Obj Obj;

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
    u8 pad3[0x1C];
    u8 f40;
    u8 f41;
    u8 pad4[0x12];
    u8 *f54;
};

typedef struct {
    V3 pos;
    V3 rot;
} Place;

typedef struct {
    s32 a, b, c, d;
} Q;

typedef struct {
    s32 a[6];
    Q b[3];
} Z;

typedef struct {
    u8 f0;
    u8 f1;
    u8 f2;
    u8 pad;
    s32 f4;
} Self;

typedef struct {
    s32 v;
    s32 pad[3];
} Row;

extern Row data_020ff820[];
extern Row data_020ff824[];
extern Row data_020ff828[];
extern Row data_020ff82c[];

extern Place *func_0204c5fc(Place *out, void *arg1, void *arg2);
extern void func_0204e66c(Big *, Obj *, Place *, void *, s32, s32, s32, Z *);
extern void func_0204ec14(Big *, Obj *, Place *, V3 *, void *, s32 *, s32, s32, s32, s32);
extern void func_0204f014(Big *, Obj *, Place *, V3 *, void *, s32 *, s32, s32, s32, s32);
extern void func_0204ff60(Big *, Self *, Obj *, V3 *, void *, s32, s32, s32);
extern void func_020502c8(void *arg0, void *arg1);

static inline void vec_set(V3 *d, const V3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

static inline s32 has_link(Obj *obj) {
    s32 flag = 0;
    u8 *p;
    if (obj->f40 != 0 && obj->f41 != 0) {
        p = obj->f54;
        if (p != 0) {
            p -= 4;
        }
        if (p != 0) {
            flag = 1;
        }
    }
    return flag;
}

static inline void msg_init(Msg *m, const Big *r) {
    m->who  = (Obj *) r->f;
    m->kind = 0;
    if (m->who == 0 && r->b != r->c) {
        m->kind = 2;
    }
}

static inline void big_set(Big *d, const Big *s) {
    vec_set(&d->a, &s->a);
    d->b = s->b;
    d->c = s->c;
    d->d = s->d;
    vec_set(&d->e, &s->e);
    d->f = s->f;
}

enum {
    TAG_OBJ = 0x80000000
};

void func_0204fa60(Self *self, Obj *obj, V3 *dir, void *arg3, V3 *out) {
    Big res;
    Place pos;
    Msg msg;
    u8 i;

    res.a.x = 0;
    res.a.y = 0;
    res.a.z = 0;
    res.b   = 0;
    res.c   = 0;
    res.d   = 0;
    res.e.x = 0;
    res.e.y = 0;
    res.e.z = 0;
    res.f   = 0;

    if (self->f0 == 0) {
        i = self->f2;
        {
            Big t;
            func_0204ff60(&t, self, obj, dir, arg3, data_020ff824[i].v, data_020ff820[i].v, data_020ff82c[i].v);
            big_set(&res, &t);
        }
    } else {
        func_0204c5fc(&pos, (u8 *) obj->f20 + 4, (u8 *) obj->f20 + 0x14);
        if ((dir->x != 0 || dir->y != 0 || dir->z != 0) || has_link(obj)) {
            if ((obj->f8 & 0x100) == 0x100) {
                if ((obj->f8 & 0x80) == 0x80) {
                    Big t;
                    func_0204ec14(&t, obj, &pos, dir, arg3, &self->f4, data_020ff824[self->f2].v, data_020ff820[self->f2].v,
                                  data_020ff828[self->f2].v, data_020ff82c[self->f2].v);
                    big_set(&res, &t);
                } else {
                    Big t;
                    func_0204f014(&t, obj, &pos, dir, arg3, &self->f4, data_020ff824[self->f2].v, data_020ff820[self->f2].v,
                                  data_020ff828[self->f2].v, data_020ff82c[self->f2].v);
                    big_set(&res, &t);
                }
            }
        } else {
            Z z;
            Q *q;
            z.a[0] = 0;
            z.a[1] = 0;
            z.a[2] = 0;
            z.a[3] = 0;
            z.a[4] = 0;
            z.a[5] = 0;
            q      = z.b;
            do {
                q->a = 0;
                q->b = 0;
                q->c = 0;
                q->d = 0;
                q++;
            } while (q < z.b + 3);
            i = self->f2;
            {
                Big t;
                func_0204e66c(&t, obj, &pos, arg3, data_020ff824[i].v, data_020ff820[i].v, data_020ff82c[i].v, &z);
                big_set(&res, &t);
            }
        }
    }

    func_020502c8(self, &res);

    if (res.b != 0 || res.f != 0) {
        msg_init(&msg, &res);
        obj->f10(obj, &msg);
        if (res.f != 0) {
            msg.who = obj;
            ((Obj *) res.f)->f10((Obj *) res.f, &msg);
        }
    }

    if (res.c & TAG_OBJ) {
        Obj *o = (Obj *) (res.c & ~TAG_OBJ);
        Msg m2;
        m2.kind = 1;
        m2.who  = o;
        obj->f10(obj, &m2);
        m2.who = obj;
        o->f10(o, &m2);
    }

    vec_set(out, &res.a);
}
