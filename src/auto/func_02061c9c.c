#include "m2c.h"

/* func_02061c9c (0x02061c9c, 624 bytes)
 *
 * Initialiser for a 0x2c byte object that func_020610e4 allocates and
 * forwards here with its remaining arguments; returns the object.
 *
 * 1. Clears the two Vec3 fields at 0x10/0x1c and the word at 0x28.
 * 2. Bails out (object left otherwise untouched) once the global counter
 *    data_021042c4 exceeds MAX_INSTANCES; the counter is incremented below
 *    on every successful initialisation.
 * 3. Allocates a one-entry array of 0x2c byte Elem records
 *    (func_0205e818, then func_020e4600 runs func_02061f0c, which zeroes
 *    a record, over each entry) and fills entry 'cur' from the arguments:
 *    two Vec3 copies, a3/a4, FX_Div(FX_ONE, a3) (fx12 reciprocal) and,
 *    via the hardware divider, (FX_ONE * FX_ONE) / (a3 - a4).
 *    The divide is started early and its result collected at the end.
 * 4. Copies vecA into the object, registers the object with
 *    data_027e03a0 (virtual slot 2) and with data_027e0410
 *    (func_02055700).
 *
 * The fourth argument is a one-word structure passed by value (see
 * func_020610e4), hence the "push {r0, r1, r2, r3}" home area.
 */

typedef struct {
    s32 x, y, z;
} Vec3;

typedef struct {
    s32 v;
} Unk_020610e4;

typedef struct {
    Vec3 f00;
    s32 f0C;
    s32 f10;
    Vec3 f14;
    s32 f20;
    s32 f24;
    s32 f28;
} Elem;

typedef struct {
    Elem *elems;
    u16 count;
    u16 cur;
    s32 f08;
    u16 f0C;
    u8 f0E;
    Vec3 f10;
    Vec3 f1C;
    s32 f28;
} Obj;

typedef struct Mgr Mgr;
typedef struct {
    void *slot0;
    void *slot1;
    void (*add)(Mgr *, void *);
} MgrVtbl;
struct Mgr {
    MgrVtbl *vt;
};

extern u8 data_021042c4;
extern Mgr data_027e03a0;
extern u8 data_027e0410;

extern void *func_0205e818(s32, s32, s32);
extern void *func_020e4600(void *, s32, s32, s32, void *, s32);
extern void func_02061f0c(void *);
extern s32 FX_Div(s32, s32);
extern void func_02055700(void *obj, void *p);

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

static inline void vec_clear(Vec3 *v) {
    Vec3 zero;
    Vec3 *p = &zero;

    p->x = 0;
    p->y = 0;
    p->z = 0;
    vec_set(v, p);
}

/* NDS hardware divider: DIVCNT mode 0 (32/32), 64-bit denominator store. */
enum {
    REG_DIVCNT     = 0x4000280,
    REG_DIV_NUMER  = 0x4000290,
    REG_DIV_DENOM  = 0x4000298,
    REG_DIV_RESULT = 0x40002a0,
    DIVCNT_BUSY    = 0x8000,
};

enum {
    FX_ONE        = 0x1000,
    MAX_INSTANCES = 100,
};

static inline void div_start(u32 numer, u32 denom) {
    *(volatile u16 *) REG_DIVCNT    = 0;
    *(volatile u32 *) REG_DIV_NUMER = numer;
    *(volatile u64 *) REG_DIV_DENOM = denom;
}

Obj *func_02061c9c(Obj *self, Vec3 *vecA, Vec3 *vecB, Unk_020610e4 a3, s32 a4) {
    void *p;
    u16 n;

    self->f10.x = 0;
    self->f10.y = 0;
    self->f10.z = 0;
    self->f1C.x = 0;
    self->f1C.y = 0;
    self->f1C.z = 0;
    self->f28   = 0;
    if (data_021042c4 > MAX_INSTANCES) {
        return self;
    }
    self->f0C   = 0xffff;
    self->f08   = 0;
    self->count = 1;
    self->cur   = 0;
    n           = self->count;
    p           = func_0205e818(n * sizeof(Elem), 3, 4);
    if (p != 0) {
        p = func_020e4600(p, n, sizeof(Elem), 0, func_02061f0c, 0);
    }
    self->elems = p;
    vec_set(&self->elems[self->cur].f00, vecA);
    vec_set(&self->elems[self->cur].f14, vecB);
    self->elems[self->cur].f10 = a3.v;
    self->elems[self->cur].f0C = a4;
    self->elems[self->cur].f20 = FX_Div(FX_ONE, a3.v);
    {
        Elem *e = self->elems + self->cur;
        div_start(FX_ONE * FX_ONE, e->f10 - e->f0C);
    }
    self->f0E = 0;
    vec_clear(&self->f10);
    vec_set(&self->f1C, vecA);
    data_027e03a0.vt->add(&data_027e03a0, self);
    func_02055700(&data_027e0410, self);
    data_021042c4++;
    while (*(volatile u16 *) REG_DIVCNT & DIVCNT_BUSY) {
    }
    self->elems[self->cur].f24 = *(volatile s32 *) REG_DIV_RESULT;
    return self;
}
