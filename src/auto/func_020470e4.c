#include "m2c.h"

typedef struct {
    s32 x, y, z;
} Vec3;

extern void func_02007f24(s32 *, double);
extern s32 func_0205e80c(s32, s32, s32);
extern s32 func_0205ab44(s32);
extern s32 func_0205ab58(s32);
extern s32 func_02062a7c(s32, s32);
extern void func_020bbeb0(void *, s32);
extern void func_020bbf78(void *, s32, s32, s32);
extern void *func_020bbfe8(void *);
extern void func_020bc4cc(void *, s32);
typedef struct {
    u16 id;
    u8 pad[0x22];
} IdEntry;
extern IdEntry data_020f6bae[];
extern u16 data_020ed2dc[];

typedef struct {
    u8 pad0[0x24];
    Vec3 pos;
    u8 pad30[0x40 - 0x30];
    u8 f40;
    u8 pad41[0x45 - 0x41];
    u8 f45;
    u8 pad46[0x384 - 0x46];
    u32 f384;
} Child;

typedef struct {
    u8 pad0[0x40];
    u8 f40;
    u8 pad41[0x64 - 0x41];
    s32 f64;
    u8 pad68[0xb0 - 0x68];
    Vec3 target;
    u8 padbc[0x6c3 - 0xbc];
    u8 f6c3;
} Inner;

typedef struct {
    u8 pad0[4];
    Inner *inner;
} Owner;

static inline int vec_ne(const Vec3 *a, const Vec3 *b) {
    if (a->x != b->x || a->y != b->y || a->z != b->z) {
        return 1;
    }
    return 0;
}

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

typedef struct {
    s32 v;
} Fx;

static inline s32 fxmulf(s32 a, Fx b) {
    return (s32) (((s64) a * (s64) b.v + 0x800) >> 12);
}

static inline s32 fxmul(s32 a, s32 b) {
    return (s32) (((s64) a * (s64) b + 0x800) >> 12);
}

static inline void vec_init(Vec3 *v, s32 x, s32 y, s32 z) {
    v->x = x;
    v->y = y;
    v->z = z;
}

/* fx32 constants (20.12 fixed point, see fxmul's >> 12). */
enum {
    FX32_2_0 = 0x2000
};
enum {
    FX32_MINUS_5_0 = -0x5000
};

/*
 * arg1 != 0: inner->f64 = 1.55 in fx32 (func_02007f24 writes a double as fx32).
 * If inner->f40 is set, a 0x3a0-byte child is allocated (func_0205e80c, null
 * checked) and initialised (func_020bbfe8), given func_0205ab44(0xc38) and an id:
 * data_020f6bae[inner->f6c3].id, or for f6c3 == 0x17 one of the six
 * data_020ed2dc ids picked by func_02062a7c(0, 5). It is placed at inner->target
 * raised by 2.0 in y (f45 set when the position changed), then
 * func_020bc4cc(child, 1) and bit 31 of f384 are set.
 * arg1 == 0: a negative inner->f64 is scaled by 1.75 and clamped from below to
 * 2.0 * -5.0 (-10.0).
 */
void func_020470e4(Owner *self, s32 arg1) {
    s32 v;
    Child *c;
    s32 id;
    u16 tab[6];
    Vec3 p;
    Fx a, b, d;
    s32 *pb;
    s32 *pd;
    s32 lim;

    if (arg1 != 0) {
        func_02007f24(&v, 1.55);
        self->inner->f64 = v;
        if (self->inner->f40 == 0) {
            return;
        }
        c = (Child *) func_0205e80c(0x3a0, 3, 4);
        if (c != 0) {
            c = func_020bbfe8(c);
        }
        func_020bbeb0(c, func_0205ab44(0xc38));
        if (self->inner->f6c3 != 0x17) {
            id = data_020f6bae[self->inner->f6c3].id;
            func_020bbf78(c, id, func_0205ab58(id), 0);
        } else {
            tab[0] = data_020ed2dc[0];
            tab[1] = data_020ed2dc[1];
            tab[2] = data_020ed2dc[2];
            tab[3] = data_020ed2dc[3];
            tab[4] = data_020ed2dc[4];
            tab[5] = data_020ed2dc[5];
            id     = tab[func_02062a7c(0, 5)];
            func_020bbf78(c, id, func_0205ab58(id), 0);
        }
        vec_init(&p, self->inner->target.x, self->inner->target.y + FX32_2_0, self->inner->target.z);
        if (vec_ne(&c->pos, &p)) {
            vec_set(&c->pos, &p);
            c->f45 = 1;
        }
        func_020bc4cc(c, 1);
        c->f384 |= 0x80000000;
    } else {
        if (self->inner->f64 >= 0) {
            return;
        }
        func_02007f24(&a.v, 1.75);
        self->inner->f64 = fxmulf(self->inner->f64, a);
        func_02007f24(&b.v, 2.0);
        pb = &b.v;
        if (pb) { // FAKE: always-true null test keeps &b in a register and lim defined before the f64 load
            lim = fxmul(*pb, FX32_MINUS_5_0);
        }
        if (self->inner->f64 < lim) {
            func_02007f24(&d.v, 2.0);
            pd = &d.v;
            if (pd) { // FAKE: always-true null test keeps &d in a register
                self->inner->f64 = fxmul(*pd, FX32_MINUS_5_0);
            }
        }
    }
}
