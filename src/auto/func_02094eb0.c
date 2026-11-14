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
    s32 d;
    Vec3 n;
} Plane;

typedef struct {
    Vec3 pos;
    Vec3 vel;
    s32 floor;
    s32 unk1c;
    u16 flags;
} Ent;

typedef struct {
    u8 pad[0x68];
    u8 *world;
} Glob;

extern Glob data_0210930c;
extern const Vec3 data_020ff9b4;

void func_02007f24(Fx *out, double v);
void func_0204c7b0(Plane *dest, void *src);
void func_0204f784(Fx *out, Ent *e, Plane *p);
void func_0204f9bc(s32 *out, void *a1, Vec3 *a2, Vec3 *a3, s32 a4);

static inline s32 fx_mul(s32 a, s32 b) {
    return (s32) (((s64) a * b + 0x800) >> 12);
}

// FAKE: int (not s32 like dot and Fx.v) in fx_mul_p/fxm; the int/s32 mismatch gives the target's dot colouring
static inline int fx_mul_p(int a, const Fx *b) {
    return (s32) (((s64) a * b->v + 0x800) >> 12);
}

static inline int fxm(int a, Fx b) { // FAKE: int types, see fx_mul_p
    return fx_mul_p(a, &b);
}

static inline void fx_scale(s32 *p, Fx k) {
    *p = fxm(*p, k);
}

static inline s32 fx_sqv(Fx b) {
    return fx_mul_p(b.v, &b);
}

static inline s32 fx_sq(const Fx *a) {
    return fx_sqv(*a);
}

static inline Fx fx(s32 v) {
    Fx r;
    r.v = v;
    return r;
}

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

static inline void vec_sub(Vec3 *out, const Vec3 *a, const Vec3 *b) {
    Vec3 t;
    s32 z = a->z - b->z;
    s32 y = a->y - b->y;
    s32 x = a->x - b->x;
    t.x   = x;
    t.y   = y;
    t.z   = z;
    vec_set(out, &t);
}

static inline void vec_add(Vec3 *out, const Vec3 *a, const Vec3 *b) {
    s32 z  = a->z + b->z;
    s32 y  = a->y + b->y;
    s32 x  = a->x + b->x;
    out->x = x;
    out->y = y;
    out->z = z;
}

static inline s32 poly_height(Ent *e, void *poly) {
    Plane pl;
    Fx h;
    Fx *hp;
    s32 v;
    func_0204c7b0(&pl, poly);
    func_0204f784(&h, e, &pl);
    hp = &h;
    /* A getter helper with the same test folds to ldr r0,[sp,#0x1c]. */
    // FAKE: always-true null test on &h keeps the address in a register (add r0,sp,#0x1c; ldr r0,[r0,#0])
    if (hp) {
        v = hp->v;
    }
    return v;
}

static inline s32 fx_from(double d) {
    Fx r;
    func_02007f24(&r, d);
    return r.v;
}

static inline u32 nz2(const Vec3 *v) {
    return v->x != 0 || v->y != 0;
}

static inline u32 vec_nonzero(const Vec3 *v) {
    return nz2(v) || v->z != 0;
}

enum {
    ENT_FLAG_FLOOR = 0x2,    /* e->floor holds a surface height (set here) */
    ENT_FLAG_REST  = 0x4,    /* set here on landing or zero velocity; skips the update */
    FX_TWO         = 0x2000, /* 2.0 in Q12 */
    FALL_STEP      = 0x599,  /* subtracted from vel.y each update (about 0.35 in Q12) */
    VEL_DAMP       = 0xe66,  /* 0.9 in Q12 */
    FLOOR_LIFT     = 0x80    /* added to the surface height for e->floor */
};

/*
 * One movement step for entity e (func_02094ca0 calls it for each node of the list at self + 4).
 * Q12 fixed point throughout; y is the vertical axis (func_0204f784 solves the plane for y).
 *  - Does nothing while ENT_FLAG_REST is set.
 *  - With a nonzero velocity, casts the segment pos -> pos + vel against the world collision
 *    (func_0204f9bc on data_0210930c.world + 0x384). When hit[1] < 2.0, builds the plane of
 *    the hit polygon hit[0] (func_0204c7b0), reflects the velocity about its normal
 *    (vel -= n * 2 * dot(vel, n)), stores the plane height under e plus FLOOR_LIFT in
 *    e->floor and sets ENT_FLAG_FLOOR.
 *  - Below vel.y 2.0, subtracts FALL_STEP from vel.y, clamped at -2.0.
 *  - Scales each nonzero velocity component by 0.9 and zeroes it once its square drops
 *    below 0.1 * 0.1 (func_02007f24 converts the double to Q12).
 *  - Clears vel.y when self byte 0xf7 is nonzero, then adds the velocity to the position.
 *  - With ENT_FLAG_FLOOR set and pos.y at or below e->floor, snaps pos.y to the floor,
 *    loads vel from data_020ff9b4 and sets ENT_FLAG_REST; a zero velocity also sets it.
 */
void func_02094eb0(u8 *self, Ent *e) {
    Vec3 target;
    Plane plane;
    Vec3 proj;
    s32 hit[2];
    s32 dot;
    s32 i;
    s32 *p;

    if (e->flags & ENT_FLAG_REST) {
        return;
    }
    if (vec_nonzero(&e->vel)) {
        vec_add(&target, &e->pos, &e->vel);
        func_0204f9bc(hit, *(void **) (data_0210930c.world + 0x384), &e->pos, &target, 0);
        if (hit[1] < FX_TWO) {
            func_0204c7b0(&plane, (void *) hit[0]);
            dot = (s32) (((s64) e->vel.x * plane.n.x + (s64) e->vel.y * plane.n.y + (s64) e->vel.z * plane.n.z + 0x800) >> 12);
            fx_scale(&dot, fx(FX_TWO));
            proj.x = fx_mul(plane.n.x, dot);
            proj.y = fx_mul(plane.n.y, dot);
            proj.z = fx_mul(plane.n.z, dot);
            vec_sub(&proj, &e->vel, &proj);
            vec_set(&e->vel, &proj);
            e->floor = poly_height(e, (void *) hit[0]) + FLOOR_LIFT;
            e->flags |= ENT_FLAG_FLOOR;
        }
    }
    if (e->vel.y < FX_TWO) {
        e->vel.y += -FALL_STEP;
        if (e->vel.y < -FX_TWO) {
            e->vel.y = -FX_TWO;
        }
    }
    // FAKE: walks p for the scale but indexes (&e->vel.x)[i] for the tests; this mix sets the loop's addressing
    p = &e->vel.x;
    for (i = 0; i < 3; i++, p++) {
        if ((&e->vel.x)[i] != 0) {
            fx_scale(p, fx(VEL_DAMP));
            if (fx_sq((Fx *) &e->vel.x + i) < fx_from(0.1 * 0.1)) {
                (&e->vel.x)[i] = 0;
            }
        }
    }
    if (self[0xf7] != 0) {
        e->vel.y = 0;
    }
    e->pos.x += e->vel.x;
    e->pos.y += e->vel.y;
    e->pos.z += e->vel.z;
    if ((e->flags & ENT_FLAG_FLOOR) && e->pos.y <= e->floor) {
        e->pos.y = e->floor;
        vec_set(&e->vel, &data_020ff9b4);
        e->flags |= ENT_FLAG_REST;
    }
    if (e->vel.x == 0 && e->vel.y == 0 && e->vel.z == 0) {
        e->flags |= ENT_FLAG_REST;
    }
}
