#include "m2c.h"

typedef struct {
    u8 pad0[0x10];
    s32 p10;
    s32 p14;
    u8 pad18[0x8];
    s32 p20;
    u8 pad24[0x8];
    s32 p2c;
    s32 p30;
    s32 p34;
} Obj;

typedef struct {
    s32 v;
} UnkVal;

void func_0204d250(void *arg0, UnkVal arg1, ...);
void func_0204d27c(void *arg0, UnkVal arg1, ...);

enum {
    FX_FRAC_BITS  = 12,
    FX_ROUND_HALF = 0x800
};

/*
 * Recomputes two Q12 fixed-point values from a and b and pushes each to self
 * through its setter only when it differs from the cached field:
 *   a->p10 * b->p30 (rounded)     vs self->p2c -> func_0204d250
 *   a->p14 * b->p34 (rounded) / 2 vs self->p20 -> func_0204d27c (arithmetic >> 1)
 * The second rounding add is done in u64; this form matches the target.
 */
void func_0204d1c8(Obj *self, s32 unused, Obj *a, Obj *b) {
    s64 m = a->p10;
    s32 v;
    m *= b->p30;
    v = (s32) ((m + FX_ROUND_HALF) >> FX_FRAC_BITS);
    if (v != self->p2c) {
        UnkVal t;
        t.v = v;
        func_0204d250(self, t);
    }
    m = a->p14;
    m *= b->p34;
    // FAKE: the (u64) cast on the second rounding add swaps the smull register pair; no value change
    m = (u64) m + FX_ROUND_HALF;
    v = (s32) (m >> FX_FRAC_BITS) >> 1;
    if (v != self->p20) {
        UnkVal t;
        t.v = v;
        func_0204d27c(self, t);
    }
}
