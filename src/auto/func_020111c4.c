#include "m2c.h"

extern s32 data_020ff9b0;
void func_02011354(s32 *out, s32 *v);

typedef struct {
    s32 v;
} Fx;

typedef struct {
    Fx x, y, z;
} Vec3;

/* fixed-point values with 12 fraction bits */
enum {
    FX_FRAC_BITS = 12
};
enum {
    FX_FRAC_MASK = 0xFFF
};

static inline Fx *fx_max(Fx *r, const Fx *a, const Fx *b) {
    if (a->v < b->v) {
        r->v = b->v;
        return r;
    }
    r->v = a->v;
    return r;
}

static inline Fx *fx_min(Fx *r, const Fx *a, const Fx *b) {
    if (a->v < b->v) {
        r->v = a->v;
        return r;
    }
    r->v = b->v;
    return r;
}

static inline s32 vec_max(const Vec3 *v) {
    Fx r, m, z, y, x;
    s32 t;
    z = v->z;
    y = v->y;
    x = v->x;
    m = *fx_max(&r, &y, &z);
    t = m.v;
    if (x.v >= t) {
        t = x.v;
    }
    return t;
}

static inline s32 vec_min(const Vec3 *v) {
    Fx r, m, z, y, x;
    s32 t;
    z = v->z;
    y = v->y;
    x = v->x;
    m = *fx_min(&r, &y, &z);
    t = x.v;
    if (t >= m.v) {
        t = m.v;
    }
    return t;
}

/* squared length from the whole parts (each truncated toward zero)
 * The six `& 0xFFFFF000LL` masks below are 64-bit constants on s32 values (FAKE, marked on each line): the
 * 64-bit and makes the mask build as mov rZ,#0; sub rM,rZ,#0x1000 like the target (a 32-bit mask
 * constant is not built this way; `~0xFFF` as LL/ull judges 29). */
static inline s32 vec_sq_whole(const Vec3 *v) {
    Fx tz, tx, ty;
    Fx *pz, *px, *py;
    s32 a, b, c;
    if (v->z.v < 0) {
        tz.v = (v->z.v + FX_FRAC_MASK) & 0xFFFFF000LL; // FAKE: 64-bit mask, forces the mov #0; sub #0x1000 mask
        pz   = &tz;
    } else {
        tz.v = v->z.v & 0xFFFFF000LL; // FAKE: 64-bit mask, forces the mov #0; sub #0x1000 mask
        pz   = &tz;
    }
    if (v->x.v < 0) {
        tx.v = (v->x.v + FX_FRAC_MASK) & 0xFFFFF000LL; // FAKE: 64-bit mask, forces the mov #0; sub #0x1000 mask
        px   = &tx;
    } else {
        tx.v = v->x.v & 0xFFFFF000LL; // FAKE: 64-bit mask, forces the mov #0; sub #0x1000 mask
        px   = &tx;
    }
    if (v->y.v < 0) {
        ty.v = (v->y.v + FX_FRAC_MASK) & 0xFFFFF000LL; // FAKE: 64-bit mask, forces the mov #0; sub #0x1000 mask
        py   = &ty;
    } else {
        ty.v = v->y.v & 0xFFFFF000LL; // FAKE: 64-bit mask, forces the mov #0; sub #0x1000 mask
        py   = &ty;
    }
    c = py->v >> FX_FRAC_BITS;
    b = px->v >> FX_FRAC_BITS;
    a = pz->v >> FX_FRAC_BITS;
    return (b * b + c * c + a * a) << FX_FRAC_BITS;
}

/* squared length of v: whole-part form when any component reaches the
   data_020ff9b0 limit, otherwise the full-precision func_02011354 */
void func_020111c4(s32 *out, Vec3 *v) {
    if (vec_max(v) >= data_020ff9b0 || vec_min(v) <= -data_020ff9b0) {
        *out = vec_sq_whole(v);
    } else {
        func_02011354(out, (s32 *) v);
    }
}
