#include "m2c.h"

// NDS hardware divider / square-root unit (ARM9 I/O registers).
enum {
    REG_DIVCNT      = 0x4000280,
    REG_DIV_NUMER   = 0x4000290,
    REG_DIV_DENOM   = 0x4000298,
    REG_DIV_RESULT  = 0x40002A0,
    REG_SQRTCNT     = 0x40002B0,
    REG_SQRT_RESULT = 0x40002B4,
    REG_SQRT_PARAM  = 0x40002B8
};

enum {
    DIV_MODE_64_64 = 2,
    SQRT_MODE_64   = 1,
    MATH_BUSY      = 0x8000
};

// 20.12 fixed point.
enum {
    FX_ONE = 0x1000,
    FX_TWO = 0x2000
};

typedef struct {
    s32 x, y, z;
} Vec3;

struct Fx {
    s32 v;
    Fx() {}
    s32 get() const {
        return v;
    }
};

// Partial interface: unknown virtuals keep their observed vtable slots.
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
    virtual void unknown24(s32, s32, s32, s32); // Slot 0x24.
    virtual void unknown28();
    virtual void unknown2c();
    virtual void unknown30();
    virtual void unknown34();
    virtual void unknown38();
    virtual Fx unknown3c(s32); // Slot 0x3c.

    s32 pad4;
    u32 flags;
    u8 padc[0xb0 - 0xc];
    Vec3 pos;
};

typedef struct {
    s32 pad0;
    Obj *obj;
    u8 pad8[0x32 - 0x8];
    s16 timer;
    Vec3 pos;
    Vec3 dir;
    s32 anim;
} Self;

extern "C" {
extern Vec3 data_020ff9b4;

s32 FX_Div(s32, s32);
}

static inline void vec_subr(const Vec3 *a, const Vec3 *b, Vec3 *o) {
    s32 z = a->z - b->z;
    s32 y = a->y - b->y;
    s32 x = a->x - b->x;
    o->x  = x;
    o->y  = y;
    o->z  = z;
}

static inline bool vec_is_zero(const Vec3 *v) {
    return v->x == 0 && v->y == 0 && v->z == 0;
}

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

static inline s32 fx_mul(s32 a, s32 b) {
    return (s32) (((s64) a * b + 0x800) >> 12);
}

static inline s32 fx_mulw(s32 a, Fx b) {
    return (s32) (((s64) a * b.get() + 0x800) >> 12);
}

// v *= s, one component per helper level (x, then y, then z); each fx_mulw
// call takes its own by-value copy of *s.
static inline void vec_scale_x(Vec3 *v, const Fx *s) {
    v->x = fx_mulw(v->x, *s);
}

static inline void vec_scale_xy(Vec3 *v, const Fx *s) {
    vec_scale_x(v, s);
    v->y = fx_mulw(v->y, *s);
}

static inline void vec_scale(Vec3 *v, const Fx *s) {
    vec_scale_xy(v, s);
    v->z = fx_mulw(v->z, *s);
}

// v = v / |v| using the hardware divider (2^56 / |v|^2) and square root.
static inline void vec_normalize(Vec3 *v) {
    s64 t;
    s32 sq;
    t = (s64) v->x * v->x;
    t += (s64) v->y * v->y;
    t += (s64) v->z * v->z;
    *(volatile u16 *) REG_DIVCNT    = DIV_MODE_64_64;
    *(volatile s64 *) REG_DIV_NUMER = 0x0100000000000000LL;
    *(volatile s64 *) REG_DIV_DENOM = t;
    *(volatile u16 *) REG_SQRTCNT   = SQRT_MODE_64;
    *(s64 *) REG_SQRT_PARAM         = t << 2;
    while (*(volatile u16 *) REG_SQRTCNT & MATH_BUSY) {
    }
    sq = *(volatile s32 *) REG_SQRT_RESULT;
    while (*(volatile u16 *) REG_DIVCNT & MATH_BUSY) {
    }
    t    = *(volatile s64 *) REG_DIV_RESULT;
    t    = t * sq;
    v->x = (s32) ((t * v->x + (1LL << 44)) >> 45);
    v->y = (s32) ((t * v->y + (1LL << 44)) >> 45);
    v->z = (s32) ((t * v->z + (1LL << 44)) >> 45);
}

// Clears obj flag 0x200; d = self->pos - obj->pos, dist = |d| (hardware sqrt).
// dist <= 2.0: timer = 0. Otherwise: scale = obj slot-0x3c result for anim (0 is
// replaced by FX_ONE and dir is reset to data_020ff9b4 at the end); dir =
// normalize(d) (data_020ff9b4 when d is zero) scaled by min(scale, dist - 2.0);
// timer = integer part of (dist - 2.0) / scale; anim = 0x69; obj slot 0x24
// virtual gets (anim, 1, 1, 0).
extern "C" void func_0203bca0(Self *self) {
    Vec3 d;
    s32 dflt;
    s32 dist;
    Obj *obj;
    s64 t;

    self->obj->flags &= ~0x200;
    obj = self->obj;
    vec_subr(&self->pos, &obj->pos, &d);
    t = (s64) d.x * d.x;
    t += (s64) d.y * d.y;
    t += (s64) d.z * d.z;
    *(volatile u16 *) REG_SQRTCNT    = SQRT_MODE_64;
    *(volatile s64 *) REG_SQRT_PARAM = t << 2;
    while (*(volatile u16 *) REG_SQRTCNT & MATH_BUSY) {
    }
    dist = (*(volatile s32 *) REG_SQRT_RESULT + 1) >> 1;
    if (dist > FX_TWO) {
        dflt     = 0;
        Fx scale = self->obj->unknown3c(self->anim);
        if (scale.v == 0) {
            scale.v = FX_ONE;
            dflt    = 1;
        }
        if (vec_is_zero(&d)) {
            vec_set(&d, &data_020ff9b4);
        } else {
            vec_normalize(&d);
        }
        vec_set(&self->dir, &d);
        if (dist - FX_TWO < scale.v) {
            scale.v = dist - FX_TWO;
        }
        vec_scale(&self->dir, &scale);
        self->timer = FX_Div(dist - FX_TWO, scale.v) >> 12;
        self->anim  = 0x69;
        self->obj->unknown24(self->anim, 1, 1, 0);
        if (dflt) {
            vec_set(&self->dir, &data_020ff9b4);
        }
    } else {
        self->timer = 0;
    }
}
