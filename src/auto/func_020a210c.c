#include "m2c.h"

/* Step self->pos toward the point func_020aa794 reports for self (Q12 vectors).
 *
 * Runs only when the current slot's owner (data_02109358[data_021027f8[0x2a]]) has a kind in 0x54..0x63,
 * func_02093fa8(func_020917b4(&data_0210930c)) returns 1 or 2, and the kind's table byte is non-zero.
 * The direction (point - pos) is made a unit vector with the hardware divider and square-root units (zero stays
 * zero), scaled by the kind's table byte as a whole-unit step length, and added to pos; pos_changed is set when
 * pos actually moved. */

enum {
    REG_DIVCNT      = 0x04000280,
    REG_DIV_NUMER   = 0x04000290,
    REG_DIV_DENOM   = 0x04000298,
    REG_DIV_RESULT  = 0x040002a0,
    REG_SQRTCNT     = 0x040002b0,
    REG_SQRT_RESULT = 0x040002b4,
    REG_SQRT_PARAM  = 0x040002b8,
    DIV_MODE_64_64  = 2,
    SQRT_MODE_64    = 1,
    MATH_BUSY       = 0x8000,
    FX_SHIFT        = 12,
    FX_HALF         = 0x800,
    SLOT_INDEX      = 0x2a,
    KIND_FIRST      = 0x54,
    KIND_LAST       = 0x63,
    MODE_A          = 1,
    MODE_B          = 2,
};

typedef volatile u16 vu16;
typedef volatile u32 vu32;
typedef volatile u64 vu64;

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct {
    s32 v;
} Fx;

typedef struct {
    u8 pad0[0x28];
    Vec3 pos;
    u8 pad1[0x49 - 0x34];
    u8 pos_changed;
} Obj;

typedef struct {
    u8 pad0[0x6bb];
    u8 kind;
} Owner;

typedef struct {
    u8 step_len;
    u8 pad[11];
} KindInfo;

extern u8 data_021027f8[];
extern Owner *data_02109358[];
extern M2C_UNK data_0210930c;
extern KindInfo data_020fec71[];
extern Vec3 data_020ff9b4;

extern u8 func_020917b4(void *);
extern u8 func_02093fa8(s32);
void func_020aa794(Obj *obj, Vec3 *out);

static inline void vec_sub(Vec3 *d, const Vec3 *a, const Vec3 *b) {
    s32 z = a->z - b->z;
    s32 y = a->y - b->y;
    s32 x = a->x - b->x;
    d->x  = x;
    d->y  = y;
    d->z  = z;
}

static inline void vec_add(Vec3 *d, const Vec3 *a, const Vec3 *b) {
    d->x = a->x + b->x;
    d->y = a->y + b->y;
    d->z = a->z + b->z;
}

static inline void vec_copy(Vec3 *d, const Vec3 *s) {
    d->x = s->x;
    d->y = s->y;
    d->z = s->z;
}

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

static inline s32 vec_ne(const Vec3 *a, const Vec3 *b) {
    if (a->x != b->x || a->y != b->y || a->z != b->z) {
        return 1;
    }
    return 0;
}

static inline void vec_normalize(Vec3 *d, const Vec3 *s) {
    s64 t;
    s32 sqrt;
    s64 div;

    t = (s64) s->x * s->x;
    t += (s64) s->y * s->y;
    t += (s64) s->z * s->z;

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

static inline void vec_unit_self(Vec3 *v) {
    if (v->x == 0 && v->y == 0 && v->z == 0) {
        vec_set(v, &data_020ff9b4);
    } else {
        vec_normalize(v, v);
    }
}

static inline s32 fx_mul_p(s32 a, const Fx *b) {
    return (s32) (((s64) a * b->v + FX_HALF) >> FX_SHIFT);
}

void func_020a210c(Obj *self) {
    Owner *owner = data_02109358[data_021027f8[SLOT_INDEX]];
    KindInfo *tab;
    Vec3 point;
    Vec3 pos;
    Vec3 step;
    Fx scale;
    Fx z, y, x;

    if (owner->kind < KIND_FIRST || owner->kind > KIND_LAST) {
        return;
    }
    if (func_02093fa8(func_020917b4(&data_0210930c)) != MODE_A && func_02093fa8(func_020917b4(&data_0210930c)) != MODE_B) {
        return;
    }
    tab = data_020fec71;
    if (tab[owner->kind].step_len == 0) {
        return;
    }
    point.x = 0;
    point.y = 0;
    point.z = 0;
    func_020aa794(self, &point);
    vec_copy(&pos, &self->pos);
    vec_sub(&step, &point, &pos);
    vec_unit_self(&step);
    scale.v = tab[owner->kind].step_len << FX_SHIFT;
    x       = scale;
    step.x  = fx_mul_p(step.x, &x);
    y       = scale;
    step.y  = fx_mul_p(step.y, &y);
    z       = scale;
    step.z  = fx_mul_p(step.z, &z);
    vec_add(&pos, &pos, &step);
    if (vec_ne(&self->pos, &pos)) {
        vec_set(&self->pos, &pos);
        self->pos_changed = 1;
    }
}
