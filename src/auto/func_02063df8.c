#include "m2c.h"

typedef volatile u64 vu64;
typedef volatile u32 vu32;
typedef volatile u16 vu16;

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct {
    s32 v;
} Fx;

typedef struct {
    Vec3 *out;
    Vec3 pos;
    Fx rx;
    s32 ry;
    s32 rz;
    u8 mode;
    u8 slot;
} Follower;

/* fixed-point values with 12 fraction bits */
enum {
    FX_FRAC_BITS = 12
};
enum {
    FX_FRAC_MASK = 0xFFF
};

extern s32 func_0200aef8(Vec3 *a0, Vec3 *a1, Fx a2);
extern s32 func_0201eec4(Vec3 *a0, Vec3 *a1, Fx a2);

extern const Vec3 data_020ff9b4;
extern u32 data_02104428[];
extern u8 data_02104480[];
extern u32 data_02104484[];

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

static inline s64 vec_dot64(const Vec3 *a, const Vec3 *b) {
    s64 t = (s64) a->x * b->x;
    t += (s64) a->y * b->y;
    t += (s64) a->z * b->z;
    return t;
}

static inline void vec_normalize(Vec3 *v) {
    s64 t;
    s32 sqrt;
    s64 div;

    if (v->x == 0 && v->y == 0 && v->z == 0) {
        vec_set(v, &data_020ff9b4);
        return;
    }
    t = vec_dot64(v, v);

    /* NDS hardware divider (0x04000280..) and square-root unit (0x040002B0..) */
    (*(vu16 *) 0x04000280) = 2;
    (*(vu64 *) 0x04000290) = 1LL << 56;
    (*(vu64 *) 0x04000298) = t;

    (*(vu16 *) 0x040002B0) = 1;
    (*(u64 *) 0x040002B8)  = t << 2;

    while ((*(vu16 *) 0x040002B0) & 0x8000) {
    }
    sqrt = (s32) (*(vu32 *) 0x040002B4);

    while ((*(vu16 *) 0x04000280) & 0x8000) {
    }
    div = (s64) (*(u64 *) 0x040002A0);

    div *= sqrt;

    v->x = (s32) ((div * v->x + (1LL << 44)) >> 45);
    v->y = (s32) ((div * v->y + (1LL << 44)) >> 45);
    v->z = (s32) ((div * v->z + (1LL << 44)) >> 45);
}

static inline void vec_add(Vec3 *out, const Vec3 *a, const Vec3 *b) {
    s32 z  = a->z + b->z;
    s32 y  = a->y + b->y;
    s32 x  = a->x + b->x;
    out->x = x;
    out->y = y;
    out->z = z;
}

static inline Vec3 *vec_sub(Vec3 *out, const Vec3 *a, const Vec3 *b) {
    s32 z  = a->z - b->z;
    s32 y  = a->y - b->y;
    s32 x  = a->x - b->x;
    out->x = x;
    out->y = y;
    out->z = z;
    return out;
}

static inline void vec_sub_xz(Vec3 *out, const Vec3 *a, const Vec3 *b) {
    s32 z  = a->z - b->z;
    s32 x  = a->x - b->x;
    out->x = x;
    out->y = 0;
    out->z = z;
}

static inline void vec_copy(Vec3 *d, const Vec3 *s) {
    d->x = s->x;
    d->y = s->y;
    d->z = s->z;
}

/* v *= s per component; each component takes its own copy of *s (the target
   reloads obj->rx after every store and keeps three dead copies at
   sp+0xc/0x10/0x14 in x, y, z order) */
static inline void vec_scale(Vec3 *v, const Fx *s) {
    Fx sx;
    Fx sy;
    Fx sz;
    sx   = *s;
    v->x = fx_mul(v->x, sx.v);
    sy   = *s;
    v->y = fx_mul(v->y, sy.v);
    sz   = *s;
    v->z = fx_mul(v->z, sz.v);
}

static inline void clamp(s32 *v, s32 c, s32 r) {
    if (*v < c - r) {
        *v = c - r;
    } else if (*v > c + r) {
        *v = c + r;
    }
}

/* squared length from the whole parts (each truncated toward zero), same
   shape as the helper in func_020111c4; the target builds the z >= 0 mask as a
   32-bit constant (mov #0x1000; rsb) and every other mask as the 64-bit one
   (mov #0; sub #0x1000). The five `& 0xFFFFF000LL` masks are FAKE (marked on
   each line): a 64-bit constant on an s32 value, used only to get that mask
   synthesis. */
