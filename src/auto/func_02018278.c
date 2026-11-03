#include "m2c.h"

/* FX_SinCosTable-style table: (sin, cos) s16 pairs in fx 1.3.12, one pair per 16 angle units. */
extern const s16 data_020f88c4[];
/* Fills the starting angle for obj (first argument) before the random turn is applied. */
extern void func_0201811c(u16 *, void *, s32, s32);
/* Random integer in [lo, hi]. */
extern s32 func_02062a7c(s32, s32);

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} V3;
/* 16-bit binary angle: 0x10000 units per full turn. */
typedef struct {
    u16 v;
} Ang;

enum {
    ANG_FULL_TURN    = 0x10000,
    DEGREES_PER_TURN = 360
};

/* FAKE: ang_get() is a pure pass-through; it only sets the order of the inliner stack temporaries */
static inline Ang ang_get(const Ang *p) {
    return *p;
}
static inline u16 ang_add_v(Ang *a, Ang b) {
    Ang r;
    r.v = a->v + b.v;
    return ang_get(&r).v;
}
static inline u16 ang_sub_v(Ang *a, Ang b) {
    Ang r;
    r.v = a->v - b.v;
    return ang_get(&r).v;
}

/* Rotates out (x, z) about the Y axis by ang using the sin/cos table, fx 20.12 rounding. */
static inline void rot_y(V3 *out, u16 ang) {
    s32 s, c, x, z, nx, t;
    s  = data_020f88c4[(ang >> 4) * 2];
    c  = data_020f88c4[(ang >> 4) * 2 + 1];
    x  = out->x;
    z  = out->z;
    t  = (s32) (((s64) z * s + 0x800) >> 12);
    nx = (s32) (((s64) x * c + 0x800) >> 12);
    nx += t;
    t      = (s32) (((s64) x * s + 0x800) >> 12);
    t      = (s32) (((s64) z * c + 0x800) >> 12) - t;
    out->x = nx;
    out->z = t;
}

/*
 * Picks a point around obj: starts from obj's angle, turns it by a random
 * [lo, hi] degrees to one side chosen at random, then places out at a random
 * [rlo, rhi] distance (whole fx 20.12 units) along that heading, offset by the
 * x/y/z triple at obj+0xb0.
 */
void func_02018278(V3 *out, void *obj, s32 arg2, s32 lo, s32 hi, s32 rlo, s32 rhi) {
    Ang base;
    s32 deg, dist;

    func_0201811c(&base.v, obj, arg2, 0);
    deg = func_02062a7c(lo, hi);
    if (func_02062a7c(0, 1) == 1) {
        base.v = ang_sub_v(&base, (Ang) {deg * ANG_FULL_TURN / DEGREES_PER_TURN});
    } else {
        base.v = ang_add_v(&base, (Ang) {deg * ANG_FULL_TURN / DEGREES_PER_TURN});
    }
    dist   = func_02062a7c(rlo, rhi);
    out->x = 0;
    out->y = 0;
    out->z = dist << 12;
    rot_y(out, base.v);
    out->x += M2C_FIELD(obj, s32 *, 0xb0);
    out->y += M2C_FIELD(obj, s32 *, 0xb4);
    out->z += M2C_FIELD(obj, s32 *, 0xb8);
}
