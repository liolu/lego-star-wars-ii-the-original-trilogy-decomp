#include "m2c.h"

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

class Base {
public:
    virtual void v00();
    virtual void v04();
    virtual void v08();
    virtual void v0c();
    virtual void v10();
    virtual void v14();
    virtual void v18();
    virtual void v1c();
    virtual void v20();
    virtual void f24(s32, s32, s32, s32);
    virtual void v28();
    virtual void v2c();
    virtual void v30();
    virtual void v34();
    virtual void v38();
    virtual void v3c();
    virtual void v40();
    virtual void v44();
    virtual void v48();
    virtual void v4c();
    virtual void f50();
};

class Child : public Base {
public:
    u8 pad4[0x212 - 4];
    u16 state;
};

class Obj : public Base {
public:
    u8 pad4b[4];
    u32 flags;
    u8 pad0c[0x60 - 0x0c];
    Vec3 pos;
    u8 pad6c[0xb0 - 0x6c];
    Vec3 cur;
    u8 padbc[0xd1 - 0xbc];
    u8 fd1;
    u8 padd2[0x214 - 0xd2];
    u8 f214;
    u8 pad215[2];
    u8 f217;
    u8 pad218[0x224 - 0x218];
    Child *child;
};

typedef struct {
    s32 v;
} Fx;

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

static inline void vec_copy(Vec3 *d, const Vec3 *s) {
    d->x = s->x;
    d->y = s->y;
    d->z = s->z;
}

static inline s64 vec_sq(const Vec3 &v) {
    s32 x = v.x;
    s32 y = v.y;
    s32 z = v.z;
    s64 r = (s64) x * x;
    r += (s64) y * y;
    r += (s64) z * z;
    return r;
}

static inline int vec_ne(const Vec3 *a, const Vec3 *b) {
    if (a->x != b->x || a->y != b->y || a->z != b->z) {
        return 1;
    }
    return 0;
}

static inline void vec_sub(Vec3 *d, const Vec3 *a, const Vec3 *b) {
    d->x = a->x - b->x;
    d->y = a->y - b->y;
    d->z = a->z - b->z;
}

// DS hardware divider / square-root unit.
enum {
    REG_DIVCNT      = 0x04000280,
    REG_DIV_NUMER   = 0x04000290,
    REG_DIV_DENOM   = 0x04000298,
    REG_DIV_RESULT  = 0x040002A0,
    REG_SQRTCNT     = 0x040002B0,
    REG_SQRT_RESULT = 0x040002B4,
    REG_SQRT_PARAM  = 0x040002B8
};

enum {
    DIV_MODE_64_64 = 2,
    SQRT_MODE_64   = 1,
    MATH_BUSY      = 0x8000
};

// 20.12 fixed point.
enum {
    FX_SHIFT = 12,
    FX_HALF  = 0x800
};

// func_02007eb4 results below this (5.0 in 20.12) snap the object onto the target.
enum {
    SNAP_LIMIT = 0x5000
};

extern "C" void func_02007f24(Fx *, f64);
extern "C" void func_02007eb4(u32 *, s32 *, s32 *);
extern "C" void func_02016664(Child *);
extern "C" const Vec3 data_020ff9b4;

static inline s32 fx_mul(s32 a, Fx b) {
    return (s32) (((s64) a * b.v + FX_HALF) >> FX_SHIFT);
}

// v *= s, one component per helper level (x, then y, then z); each fx_mul
// takes its own by-value copy of *s.
static inline void vec_scale_x(Vec3 *v, const Fx *s) {
    v->x = fx_mul(v->x, *s);
}

static inline void vec_scale_xy(Vec3 *v, const Fx *s) {
    vec_scale_x(v, s);
    v->y = fx_mul(v->y, *s);
}

static inline void vec_scale(Vec3 *v, const Fx *s) {
    vec_scale_xy(v, s);
    v->z = fx_mul(v->z, *s);
}