static inline s32 vec_sq_whole(const Vec3 *v) {
    Fx tz, tx, ty;
    Fx *pz;
    Fx *px;
    Fx *py;
    s32 a, b, c;

    if (v->z < 0) {
        tz.v = (v->z + FX_FRAC_MASK) & 0xFFFFF000LL; // FAKE: 64-bit mask, forces the mov #0; sub #0x1000 mask
        pz   = &tz;
    } else {
        tz.v = v->z & 0xFFFFF000;
        pz   = &tz;
    }
    if (v->x < 0) {
        tx.v = (v->x + FX_FRAC_MASK) & 0xFFFFF000LL; // FAKE: 64-bit mask, forces the mov #0; sub #0x1000 mask
        px   = &tx;
    } else {
        tx.v = v->x & 0xFFFFF000LL; // FAKE: 64-bit mask, forces the mov #0; sub #0x1000 mask
        px   = &tx;
    }
    if (v->y < 0) {
        ty.v = (v->y + FX_FRAC_MASK) & 0xFFFFF000LL; // FAKE: 64-bit mask, forces the mov #0; sub #0x1000 mask
        py   = &ty;
    } else {
        ty.v = v->y & 0xFFFFF000LL; // FAKE: 64-bit mask, forces the mov #0; sub #0x1000 mask
        py   = &ty;
    }
    c = py->v >> FX_FRAC_BITS;
    b = px->v >> FX_FRAC_BITS;
    a = pz->v >> FX_FRAC_BITS;
    return b * b + c * c + a * a;
}

/* new position for *obj->out from obj->pos and *target, by obj->mode:
   0: *target, or obj->pos + obj->rx along (target - pos) when func_0200aef8
      returns 0; 1: *target, with x/z set to obj->pos + obj->rx along the x/z
      direction when func_0201eec4 returns 0, and y clamped to pos.y +- ry;
   2: *target clamped to obj->pos +- (rx, ry, rz); other modes keep *obj->out.
   With obj->slot 0 it is always written back; otherwise the write and the
   per-slot update of data_02104480/data_02104484 are skipped while the byte at
   data_02104428+0x6c equals the slot's stored byte and the whole-part squared
   distance to *target is not below the slot's stored value */
void func_02063df8(Follower *obj, Vec3 *target) {
    Vec3 cur;
    Vec3 dir0;
    Vec3 res1;
    Vec3 dir1;
    Vec3 res2;
    Vec3 res0;
    Vec3 diff;
    s32 len2;

    if (obj->out == NULL) {
        return;
    }
    vec_copy(&cur, obj->out);
    switch (obj->mode) {
        case 0:
            if (func_0200aef8(&obj->pos, target, obj->rx) == 0) {
                vec_sub(&dir0, target, &obj->pos);
                vec_normalize(&dir0);
                vec_scale(&dir0, &obj->rx);
                vec_add(&res0, &obj->pos, &dir0);
                vec_set(&cur, &res0);
            } else {
                vec_set(&cur, target);
            }
            break;
        case 1:
            vec_copy(&res1, target);
            if (func_0201eec4(&obj->pos, target, obj->rx) == 0) {
                vec_sub_xz(&dir1, target, &obj->pos);
                vec_normalize(&dir1);
                {
                    s32 m  = fx_mul(dir1.x, obj->rx.v);
                    res1.x = obj->pos.x;
                    res1.x += m;
                }
                {
                    s32 m  = fx_mul(dir1.z, obj->rx.v);
                    res1.z = obj->pos.z;
                    res1.z += m;
                }
            }
            clamp(&res1.y, obj->pos.y, obj->ry);
            vec_set(&cur, &res1);
            break;
        case 2:
            vec_copy(&res2, target);
            clamp(&res2.x, obj->pos.x, obj->rx.v);
            clamp(&res2.y, obj->pos.y, obj->ry);
            clamp(&res2.z, obj->pos.z, obj->rz);
            vec_set(&cur, &res2);
            break;
    }

    if (obj->slot == 0) {
        vec_set(obj->out, &cur);
        return;
    }
    len2 = vec_sq_whole(vec_sub(&diff, &cur, target));
    if (((u8 *) data_02104428)[0x6c] == data_02104480[obj->slot] && (u32) len2 >= data_02104484[obj->slot]) {
        return;
    }
    vec_set(obj->out, &cur);
    data_02104484[obj->slot] = len2;
    data_02104480[obj->slot] = ((u8 *) data_02104428)[0x6c];
}
