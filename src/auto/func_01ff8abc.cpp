#include "m2c.h"

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
};

enum {
    FX32_ONE      = 0x1000,    /* Q12 1.0 */
    PROBE_SCALE   = 0x4000,    /* Q12 4.0: the probe offset is 4x the velocity */
    MODE_MASK     = 0x3,       /* actors with both of these mode bits clear are skipped */
    EXCLUDE_FLAGS = 0x2200002, /* actors with any of these flag bits set are skipped */
};

typedef volatile u16 vu16;
typedef volatile u32 vu32;
typedef volatile u64 vu64;

typedef struct Vec3 {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct Box {
    s32 v[6];
} Box;

typedef struct Iter {
    s32 w[6];
} Iter;

class Shape {
public:
    virtual void unknown00();
    virtual void getBounds(Box *box); // slot 0x04
    Vec3 pos;                         // 0x04
    u8 pad10[0x16];
    u8 enabled; // 0x26
};

class Actor {
public:
    virtual void unknown00();
    virtual void unknown04();
    virtual s32 unknown08(); // slot 0x08
    u8 pad04[4];
    u32 flags; // 0x08
    u16 mode;  // 0x0c
    u8 pad0e[0x12];
    Shape *shape; // 0x20
    u8 pad24[4];
    s32 stamp; // 0x28
    u8 pad2c[0x34];
    Vec3 vel;  // 0x60
    Vec3 push; // 0x6c
    u8 pad78[0x647];
    u8 b6bf; // 0x6bf
};

typedef struct Link {
    Actor *obj;
    struct Link *next;
} Link;

extern "C" {
extern s32 data_021019d8;
extern M2C_UNK data_021094ac;
extern Vec3 data_020ff9b4;

s32 func_01ff84a8(void *obj);
s32 func_01ff84bc(Shape *a, Shape *b);
void func_02008c44(Vec3 *out, Shape *a, Shape *b, Vec3 *d);
void *func_020555c4(void *arg0, s32 arg1, void *arg2);
void func_02055630(void *arg0);
s32 func_02055660(const void *object);
s32 func_02055684(void *arg0);
}

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

static inline void vec_make(Vec3 *v, s32 x, s32 y, s32 z) {
    v->x = x;
    v->y = y;
    v->z = z;
}

static inline s32 fx_mul(s32 a, s32 b) {
    return (s32) (((s64) a * b + 0x800) >> 12);
}

/* Q12 unit vector d = s / |s| via the hardware divider and square root:
 * (2^56 / t) * sqrt(4t), scaled down by 2^45, with t = |s|^2. Same helper as
 * func_020a9a30.c and other matched files. The 64-bit DIV_RESULT read is a plain
 * u64 load, as in those files. */
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

/* Normalize v in place; a zero vector takes the fallback data_020ff9b4 instead. */
static inline void vec_unit_self(Vec3 *v) {
    if (v->x == 0 && v->y == 0 && v->z == 0) {
        vec_set(v, &data_020ff9b4);
    } else {
        vec_normalize(v, v);
    }
}

/* Temporary built by a constructor: its arguments (the z, y, x products) are
 * evaluated before any push store. */
class FxVec {
public:
    s32 x;
    s32 y;
    s32 z;
    FxVec(s32 ax, s32 ay, s32 az) {
        x = ax;
        y = ay;
        z = az;
    }
};

static inline void operator+=(Vec3 &a, const FxVec &b) {
    a.x += b.x;
    a.y += b.y;
    a.z += b.z;
}

static inline s32 vec_dot3(const Vec3 *a, s32 x, s32 y, s32 z) {
    s64 t = (s64) a->x * x + (s64) a->y * y + (s64) a->z * z;
    return (s32) ((t + 0x800) >> 12);
}

/* For each actor that self's shape would hit when moved ahead along self's
 * velocity, add a vector to that actor's push field. self's shape is probed at
 * pos + 4 * vel and ends at its original position.
 *
 * Without force, a zero velocity returns at once. force also selects the vector
 * when the two shapes still overlap at rest (see the hit branch). Callers:
 * func_0200dafc passes force = 1, func_02022a3c passes 0.
 */
extern "C" void func_01ff8abc(Actor *self, bool force) {
    Vec3 probe;
    Box box;
    Iter it;
    Vec3 dir;
    Vec3 normal;
    Shape *shape;
    bool allow_b6bf;
    Link *node;
    Actor *other;
    Shape *other_shape;
    s32 *zp;

    {
        bool still = self->vel.x == 0 && self->vel.y == 0 && self->vel.z == 0;
        if (still && !force) {
            return;
        }
    }
    shape = self->shape;
    /* Probe offset: 4x the velocity (Q12), applied to self's shape. */
    {
        s32 z   = fx_mul(self->vel.z, PROBE_SCALE);
        s32 y   = fx_mul(self->vel.y, PROBE_SCALE);
        s32 x   = fx_mul(self->vel.x, PROBE_SCALE);
        probe.x = x;
        probe.y = y;
        probe.z = z;
    }
    shape->pos.x += probe.x;
    shape->pos.y += probe.y;
    shape->pos.z += probe.z;
    /* Bounds of the probed shape: its virtual slot 0x04 fills the zeroed box. */
    zp = box.v;
    do {
        *zp++ = 0;
    } while (zp < box.v + 6);
    shape->getBounds(&box);
    /* Actors with b6bf set are considered only when self has a MODE_MASK bit and
     * b6bf set. */
    allow_b6bf = false;
    if ((self->mode & MODE_MASK) && self->b6bf != 0) {
        allow_b6bf = true;
    }
    /* New stamp: each actor is handled at most once per call. */
    data_021019d8++;
    /* Walk the actor lists of the cells of a 16x16 wrapping grid under box words
     * 0-1 and 4-5 (presumably the X and Z ranges). */
    func_020555c4(&it, (s32) &data_021094ac, &box);
    while (!func_02055684(&it)) {
        for (node = (Link *) func_02055660(&it); node != NULL; node = node->next) {
            other = node->obj;
            if (!(other->mode & MODE_MASK)) {
                continue;
            }
            if (other->flags & EXCLUDE_FLAGS) {
                continue;
            }
            if (other == self || other->stamp == data_021019d8) {
                continue;
            }
            /* func_01ff84a8 tests (1 << the s32 at other+0x84) & 3: that value
             * must be 0 or 1. */
            if (!func_01ff84a8(other)) {
                continue;
            }
            if (other->b6bf != 0 && !allow_b6bf) {
                continue;
            }
            other->stamp = data_021019d8;
            other_shape  = other->shape;
            if (other_shape == NULL || other_shape->enabled == 0) {
                continue;
            }
            /* Shape-pair test at the probe position: func_0200aef8 on the
             * positions and the summed sum fields, then a per-kind test. */
            if (!func_01ff84bc(shape, other_shape)) {
                continue;
            }
            shape->pos.x -= probe.x;
            shape->pos.y -= probe.y;
            shape->pos.z -= probe.z;
            if (force && func_01ff84bc(shape, other_shape)) {
                /* Still a hit at rest: push other along the X/Z separation from
                 * self, as a Q12 unit vector. Coincident X/Z positions use +Z or
                 * -Z, chosen by comparing the actors' virtual slot 0x08 results
                 * (self lower: +Z). */
                vec_make(&dir, other_shape->pos.x - shape->pos.x, 0, other_shape->pos.z - shape->pos.z);
                if (dir.z == 0 && dir.x == 0) {
                    if (self->unknown08() < other->unknown08()) {
                        dir.z = FX32_ONE;
                    } else {
                        dir.z = -FX32_ONE;
                    }
                }
                vec_unit_self(&dir);
                other->push.x += dir.x;
                other->push.y += dir.y;
                other->push.z += dir.z;
            } else {
                /* Push other by normal * dot(self->vel, normal) (Q12), with the
                 * normal func_02008c44 returns for the shape pair and the probe
                 * offset (the velocity component along it if it has unit length;
                 * not verified). */
                s32 dot;
                func_02008c44(&normal, shape, other_shape, &probe);
                {
                    s32 nx = normal.x;
                    s32 ny = normal.y;
                    s32 nz = normal.z;
                    dot    = vec_dot3(&self->vel, nx, ny, nz);
                    other->push += FxVec(fx_mul(nx, dot), fx_mul(ny, dot), fx_mul(nz, dot));
                }
            }
            /* Reapply the probe offset for the next candidate. */
            shape->pos.x += probe.x;
            shape->pos.y += probe.y;
            shape->pos.z += probe.z;
        }
        func_02055630(&it);
    }
    /* Remove the probe offset. */
    shape->pos.x -= probe.x;
    shape->pos.y -= probe.y;
    shape->pos.z -= probe.z;
}