// Moves obj (current position at +0xb0) toward target.
// target == current: set flag 0x200, store target, set f217 and notify the child (state 0xa9: obj slot 0x24
// with 0x56, f214 = 0xe, child slot 0x50; else child slot 0x24 with 0x78 and func_02016664).
// Otherwise dir = target - current with y = 0, normalised to a 20.12 unit vector (data_020ff9b4 when zero),
// step = dir scaled by the value func_02007f24 writes; if func_02007eb4(current, target) < SNAP_LIMIT the
// object snaps as above (also clearing flag 0x20), else step is stored at +0x60 and flag 0x200 is cleared.
extern "C" void func_02023374(Obj *obj, Vec3 target) {
    Vec3 dir;
    Vec3 step;
    u32 dist;
    Fx scale;
    s64 sq;
    s64 q;
    s32 root;
    Vec3 t;
    Vec3 c;
    vec_copy(&t, &target);
    vec_copy(&c, &obj->cur);

    if (t.x == c.x && t.y == c.y && t.z == c.z) {
        obj->flags |= 0x200;
        if (vec_ne(&obj->cur, &target)) {
            vec_set(&obj->cur, &target);
            obj->fd1 = 1;
        }
        obj->f217 = 1;
        if (obj->child == 0) {
            return;
        }
        if (obj->child->state == 0xa9) {
            obj->f24(0x56, 0, 1, 0);
            obj->f214 = 0xe;
            obj->child->f50();
        } else {
            obj->child->f24(0x78, 0, 1, 0);
            func_02016664(obj->child);
        }
        return;
    }
    vec_sub(&dir, &t, &c);
    dir.y = 0;
    if (dir.x == 0 && dir.y == 0 && dir.z == 0) {
        vec_set(&dir, &data_020ff9b4);
    } else {
        sq = vec_sq(dir);
        // q = (2^56 / sq) * sqrt(4 * sq) = 2^57 / |dir|, so dir * q >> 45 is dir / |dir| in 20.12.
        *(volatile u16 *) REG_DIVCNT    = DIV_MODE_64_64;
        *(volatile u64 *) REG_DIV_NUMER = (u64) 1 << 56;
        *(volatile s64 *) REG_DIV_DENOM = sq;
        *(volatile u16 *) REG_SQRTCNT   = SQRT_MODE_64;
        *(u64 *) REG_SQRT_PARAM         = sq << 2;
        while (*(volatile u16 *) REG_SQRTCNT & MATH_BUSY) {
        }
        root = *(volatile u32 *) REG_SQRT_RESULT;
        while (*(volatile u16 *) REG_DIVCNT & MATH_BUSY) {
        }
        q     = *(volatile s64 *) REG_DIV_RESULT;
        q     = q * root;
        dir.x = (s32) ((q * dir.x + ((s64) 1 << 44)) >> 45);
        dir.y = (s32) ((q * dir.y + ((s64) 1 << 44)) >> 45);
        dir.z = (s32) ((q * dir.z + ((s64) 1 << 44)) >> 45);
    }
    vec_copy(&step, &dir);
    func_02007f24(&scale, 1.0);
    vec_scale(&step, &scale);
    func_02007eb4(&dist, &obj->cur.x, &target.x);
    if ((s32) dist < SNAP_LIMIT) {
        obj->flags |= 0x200;
        if (vec_ne(&obj->cur, &target)) {
            vec_set(&obj->cur, &target);
            obj->fd1 = 1;
        }
        obj->f217 = 1;
        obj->flags &= ~0x20;
        if (obj->child == 0) {
            return;
        }
        if (obj->child->state == 0xa9) {
            obj->f24(0x56, 0, 1, 0);
            obj->f214 = 0xe;
            obj->child->f50();
        } else {
            obj->child->f24(0x78, 0, 1, 0);
            func_02016664(obj->child);
        }
    } else {
        vec_set(&obj->pos, &step);
        obj->flags &= ~0x200;
    }
}
