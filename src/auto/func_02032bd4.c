#include "m2c.h"

typedef volatile u64 vu64;
typedef volatile u32 vu32;
typedef volatile u16 vu16;

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec;

typedef struct {
    s32 v;
} Fx;

typedef struct {
    u8 pad_00[0x26];
    u8 field_26;
} Sub2c;

typedef struct {
    u8 pad_00[9];
    u8 kind;
} Sub5fc;

typedef struct {
    u8 pad_00[8];
    s32 field_8;
} Sub604;

typedef struct {
    u8 pad_000[8];
    u32 flags;
    u8 pad_00c[0x2c - 0xc];
    Sub2c *field_2c;
    u8 pad_030[0x60 - 0x30];
    Vec field_60;
    u8 pad_06c[0xb0 - 0x6c];
    Vec pos;
    u8 pad_0bc[0x5e0 - 0xbc];
    u8 field_5e0[0x5fc - 0x5e0];
    Sub5fc *field_5fc;
    u8 pad_600[4];
    Sub604 *field_604;
    u8 pad_608[0x6bb - 0x608];
    u8 field_6bb;
} Obj;

typedef struct {
    u8 pad_00[4];
    Obj *obj;
    u8 pad_08[4];
    u8 field_c;
    u8 pad_0d[3];
    Vec pos;
    s32 field_1c;
    Vec vel;
    Fx scale;
    s32 field_30;
    s32 field_34;
    u16 timer;
    u8 pad_3a[2];
    u8 field_3c;
    u8 field_3d;
} Self;

extern Vec data_020ff9b4;
extern u8 data_0210930c[];

extern void func_0201812c(Obj *a, Vec *b, s32 flag);
extern void func_02017ee4(Obj *self, Vec *vec, s32 blend);
extern s32 func_0200aef8(Vec *a, Vec *b, s32 c);
extern s32 func_0201eec4(Vec *a, Vec *b, s32 c);
extern void func_02004b68(Obj *, s32, Obj *, s32);
extern s32 func_0202c59c(void *arg0, s32 *arg1, s32 *arg2);
extern s32 func_0202cb44(void *arg0);
extern void func_0202cbd4(s32 *arg0, Sub5fc *arg1);
extern void func_0202cbf4(s32 *destination, Sub5fc *object);
extern void func_020675d8(void *arg0, u8 arg1);
extern s32 FX_Div(Fx a,
                  s32 b); // FAKE: numerator typed as one-word struct; by-value arg load gives add r0,sp,#0xc; ldr r0,[r0]

static inline void vec_sub(const Vec *a, const Vec *b, Vec *out) {
    s32 z  = a->z - b->z;
    s32 y  = a->y - b->y;
    s32 x  = a->x - b->x;
    out->x = x;
    out->y = y;
    out->z = z;
}

