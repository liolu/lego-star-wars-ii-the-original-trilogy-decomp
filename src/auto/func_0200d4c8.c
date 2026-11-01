#include "m2c.h"

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct {
    void *vptr;
    s32 p0;
    s32 p1;
    s32 p2;
    s32 f10;
    s32 a0;
    s32 a1;
    s32 a2;
    s32 sum;
    u8 f24[4];
    s32 b0;
    s32 b1;
    s32 b2;
} Obj;

typedef struct {
    s32 f0;
    s32 x;
    s32 y;
    s32 z;
    s32 f10;
    u8 pad[4];
    u8 f18[1];
} Src;

typedef struct {
    s32 xmin, xmax;
    s32 ymin, ymax;
    s32 zmin, zmax;
} Box;

typedef struct {
    s32 a;
    s32 b;
} Res;

extern M2C_UNK data_020fcd18;
extern M2C_UNK data_020fe438;
typedef struct {
    u8 pad[0x384];
    s32 v;
} Inner;
typedef struct {
    u8 pad[0x68];
    Inner *in;
} Glob;
extern Glob data_0210930c;

void func_02010af0(void *);
void func_0200d6d0(Vec3 *, void *, s32);
void func_0200af88(s8 *arg0, s8 arg1);
void func_0204c594(Obj *o, void *src);
void func_0204c6e8(Box *, Obj *);
Res func_01ff9188(s32, Box *, Vec3 *);
void func_0200cef4(void *);
extern Vec3 data_020ff9b4;

static inline void vec_sub(Vec3 *out, const Vec3 *a, const Vec3 *b) {
    s32 z  = a->z - b->z;
    s32 y  = a->y - b->y;
    s32 x  = a->x - b->x;
    out->x = x;
    out->y = y;
    out->z = z;
}

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

/* One halfword wrapped in a struct; the field at +0x1d8 has this type. */
typedef struct {
    u16 v;
} HV;
typedef struct {
    u8 pad0[8];
    u32 flags;
    u8 pad1[0x54];
    Vec3 v60;
    u8 pad2[0x44];
    Vec3 vb0;
    u8 pad3[0x1c2 - 0xbc];
    u8 f1c2;
    u8 pad4[5];
    s32 f1c8;
    Vec3 v1cc;
    u8 pad5[0x1d8 - 0x1d8];
    HV f1d8;
} Ent;

enum {
    FLAG_20     = 0x20,   /* bit of Ent.flags cleared on both update paths */
    SHAPE_F25   = 5,      /* byte stored at shape+0x25 after the second vptr */
    RES_B_LIMIT = 0x2000, /* func_01ff9188 result word 1 at or above this: no update */
};

static inline HV hv_make(u16 v) {
    HV r;
    r.v = v;
    return r;
}
static inline void hv_set(HV *d, HV s) {
    d->v = s.v;
}

/* Build a box from the object at e+0x20 and the vector arg1, pass it to
 * func_01ff9188 and, unless its result word 1 is >= RES_B_LIMIT, update e.
 *
 * - func_02010af0(e) runs first.
 * - part = the object at e+0x20; its vector at +4 is pos, its vector at +0x14
 *   is the box half extent.
 * - flat = arg1 with y replaced by pos.y (func_0200d6d0); delta = pos - flat;
 *   center = pos - delta.
 * - shape: a local object given vptr data_020fcd18, then data_020fe438, with
 *   center at +4 and the half extent set by func_0204c594 from part+0x14.
 * - bounds: zeroed, then filled by func_0204c6e8 as centre -/+ half extent per
 *   axis (x min, x max, y min, y max, z min, z max).
 * - func_01ff9188(data_0210930c->+0x68->+0x384, &bounds, &delta).
 * - If the byte at +0x1c2 is set: clear FLAG_20 and call func_0200cef4(e).
 *   Otherwise set +0x1c8 = 1, copy vb0 into v1cc, zero the halfword at +0x1d8,
 *   copy data_020ff9b4 into v60 and clear FLAG_20.
 */
void func_0200d4c8(Ent *e, void *arg1) {
    Src *part;
    Vec3 flat;
    Vec3 delta;
    Vec3 center;
    Obj shape;
    Box bounds;
    s32 *w;
    Res res;

    func_02010af0(e);
    part = M2C_FIELD(e, Src **, 0x20);
    func_0200d6d0(&flat, arg1, part->y);
    vec_sub(&delta, (const Vec3 *) &part->x, &flat);
    vec_sub(&center, (const Vec3 *) &part->x, &delta);

    shape.vptr = &data_020fcd18;
    shape.p0   = center.x;
    shape.p1   = center.y;
    shape.p2   = center.z;
    shape.f10  = 0;
    shape.a0   = 0;
    shape.a1   = 0;
    shape.a2   = 0;
    shape.sum  = 0;
    func_0200af88((s8 *) &shape.f24[0], -1);
    shape.vptr   = &data_020fe438;
    shape.f24[2] = 0;
    shape.b0     = 0;
    shape.b1     = 0;
    shape.b2     = 0;
    shape.f24[1] = SHAPE_F25;
    func_0204c594(&shape, &part->f10 + 1);
    w = (s32 *) &bounds;
    do {
        *w++ = 0;
    } while (w < (s32 *) (&bounds + 1));
    func_0204c6e8(&bounds, &shape);
    res = func_01ff9188(data_0210930c.in->v, &bounds, &delta);
    if (res.b >= RES_B_LIMIT) {
        return;
    }
    if (e->f1c2 != 0) {
        e->flags &= ~FLAG_20;
        func_0200cef4(e);
        return;
    }
    e->f1c8 = 1;
    {
        const Vec3 *src = &e->vb0;
        vec_set(&e->v1cc, src);
    }
    hv_set(&e->f1d8, hv_make(0));
    vec_set(&e->v60, &data_020ff9b4);
    e->flags &= ~FLAG_20;
}
