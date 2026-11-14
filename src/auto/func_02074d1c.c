#include "m2c.h"

typedef volatile u16 vu16;

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct {
    u8 pad[0x14];
    Vec3 pos;
    s32 radius;
} Ent;

typedef struct {
    Ent **ents;
    u32 count;
} EntList;

typedef struct {
    EntList *list;
    Vec3 pos;
    u32 bits[4];
    Ent *best;
} Finder;

extern void func_020e0ad4(const char *);
extern void abort(void);
extern void func_02075614(u32 *, u32, s32);
extern int func_0207567c(const unsigned int *, unsigned int);
extern char data_020fecac[];
extern char data_020fec88[];

typedef struct {
    u32 *words;
    u32 idx;
} BitRef;

enum {
    FX_SHIFT        = 12,         /* fx32 is 20.12 fixed point */
    FX_FRAC_MASK    = 0xfff,
    FX_WHOLE_MASK   = -0x1000,
    BITSET_BITS     = 0x80,       /* bits[4] holds 128 flags; test/set abort past it */
    REG_SQRTCNT     = 0x040002b0, /* DS hardware square-root unit */
    REG_SQRT_RESULT = 0x040002b4,
    REG_SQRT_PARAM  = 0x040002b8,
    SQRTCNT_BUSY    = 0x8000,
};

/* Clears an fx out slot; every caller overwrites it at once. */
static inline s32 *fx_zero(s32 *p) {
    if (p) { // FAKE: always-true null test (p is a stack temp); splits the block so the zero is made after the add
        *p = 0; // FAKE: dead store; its zero is what mwcc builds -0x1000 from (mov #0; sub #0x1000)
    }
    return p;
}

static inline const s32 *snap(s32 *out, const s32 *p) {
    if (*p < 0) {
        s32 t         = *p + FX_FRAC_MASK;
        *fx_zero(out) = t & FX_WHOLE_MASK;
        return out;
    }
    *fx_zero(out) = *p & FX_WHOLE_MASK;
    return out;
}

static inline const s32 *snapz(s32 *out, const s32 *p) {
    if (*p < 0) {
        s32 t         = *p + FX_FRAC_MASK;
        *fx_zero(out) = t & FX_WHOLE_MASK;
        return out;
    }
    *out = *p & FX_WHOLE_MASK; // FAKE: snapz = snap without fx_zero on this path; z's positive path needs no dead zero
    return out;
}

static inline Vec3 *vec_sub(Vec3 *o, const Vec3 *a, const Vec3 *b) {
    s32 z = a->z - b->z;
    s32 y = a->y - b->y;
    s32 x = a->x - b->x;
    o->x  = x;
    o->y  = y;
    o->z  = z;
    return o;
}

static inline s32 whole_sq(const s32 *p) {
    s32 w = *p >> FX_SHIFT;
    return w * w;
}

static inline s32 len_sq(const Vec3 *v) {
    s32 ty, tx, tz;
    return whole_sq(snap(&tx, &v->x)) + whole_sq(snap(&ty, &v->y)) + whole_sq(snapz(&tz, &v->z));
}

static inline BitRef *bit_ref(BitRef *r, u32 *w, u32 idx) {
    r->words = w;
    r->idx   = idx;
    return r;
}

static inline int bitset_test(const u32 *w, u32 pos) {
    if (pos >= BITSET_BITS) {
        func_020e0ad4(data_020fecac);
        abort();
    }
    return func_0207567c(w, pos);
}

static inline void bitset_set(u32 *w, u32 pos, s32 val) {
    if (pos >= BITSET_BITS) {
        func_020e0ad4(data_020fec88);
        abort();
    }
    func_02075614(w, pos, val);
}

static inline BitRef *bitref_assign2(BitRef *r, s32 val) {
    bitset_set(r->words, r->idx, val);
    return r;
}

/* Picks the entry of self->list nearest to self->pos that is not yet flagged in self->bits.
 * Distance = hardware sqrt of the sum of squared whole-unit offsets (each truncated toward
 * zero), minus the entry's whole-unit radius. The winner is flagged and stored in self->best
 * (NULL when none is left); func_02027124 calls this to step its candidate iterator. */
Finder *func_02074d1c(Finder *self) {
    s32 i;
    Ent *best;
    s32 best_d;
    u32 best_idx;
    best   = 0;
    best_d = 0x7fffffff;

    for (i = 0; i < self->list->count; i++) {
        Ent *e;
        Vec3 d;
        s32 sum;
        if (bitset_test(self->bits, i)) {
            continue;
        }
        e = self->list->ents[i];
        if (e == 0) {
            continue;
        }
        {
            const Vec3 *pv = vec_sub(&d, &e->pos, &self->pos);
            sum            = len_sq(pv);
        }
        *(vu16 *) REG_SQRTCNT            = 0;
        *(volatile s32 *) REG_SQRT_PARAM = sum;
        while (*(vu16 *) REG_SQRTCNT & SQRTCNT_BUSY) {
        }
        {
            s32 m = *(vu16 *) REG_SQRT_RESULT - (e->radius >> FX_SHIFT);
            if (m < best_d) {
                best_d   = m;
                best     = e;
                best_idx = i;
            }
        }
    }
    if (best != 0) {
        BitRef r;
        bitref_assign2(bit_ref(&r, self->bits, best_idx), 1);
    }
    self->best = best;
    return self;
}
