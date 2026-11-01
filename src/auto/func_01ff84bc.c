#include "m2c.h"

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct {
    void *vptr;
    Vec3 pos;
    s32 f10;
    Vec3 a;
    s32 sum;
    s8 f24;
    s8 kind;
    s8 f26;
    s8 f27;
    s32 b0;
    s32 b1;
    s32 b2;
} Shape;

typedef struct {
    s32 v;
} WordArg;

extern M2C_UNK data_020fcd18;
extern M2C_UNK data_020fe438;

extern s32 func_0200aef8(void *, void *, WordArg);
void func_0200af88(s8 *arg0, s8 arg1);
void func_0204c594(Shape *o, Vec3 *src);
s32 func_01ff9d4c(Shape *a, Shape *b);
s32 func_01ff9e38(Shape *box, Shape *s);
s32 func_0204d3bc(Shape *a, Shape *b);
s32 func_0204d6d4(Shape *a, Shape *b);
s32 func_0204d784(Shape *a, Shape *b);
s32 func_0204d904(Shape *a, Shape *b);

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

static inline void make_box(Shape *o, Shape *cyl, Vec3 *hp) {
    o->vptr  = &data_020fcd18;
    o->pos.x = 0;
    o->pos.y = 0;
    o->pos.z = 0;
    o->f10   = 0;
    o->a.x   = 0;
    o->a.y   = 0;
    o->a.z   = 0;
    o->sum   = 0;
    func_0200af88(&o->f24, -1);
    o->f26  = 0;
    o->vptr = &data_020fe438;
    o->b0   = 0;
    o->b1   = 0;
    o->b2   = 0;
    o->kind = 5;
    vec_set(&o->pos, &cyl->pos);
    {
        s32 r = cyl->b1;
        s32 y = cyl->b0;
        hp->x = r;
        hp->y = y;
        hp->z = r;
    }
    func_0204c594(o, hp);
}

s32 func_01ff84bc(Shape *a, Shape *b) {
    WordArg r;
    r.v = a->sum + b->sum;
    if (func_0200aef8(&a->pos, &b->pos, r) != 0) {
        switch (a->kind) {
            case 0:
                switch (b->kind) {
                    case 0:
                        return 1;
                    case 3:
                        return func_0204d3bc(a, b);
                    case 5:
                        return func_01ff9e38(b, a);
                    case 11:
                        return func_0204d784(b, a);
                }
                break;
            case 3:
                switch (b->kind) {
                    case 0:
                        return func_0204d3bc(b, a);
                    case 3:
                        return func_0204d6d4(a, b);
                    case 5: {
                        Shape o;
                        Vec3 h;
                        make_box(&o, a, &h);
                        return func_01ff9d4c(&o, b);
                    }
                    case 11:
                        return func_0204d904(b, a);
                }
                break;
            case 5:
                switch (b->kind) {
                    case 0:
                        return func_01ff9e38(a, b);
                    case 3: {
                        Shape o;
                        Vec3 h;
                        make_box(&o, b, &h);
                        return func_01ff9d4c(a, &o);
                    }
                    case 5:
                        return func_01ff9d4c(a, b);
                    case 11:
                        return 0;
                }
                break;
            case 11:
                switch (b->kind) {
                    case 0:
                        return func_0204d784(a, b);
                    case 3:
                        return func_0204d904(a, b);
                    case 5:
                        return 0;
                    case 11:
                        break;
                }
                break;
        }
    }
    return 0;
}
