/* Reconstructed from USA ARM9 at 0x0204ec14 (1024 bytes).
 *
 * Clears a Hit result, computes v through func_0204da60 (plus the vector at
 * +0x78 of the object at obj->f54 - 4 when obj->f40 and obj->f41 are set) and
 * fills t1 through func_0204dbd8, which is copied into hit. When hit.b or
 * hit.c is set and a8 is non-zero, nb is built by func_0204c74c from box and
 * (0, a8, 0), node->f8 is raised by a8 and t2 is filled from nb. t2 replaces
 * hit (with y raised by a8) when func_020111c4 gives a larger value for t2.a
 * than for hit.a and func_01ff9188 returns b >= 0x2000; node->f8 is then
 * restored. Finally a7 gets the positive part of hit.a.y plus a8 and the
 * result is passed on to func_0204e66c.
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

typedef struct {
    s32 s;
    V3 v;
} Pair;

typedef struct {
    V3 a;
    s32 b;
    s32 c;
    s32 d;
    Pair p[3];
} Hit;

typedef struct {
    s32 min[3];
    s32 max[3];
} Box;

typedef struct {
    s32 a;
    s32 b;
} Res;

typedef struct {
    s32 v;
} Fx;

typedef struct {
    u8 pad[0x8];
    s32 f8;
    u8 pad2[0x1A];
    u8 f26;
} Node;

typedef struct {
    u8 pad[0x20];
    Node *f20;
    u8 pad2[0x1C];
    u8 f40;
    u8 f41;
    u8 pad3[0x12];
    u8 *f54;
} Obj;

enum {
    RES_B_MIN = 0x2000
};

extern void func_0204da60(V3 *, V3 *, s32 *);
extern void func_0204dbd8(Hit *, Obj *, Node *, Box *, V3 *, void *);
extern void func_0204c74c(Box *, V3 *, Box *);
extern Res func_01ff9188(void *, Box *, V3 *);
extern void func_020111c4(Fx *, const V3 *);
extern void func_0204e66c(Big *, Obj *, Box *, void *, s32, Fx, s32, Hit *);

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
    d->c      = s->c;
    d->d      = s->d;
    d->p[0].s = s->p[0].s;
    vec_set(&d->p[0].v, &s->p[0].v);
    d->p[1].s = s->p[1].s;
    vec_set(&d->p[1].v, &s->p[1].v);
    d->p[2].s = s->p[2].s;
    vec_set(&d->p[2].v, &s->p[2].v);
}

// FAKE: redundant pointer copy plus a null test that is always true at the call site;
// it keeps &l2 in a register after the call (add r1,sp,#0x14; ldr r1,[r1]).
static inline s32 fx_val(const Fx *p) {
    const Fx *q = p;
    s32 v       = 0;
    if (q) {
        v = q->v;
    }
    return v;
}

static inline s32 has_link(Obj *obj) {
    s32 flag = 0;
    u8 *p;
    if (obj->f40 != 0 && obj->f41 != 0) {
        p = obj->f54;
        if (p) {
            p -= 4;
        }
        if (p) {
            flag = 1;
        }
    }
    return flag;
}

void func_0204ec14(Big *out, Obj *obj, Box *box, V3 *dir, void *a4, s32 *a5, s32 a6, Fx a7, Fx a8, s32 a9) {
    Hit hit;
    Hit t2;
    Hit t1;
    V3 v;
    V3 off;
    Box nb;
    Res r;
    Fx l2;
    Fx l1;
    Node *node;
    Pair *p;
    s32 *q;
    u8 *lp;

    hit.a.x = 0;
    hit.a.y = 0;
    hit.a.z = 0;
    hit.b   = 0;
    hit.c   = 0;
    hit.d   = 0;
    p       = hit.p;
    do {
        p->s   = 0;
        p->v.x = 0;
        p->v.y = 0;
        p->v.z = 0;
        p++;
    } while (p < hit.p + 3);

    node = obj->f20;
    func_0204da60(&v, dir, a5);
    if (has_link(obj)) {
        lp = obj->f54;
        if (lp != 0) {
            lp -= 4;
        }
        v.x += M2C_FIELD(lp, s32 *, 0x78);
        v.y += M2C_FIELD(lp, s32 *, 0x7c);
        v.z += M2C_FIELD(lp, s32 *, 0x80);
    }

    func_0204dbd8(&t1, obj, node, box, &v, a4);
    hit_set(&hit, &t1);

    if ((hit.b != 0 || hit.c != 0) && a8.v != 0) {
        off.y = a8.v;
        off.x = 0;
        off.z = 0;
        q     = nb.min;
        do {
            *q++ = 0;
        } while (q < (s32 *) (&nb + 1));
        func_0204c74c(box, &off, &nb);
        if (node != 0) {
            node->f8 += a8.v;
        }
        func_0204dbd8(&t2, obj, node, &nb, dir, a4);
        func_020111c4(&l1, &hit.a);
        func_020111c4(&l2, &t2.a);
        if (fx_val(&l2) > l1.v) {
            r = func_01ff9188(a4, box, &off);
            if (r.b >= RES_B_MIN) {
                hit_set(&hit, &t2);
                hit.a.y += a8.v;
            }
        }
        if (node != 0) {
            node->f8 -= a8.v;
        }
    }

    if (hit.a.y > 0) {
        a7.v += hit.a.y;
    }
    a7.v += a8.v;
    func_0204e66c(out, obj, box, a4, a6, a7, a9, &hit);
}