static inline void vec_set(Vec *d, const Vec *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

/* DS hardware divider / square-root unit (I/O registers and control values). */
enum {
    REG_DIVCNT     = 0x04000280,
    REG_DIV_NUMER  = 0x04000290,
    REG_DIV_DENOM  = 0x04000298,
    REG_DIV_RESULT = 0x040002A0
};
enum {
    REG_SQRTCNT     = 0x040002B0,
    REG_SQRT_RESULT = 0x040002B4,
    REG_SQRT_PARAM  = 0x040002B8
};
enum {
    DIV_MODE_64_64 = 2,
    SQRT_MODE_64   = 1,
    HW_BUSY        = 0x8000
};

/* dst = src / |src| (fx 20.12 unit vector): 2^56 / |src|^2 times sqrt(4 |src|^2), rounded >> 45. */
static inline void vec_normalize(const Vec *src, Vec *dst) {
    s64 t;
    s32 sqrt;
    s64 div;

    t = (s64) src->x * src->x;
    t += (s64) src->y * src->y;
    t += (s64) src->z * src->z;

    (*(vu16 *) REG_DIVCNT)    = DIV_MODE_64_64;
    (*(vu64 *) REG_DIV_NUMER) = 1LL << 56;
    (*(vu64 *) REG_DIV_DENOM) = t;

    (*(vu16 *) REG_SQRTCNT)   = SQRT_MODE_64;
    (*(u64 *) REG_SQRT_PARAM) = t << 2;

    while ((*(vu16 *) REG_SQRTCNT) & HW_BUSY) {
    }
    sqrt = (s32) (*(vu32 *) REG_SQRT_RESULT);

    while ((*(vu16 *) REG_DIVCNT) & HW_BUSY) {
    }
    div = (s64) (*(u64 *) REG_DIV_RESULT);

    div *= sqrt;

    dst->x = (s32) ((div * src->x + (1LL << 44)) >> 45);
    dst->y = (s32) ((div * src->y + (1LL << 44)) >> 45);
    dst->z = (s32) ((div * src->z + (1LL << 44)) >> 45);
}

/* fx 20.12 multiply, rounded to nearest. */
static inline s32 fx_mul(s32 a, Fx b) {
    return (s32) (((s64) a * b.v + 0x800) >> 12);
}
/* Scales each component of v by *len (one by-value Fx copy per component). */
static inline void vec_scale(Vec *v, const Fx *len) {
    Fx x, y, z;

    x    = *len;
    v->x = fx_mul(v->x, x);
    y    = *len;
    v->y = fx_mul(v->y, y);
    z    = *len;
    v->z = fx_mul(v->z, z);
}

/*
 * When obj->field_6bb == 0x54: vel = unit(self->pos - obj->pos) * scale, with y zeroed while obj flag 0x20
 * is set and data_020ff9b4 used for a zero difference. timer is decremented once per call.
 */
void func_02032bd4(Self *self) {
    Obj *obj = self->obj;
    s32 hit;
    s32 special;
    s32 limit;
    s32 cur;
    s32 cur2;
    Fx val;
    s32 below;

    if (obj->field_6bb == 0x54) {
        Vec d;
        vec_sub(&self->pos, &obj->pos, &d);
        vec_set(&self->vel, &d);
        if ((self->obj->flags & 0x20) == 0x20) {
            self->vel.y = 0;
        }
        if (self->vel.x == 0 && self->vel.y == 0 && self->vel.z == 0) {
            vec_set(&self->vel, &data_020ff9b4);
        } else {
            vec_normalize(&self->vel, &self->vel);
        }
        vec_scale(&self->vel, &self->scale);
    }

    func_0201812c(self->obj, &self->pos, 0);
    {
        Obj *o   = self->obj;
        s32 busy = 0;
        if (o->field_2c != 0 && o->field_2c->field_26 != 0) {
            busy = 1;
        }
        if (busy == 0) {
            func_02017ee4(o, &self->vel, 0x4b);
            if (self->vel.y != 0) {
                self->obj->field_60.y = self->vel.y;
            }
        }
    }

    {
        Obj *o = self->obj;
        s32 res;
        s32 done = 0;
        if ((o->flags & 0x20) == 0x20) {
            res = func_0201eec4(&o->pos, &self->pos, self->field_1c);
        } else {
            res = func_0200aef8(&o->pos, &self->pos, self->field_1c);
        }
        if (res != 0) {
            done = 1;
        }
        if (self->timer-- == 0 || done != 0) {
            if (self->field_3c != 0) {
                func_020675d8((u8 *) *(void **) ((u8 *) *(void **) (*(u8 **) &data_0210930c[0x68] + 0x38c) + 4) + 0x28,
                              self->field_3c);
            }
            self->obj->flags |= 0x200;
            func_02004b68(self->obj, 1, self->obj, 2);
            self->field_c |= 1;
            return;
        }
    }

    if (self->field_3d == 0 || self->obj->field_5fc == 0) {
        return;
    }
    if (func_0202cb44(self->obj->field_5e0) == 0) {
        return;
    }
    {
        Sub5fc *s = self->obj->field_5fc;
        special   = 0;
        limit     = self->obj->field_604->field_8;
        {
            u32 k = s->kind - 4;
            if (k <= 16 && ((1 << k) & 0x18001)) {
                special = 1;
            }
        }
        hit = 0;
        func_0202cbd4(&cur, s);
        if (limit > cur) {
        } else if (special != 0) {
            hit = 1;
        }
        if (hit == 0) {
            s32 res;
            func_0202cbf4(&cur2, self->obj->field_5fc);
            below = limit < cur2;
            res   = func_0202c59c(self->obj->field_5e0, &self->field_30, &self->field_34);
            if (below) {
                if (res != 0) {
                    if (special != 0) {
                        hit = 1;
                    }
                } else if (special != 0) {
                    func_0202cbf4(&val.v, self->obj->field_5fc);
                    if (FX_Div(val, 0x2000) - limit > 0) { // FAKE: struct-typed numerator, see FX_Div decl
                        hit = 1;
                    }
                }
            }
        }
    }
    if (hit == 0) {
        return;
    }
    self->obj->flags |= 0x200;
    self->field_c |= 1;
    vec_set(&self->obj->field_60, &data_020ff9b4);
}
