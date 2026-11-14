#include "m2c.h"

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct {
    s32 v;
} Fx;

typedef struct {
    u16 v;
} Ang;

typedef struct {
    s32 x;
    s32 z;
} XZ;

typedef struct {
    u16 v;
    u8 f2;
    u8 f3;
    u8 f4;
    u8 f5;
} Key;

typedef struct {
    u8 v[2];
} W2;

typedef struct {
    u8 lo;
    // FAKE: full-width bitfield (same layout as a plain u8) gives the +9 byte load an extract use, which lets it
    // schedule ahead of the +8 load.
    u8 hi : 8;
} W2B;

typedef struct {
    u8 v[3];
} W3;

typedef struct {
    u8 pad0[8];
    u32 flags;
    u8 pad0c[0xb0 - 0xc];
    Vec3 pos;
    u8 padbc[0xca - 0xbc];
    Ang yaw;
    u8 padcc[0xd0 - 0xcc];
    u8 yaw_dirty;
    u8 padd1[0x236 - 0xd1];
    u8 f236;
    u8 pad237;
    void *f238;
} Ent;

typedef struct {
    u8 pad0[4];
    Ent *f4;
    u8 pad8[0x14];
    u8 f1c;
    u8 f1d;
    u8 f1e;
    u8 pad1f[0x44 - 0x1f];
    Ang f44;
    u16 f46;
    Ent *f48;
    void *f4c;
    u8 pad50[4];
    Ent *slots[2];
    u8 f5c;
    u8 f5d;
    u8 f5e;
    u8 pad5f;
    Ent *f60;
} Obj;

typedef struct {
    u8 pad0[8];
    W2B w2;
    W3 w3;
} WeightTab;
extern const WeightTab data_020ed284;
extern Vec3 data_020ff9b4;
extern const s16 data_020f88c4[];

extern void func_0202732c(Key *);
extern void func_02027348(Key *, Key *);
extern void func_02027388(Key *, Key *, u8 *, s32);
extern void func_02027590(Key *, Ent *, Vec3 *);
extern void func_0202765c(Key *, Ent *, s32, u8 *, s32, u8, u8, u8 *);
extern void func_020278ac(Key *, Ent *);
extern void func_02027c70(Key *, Ent *, s32);
extern s32 func_0200aef8(Vec3 *, Vec3 *, Fx);
extern s32 func_020347fc(Obj *);
extern s32 func_02034978(Obj *);
extern void func_02034a90(Obj *);
extern void func_020344f0(Obj *, s32);
extern void func_02030038(Obj *);
extern void func_0202ff38(Obj *);
extern s16 func_020985d8(Fx, Fx);
extern void func_02018200(Ent *, Ent *, s32);
extern void func_02017ee4(Ent *, Vec3 *, s32);
extern void func_02030540(Obj *, u8, Ang);

enum {
    REG_DIVCNT      = 0x04000280,
    REG_DIV_NUMER   = 0x04000290,
    REG_DIV_DENOM   = 0x04000298,
    REG_DIV_RESULT  = 0x040002A0,
    REG_SQRTCNT     = 0x040002B0,
    REG_SQRT_RESULT = 0x040002B4,
    REG_SQRT_PARAM  = 0x040002B8,
    DIV_MODE_64_64  = 2,
    SQRT_MODE_64    = 1,
    HW_BUSY         = 0x8000,
};

static inline Ang ang_make(u16 v) {
    Ang a;
    a.v = v;
    return a;
}

static inline u16 ang_get(Ang a) {
    return a.v;
}

static inline u16 ang_val(const Ang *p) {
    return ang_get(*p);
}

static inline Ang ang_add(u16 base, const Ang *d) {
    Ang a;
    a.v = base + ang_get(*d);
    return a;
}

static inline s32 fx_mul(s32 a, Fx b) {
    return (s32) (((s64) a * b.v + 0x800) >> 12);
}

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

static inline void vec_sub(Vec3 *d, const Vec3 *a, const Vec3 *b) {
    s32 z = a->z - b->z;
    s32 y = a->y - b->y;
    s32 x = a->x - b->x;
    d->x  = x;
    d->y  = y;
    d->z  = z;
}

static inline void vec_normalize(const Vec3 *src, Vec3 *dst) {
    s64 t;
    s32 sqrt;
    s64 div;

// FAKE: raw s32 indexing of src instead of src->x/y/z makes mwcc reload the three components here instead of
    // reusing the caller's zero-test loads (same spelling as matched src/auto/func_02016248.c, unmarked there).
    t = (s64) ((const s32 *) src)[0] * ((const s32 *) src)[0];
    t += (s64) ((const s32 *) src)[1] * ((const s32 *) src)[1];
    t += (s64) ((const s32 *) src)[2] * ((const s32 *) src)[2];
    *(volatile u16 *) REG_DIVCNT    = DIV_MODE_64_64;
    *(volatile u64 *) REG_DIV_NUMER = 1LL << 56;
    *(volatile u64 *) REG_DIV_DENOM = t;
    *(volatile u16 *) REG_SQRTCNT   = SQRT_MODE_64;
    *(u64 *) REG_SQRT_PARAM         = t << 2;
    while (*(volatile u16 *) REG_SQRTCNT & HW_BUSY) {
    }
    sqrt = *(volatile s32 *) REG_SQRT_RESULT;
    while (*(volatile u16 *) REG_DIVCNT & HW_BUSY) {
    }
    div = *(s64 *) REG_DIV_RESULT;
    div *= sqrt;
    dst->x = (s32) ((div * src->x + (1LL << 44)) >> 45);
    dst->y = (s32) ((div * src->y + (1LL << 44)) >> 45);
    dst->z = (s32) ((div * src->z + (1LL << 44)) >> 45);
}

