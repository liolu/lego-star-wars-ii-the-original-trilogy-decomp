#include "m2c.h"

typedef volatile u64 vu64;
typedef volatile u32 vu32;
typedef volatile u16 vu16;

// DS divider / square-root unit registers.
enum {
    REG_DIVCNT      = 0x04000280,
    REG_DIV_NUMER   = 0x04000290,
    REG_DIV_DENOM   = 0x04000298,
    REG_DIV_RESULT  = 0x040002a0,
    REG_SQRTCNT     = 0x040002b0,
    REG_SQRT_RESULT = 0x040002b4,
    REG_SQRT_PARAM  = 0x040002b8
};

enum {
    DIV_MODE_64_64 = 2,
    SQRT_MODE_64   = 1,
    MATH_BUSY      = 0x8000
};

// 20.12 fixed point.
enum {
    FX_SHIFT = 12,
    FX_HALF  = 0x800,
    FX_ONE   = 0x1000
};

struct Fx {
    s32 v;
};

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

// Result of the slot-0x3c virtual; the non-trivial copy constructor makes
// the compiler return it through a hidden pointer in r0 (this in r1).
class Dist {
public:
    s32 v;
    Dist(const Dist &other);
};

class Actor {
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
    virtual void unknown24(u16 id, s32 a, s32 b, s32 c); // Slot 0x24.
    virtual void unknown28();
    virtual void unknown2c();
    virtual void unknown30();
    virtual void unknown34();
    virtual void unknown38();
    virtual Dist unknown3c(u16 id); // Slot 0x3c.
};

typedef struct {
    u8 pad00[4];
    Actor *actor;
    u8 pad08[8];
    Vec3 pos;
    s32 radius;
    Vec3 dir;
    Fx speed;
    s32 f30;
    s32 f34;
    u16 f38;
    u16 id;
    u8 pad3c;
    u8 f3d;
} Mover;

extern "C" Vec3 data_020ff9b4;

extern "C" void func_0201812c(void *a, Vec3 *b, s32 flag);
extern "C" u8 func_0202cb68(void *arg0);
extern "C" u8 func_0202cb7c(void *arg0);
extern "C" s32 FX_Div(s32, s32);

static inline void vec_copy(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

static inline void vec_sub(const Vec3 *a, const Vec3 *b, Vec3 *d) {
    s32 x, y, z;
    z    = a->z - b->z;
    y    = a->y - b->y;
    x    = a->x - b->x;
    d->x = x;
    d->y = y;
    d->z = z;
}

static inline s32 vec_mag(const Vec3 *v) {
    s64 t = (s64) v->x * v->x;
    t += (s64) v->y * v->y;
    t += (s64) v->z * v->z;
    (*(vu16 *) REG_SQRTCNT)    = SQRT_MODE_64;
    (*(vu64 *) REG_SQRT_PARAM) = t << 2;
    while ((*(vu16 *) REG_SQRTCNT) & MATH_BUSY) {
    }
    return (s32) ((*(vu32 *) REG_SQRT_RESULT) + 1) >> 1;
}

static inline void vec_normalize(const Vec3 *s, Vec3 *d) {
    s64 t;
    s32 sqrt;
    s64 div;

    t = (s64) ((const s32 *) s)[0] * ((const s32 *) s)[0];
    t += (s64) ((const s32 *) s)[1] * ((const s32 *) s)[1];
    t += (s64) ((const s32 *) s)[2] * ((const s32 *) s)[2];

    (*(vu16 *) REG_DIVCNT)    = DIV_MODE_64_64;
    (*(vu64 *) REG_DIV_NUMER) = 1LL << 56;
    (*(vu64 *) REG_DIV_DENOM) = t;

    (*(vu16 *) REG_SQRTCNT)   = SQRT_MODE_64;
    (*(u64 *) REG_SQRT_PARAM) = t << 2;

    while ((*(vu16 *) REG_SQRTCNT) & MATH_BUSY) {
    }
    sqrt = (s32) (*(vu32 *) REG_SQRT_RESULT);

    while ((*(vu16 *) REG_DIVCNT) & MATH_BUSY) {
    }
    div = (s64) (*(u64 *) REG_DIV_RESULT);

    div *= sqrt;

    d->x = (s32) ((div * s->x + (1LL << 44)) >> 45);
    d->y = (s32) ((div * s->y + (1LL << 44)) >> 45);
    d->z = (s32) ((div * s->z + (1LL << 44)) >> 45);
}

static inline s32 fx_mul(s32 a, Fx b) {
    return (s32) (((s64) a * b.v + FX_HALF) >> FX_SHIFT);
}

// v *= s, one component per helper level: x, then y, then z. Each component
// re-reads *s after the previous store (v may alias s) and passes it by value,
// so every fx_mul call leaves its own stack copy of the scale.
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

// Clears actor flag 0x200 and calls func_0201812c(actor, &pos, 0).
// d = pos - actor position (+0xb0), with d.y = 0 when actor flag 0x20 is set;
// dist = |d|. When dist > radius: a negative speed is replaced by the actor's
// slot-0x3c result, a zero speed by FX_ONE (and dir is then reset to
// data_020ff9b4 at the end); dir = normalize(d) (data_020ff9b4 for d == 0)
// scaled by min(speed, dist - radius); f38 = integer part of
// (dist - radius) / speed; the actor's slot-0x24 virtual gets (id, 1, 1, 0).
// Otherwise f38 = 0. With f3d set, f30/f34 are refreshed from the actor's
// +0x5e0 block via func_0202cb68/func_0202cb7c.
extern "C" void func_0203309c(Mover *self) {
    Vec3 d;
    s32 dist;
    s32 reset;

    M2C_FIELD(self->actor, u32 *, 8) &= ~0x200;
    func_0201812c(self->actor, &self->pos, 0);
    vec_sub(&self->pos, &M2C_FIELD(self->actor, Vec3 *, 0xb0), &d);
    if ((M2C_FIELD(self->actor, u32 *, 8) & 0x20) == 0x20) {
        d.y = 0;
    }
    dist = vec_mag(&d);
    if (dist > self->radius) {
        reset = 0;
        if (self->speed.v < 0) {
            self->speed.v = self->actor->unknown3c(self->id).v;
        }
        if (self->speed.v == 0) {
            self->speed.v = FX_ONE;
            reset         = 1;
        }
        if (d.x == 0 && d.y == 0 && d.z == 0) {
            vec_copy(&d, &data_020ff9b4);
        } else {
            vec_normalize(&d, &d);
        }
        vec_copy(&self->dir, &d);
        if (dist - self->radius < self->speed.v) {
            self->speed.v = dist - self->radius;
        }
        vec_scale(&self->dir, &self->speed);
        self->f38 = FX_Div(dist - self->radius, self->speed.v) >> 12;
        self->actor->unknown24(self->id, 1, 1, 0);
        if (reset) {
            vec_copy(&self->dir, &data_020ff9b4);
        }
    } else {
        self->f38 = 0;
    }
    if (self->f3d) {
        self->f30 = func_0202cb68((u8 *) self->actor + 0x5e0);
        self->f34 = func_0202cb7c((u8 *) self->actor + 0x5e0);
    }
}
