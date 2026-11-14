/* Reconstructed from USA ARM9 at 0x0204da60 (376 bytes).
 *
 * v = func_0200d6d0(b). If v is zero, c = data_020ff9b4; otherwise
 * c = v + a * (data_020fe498.y - dot(v, a)). Then q = -(a.x * b.x + a.z * b.z)
 * / a.y (the y that makes (b.x, q, b.z) orthogonal to a) and
 * out = (c + (b.x, q, b.z)) >> 1. Products are 20.12 fixed point, rounded by
 * adding one half before the shift.
 */
#include "m2c.h"

enum {
    FX_FRAC_BITS = 12
};
enum {
    FX_HALF = 0x800
};

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct {
    s32 v;
} Word;

extern Vec3 data_020ff9b4;
extern Vec3 data_020fe498;

void func_0200d6d0(Vec3 *, const Vec3 *, Word);
s32 FX_Div(s32, s32);

static inline s32 vec_is_zero(const Vec3 *p) {
    s32 xy;
    s32 r;
    r  = 0;
    xy = 0;
    if (!(p->x != 0 || p->y != 0)) {
        xy = 1;
    }
    if (xy) {
        if (p->z == 0) {
            r = 1;
        }
    }
    return r;
}

void func_0204da60(Vec3 *out, const Vec3 *b, const Vec3 *a) {
    Word w;
    Vec3 v;
    s64 t;
    s32 pz;
    s32 dot;
    s32 cx;
    s32 cy;
    s32 cz;
    s32 q;
    s32 py;
    s64 u;

    w.v = 0;
    func_0200d6d0(&v, b, w);
    if (vec_is_zero(&v)) {
        cx = data_020ff9b4.x;
        cy = data_020ff9b4.y;
        cz = data_020ff9b4.z;
    } else {
        t   = (s64) v.x * a->x + (s64) v.y * a->y + (s64) v.z * a->z;
        dot = (s32) ((t + FX_HALF) >> FX_FRAC_BITS);
        dot = data_020fe498.y - dot;
        t   = ((s64) a->z * dot + FX_HALF) >> FX_FRAC_BITS;
        pz  = (s32) t;
        // FAKE: y product held in the shared s64 t (redefined below) so mwcc does not merge it into cy
        t  = ((s64) a->y * dot + FX_HALF) >> FX_FRAC_BITS;
        py = (s32) t;
        // FAKE: x rounds in a split statement (z/y are one-line); the uniform spelling changes schedule and registers
        t = (s64) a->x * dot;
        t += FX_HALF;
        cx = v.x + (s32) (t >> FX_FRAC_BITS);
        cy = v.y + py;
        cz = v.z + pz;
    }
    u = (s64) a->z * b->z + FX_HALF;
    q = (s32) (u >> FX_FRAC_BITS);
    u = (s64) a->x * b->x;
    q = FX_Div(-((s32) ((u + FX_HALF) >> FX_FRAC_BITS) + q), a->y);
    cz += b->z;
    cy += q;
    cx += b->x;
    out->x = cx >> 1;
    out->y = cy >> 1;
    out->z = cz >> 1;
}