static inline void vec_scale(Vec3 *v, const Fx *len) {
    Fx x, y, z;

    x    = *len;
    v->x = fx_mul(v->x, x);
    y    = *len;
    v->y = fx_mul(v->y, y);
    z    = *len;
    v->z = fx_mul(v->z, z);
}

static inline void ent_set_yaw_u(Ent *e, u16 yaw) {
    if (e->yaw.v != yaw) {
        e->yaw.v     = yaw;
        e->yaw_dirty = 1;
    }
}

static inline Ang ang_addm(u16 base, Ang d) {
    return ang_make(base + d.v);
}

static inline void ang_step_g(u16 *p, u16 step) {
    Ang d;
    const Ang *q = &d;
    d.v          = step;
    *p           = ang_get(ang_addm(*p, *q));
}

static inline Ang ang_add_p(u16 base, const Ang *d) {
    Ang a;
    a.v = base + ang_val(d);
    return a;
}

void func_02035cd0(Obj *this) {
    Key keys2[2];
    Vec3 dir;
    Key keys3[3];
    Vec3 tgt;
    Vec3 push;
    Fx speed;
    Key k78;
    W3 w3;
    Key k6e, k68, k62, k5c;
    Fx range;
    Key k52, k4c, k46, k40;
    W2 w2;

    s32 i;

    for (i = 0; i < 2; i++) {
        if (this->slots[i] != 0 && (this->slots[i]->flags & 0x802)) {
            this->slots[i] = 0;
        }
    }
    if (this->f60 != 0) {
        Ent *e = this->f60;
        if ((e->flags & 2) == 2 || this->f4 != e->f238 || e->f236 == 0) {
            this->f60 = 0;
        }
    } else if (this->slots[0] == 0 && this->slots[1] == 0) {
        func_02034a90(this);
    }

    if (func_020347fc(this) != 0) {
        speed.v = 0x14000;
        {
            // FAKE: volatile read of the lo byte (+8) pins it in the IR so the bytes load as
            // ldrb r1,[+9]; ldrb r2,[+8] like the target (plain copies load +8 first).
            u8 lo   = ((const volatile u8 *) &data_020ed284.w2)[0];
            w2.v[1] = data_020ed284.w2.hi;
            w2.v[0] = lo;
        }
        // FAKE: repeats the hi store after the lo store; the first one becomes dead and is dropped, so the bytes
        // are stored lo (sp+0x2a) then hi (sp+0x2b) like the target.
        w2.v[1] = data_020ed284.w2.hi;
        func_0202732c(&keys2[0]);
        func_0202732c(&keys2[1]);
        func_0202732c(&k78);
        vec_sub(&dir, &this->f48->pos, &this->f4->pos);
        ent_set_yaw_u(this->f4, ang_val(&this->f44));
        if (func_0200aef8(&this->f48->pos, &this->f4->pos, speed) != 0) {
            keys2[0].f2 = 0;
            keys2[0].f5 = 1;
            keys2[0].f4 = 1;
            {
                Fx z;
                Fx x;
                Ang a;
                z.v        = dir.z;
                x.v        = dir.x;
                a.v        = (u16) func_020985d8(x, z) - ang_val(&this->f4->yaw);
                keys2[0].v = ang_val(&a);
            }
        } else {
            if (dir.x == 0 && dir.y == 0 && dir.z == 0) {
                vec_set(&dir, &data_020ff9b4);
            } else {
                vec_normalize(&dir, &dir);
            }
            vec_scale(&dir, &speed);
            vec_sub(&tgt, &this->f48->pos, &dir);
            func_02027590(&k68, this->f4, &tgt);
            func_02027348(&keys2[0], &k68);
        }
        func_020278ac(&k62, this->f4);
        func_02027348(&keys2[1], &k62);
        func_02027388(&k5c, keys2, w2.v, 2);
        func_02027348(&k78, &k5c);
        func_02030540(this, k78.f2, ang_add(this->f4->yaw.v, (Ang *) &k78.v));
        this->f44.v = ang_val(&this->f4->yaw);
        func_02018200(this->f4, this->f48, 0);
        range.v = (s32) (((s64) speed.v * 0x2000 + 0x800) >> 12);
        if (func_0200aef8(&this->f48->pos, &this->f4->pos, range) == 0) {
            return;
        }
        push.x = (s32) (((s64) data_020f88c4[(this->f46 >> 4) * 2] * 0x3000 + 0x800) >> 12);
        push.y = 0;
        push.z = (s32) (((s64) data_020f88c4[(this->f46 >> 4) * 2 + 1] * 0x3000 + 0x800) >> 12);
        func_02017ee4(this->f4, &push, 0x80);
        ang_step_g(&this->f46, 0x666);
        return;
    }

    func_02034978(this);
    if (this->f4c == 0) {
        func_020344f0(this, 0x19);
    }
    if (this->f1e & 2) {
        func_02030038(this);
        return;
    }
    w3 = data_020ed284.w3;
    func_0202732c(&keys3[0]);
    func_0202732c(&keys3[1]);
    func_0202732c(&keys3[2]);
    func_0202732c(&k6e);
    func_0202765c(&k52, this->f4, this->f5c, &this->f5d, this->f5e, this->f1c, 1, 0);
    func_02027348(&keys3[0], &k52);
    func_020278ac(&k4c, this->f4);
    func_02027348(&keys3[1], &k4c);
    func_02027c70(&k46, this->f4, 2);
    func_02027348(&keys3[2], &k46);
    func_02027388(&k40, keys3, w3.v, 3);
    func_02027348(&k6e, &k40);
    func_02030540(this, k6e.f2, ang_add_p(this->f4->yaw.v, (Ang *) &k6e.v));
    if (k6e.f3 != 0) {
        func_0202ff38(this);
    }
}
