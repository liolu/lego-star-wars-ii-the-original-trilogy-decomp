#include "m2c.h"

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct Inner {
    u8 pad0[0x4];
    Vec3 pos;
} Inner;

typedef struct Target {
    u8 pad0[0x20];
    Inner *link;
    u8 pad1[0x8c];
    Vec3 pos;
} Target;

// Only slot 0x44 is called here; the earlier virtuals keep the observed slot position.
class Obj {
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
    virtual void unknown24();
    virtual void unknown28();
    virtual void unknown2c();
    virtual void unknown30();
    virtual void unknown34();
    virtual void unknown38();
    virtual void unknown3c();
    virtual void unknown40();
    virtual void *unknown44(); // Slot 0x44.

    u8 pad0[0x5c];
    Vec3 vel;
    u8 pad1[0x44];
    Vec3 pos;
    u8 pad2[0xe];
    u16 angle;
    u8 pad3[0x50e];
    u8 f5da;
};

struct Owner {
    u8 pad0[4];
    Obj *obj;
};

extern "C" {

extern void func_02007f24(s32 *out, double v);
extern Obj *func_02010c6c(Obj *a0, s32 a1, s32 a2, void *a3, u8 a4, Vec3 *a5, Target *a6, s32 a7);
extern s32 FX_Div(s32, s32);
extern s32 FX_Sqrt(s32);
extern const u8 data_020f4d0c[];
extern const s16 data_020f88c4[];

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

static inline s32 fx_neg(const s32 *a) {
// FAKE: redundant pointer copy p and always-true null test (a is always &k); together they set the k
// load/negate order and registers around the FX_Div call (plain `return -*a` no longer matches)
    s32 r        = 0;
    const s32 *p = a;
    if (p) {
        r = -*p;
    }
    return r;
}

static inline s64 vec_len_sq_p(const Vec3 *v) {
    s64 t = (s64) v->x * v->x;
    t += (s64) v->y * v->y;
    t += (s64) v->z * v->z;
    return t;
}

// FAKE: one-field wrapper used only for the angle; makes the angle value get its register after the dv fields
typedef struct {
    s32 v;
} Ang;

void func_0202fcc8(Owner *arg0, Target *arg1) {
    Obj *o;
    Obj *r;
    Vec3 pos;
    Vec3 out;
    const Vec3 *src;
    s32 k;
    s32 q;
    Ang ang;
    Vec3 dv;
    s32 d;
    s64 t;
    s32 s, c, i, j;

    o = arg0->obj;
    r = func_02010c6c(arg0->obj, 7, 0, o->unknown44(), o->f5da, &o->pos, arg1, 0);
    if (r == 0) {
        return;
    }
    if (arg1 != 0) {
        pos.x = arg1->pos.x;
        pos.y = arg1->pos.y;
        pos.z = arg1->pos.z;
        {
            int f = arg1->link != 0 && ((u8 *) arg1->link)[0x26] != 0;
            if (f) {
                vec_set(&pos, &arg1->link->pos);
            }
        }
        dv.z  = pos.z - r->pos.z;
        dv.y  = pos.y - r->pos.y;
        dv.x  = pos.x - r->pos.x;
        ang.v = r->angle;
        func_02007f24(&k, 0.115);
        q = FX_Sqrt(FX_Div((s32) (((s64) dv.y * 0x2000 + 0x800) >> 12), fx_neg(&k)));
        if (q < 0x1000) {
            q = 0x1000;
        }
        t = vec_len_sq_p(&dv);
        // DS hardware square-root unit (SQRTCNT 0x040002B0, SQRT_RESULT 0x040002B4, SQRT_PARAM 0x040002B8)
        *(volatile u16 *) 0x040002B0 = 1;
        *(volatile u64 *) 0x040002B8 = (u64) t << 2;
        while (*(volatile u16 *) 0x040002B0 & 0x8000) {
        }
        d = FX_Div((s32) (*(volatile u32 *) 0x040002B4 + 1) >> 1, q);
        if (d > (s32) (data_020f4d0c[0x58] << 10)) {
            d = data_020f4d0c[0x58] << 10;
        }
        i     = (ang.v >> 4) << 1;
        s     = data_020f88c4[i];
        j     = i + 1;
        c     = data_020f88c4[j];
        out.x = (s32) (((s64) d * s + 0x800) >> 12);
        out.y = 0;
        out.z = (s32) (((s64) d * c + 0x800) >> 12);
        src   = &out;
        vec_set(&r->vel, src);
    } else {
        Obj *p = arg0->obj;
        r->vel.x += p->vel.x;
        r->vel.y += p->vel.y;
        r->vel.z += p->vel.z;
    }
}
}
