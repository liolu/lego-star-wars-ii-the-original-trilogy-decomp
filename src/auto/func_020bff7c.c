/* Reconstructed from USA ARM9 at 0x020bff7c (220 bytes).
 *
 * Sets up a linear ramp in the 16-byte entry data_0210ecf4[arg1->0x4C]:
 * start = lo0 + func_02062b10(hi0 - lo0) and end = lo1 + func_02062b10(hi1 - lo1)
 * (func_02062b10 scales the 15-bit value of func_02062d7c to [0, n]), then
 * rate = (end - start) / span in fx32.  span is (b - a) >> FX32_SHIFT when mode is set,
 * otherwise a and b are first scaled by the fx32 factor at arg1 + 0x74.
 * The halfword at +0 is cleared through the H wrapper returned by mkH: the
 * inline's return temporaries are the two dead halfword stores at sp and sp+2.
 */
#include "m2c.h"

extern u32 func_02062b10(s32);
extern s32 FX_Div(s32, s32);

typedef struct {
    u8 pad0[8];
    s32 a;
    s32 b;
    s32 mode;
    s32 lo0;
    s32 hi0;
    s32 lo1;
    s32 hi1;
} Src;

typedef struct {
    u16 v;
} H;

typedef struct {
    u16 flag;
    u16 pad;
    u32 pad4;
    s32 start;
    s32 rate;
} Entry;

extern Entry data_0210ecf4[];

enum {
    FX32_SHIFT = 0xc,
    FX32_HALF  = 0x800
};

static inline H mkH(u16 x) {
    H h;
    h.v = x;
    return h;
}

void func_020bff7c(Src *src, void *arg1) {
    s32 start;
    s32 span;
    s32 end;
    s32 rate;
    Entry *e = &data_0210ecf4[M2C_FIELD(arg1, s32 *, 0x4C)];

    if (src->mode != 0) {
        span = (src->b - src->a) >> FX32_SHIFT;
    } else {
        // FAKE: volatile word view of src (f[2] = src->a, f[3] = src->b) keeps those two loads in source
        // order (a then b) after the arg1+0x74 load instead of letting mwcc schedule them earlier.
        volatile s32 *f = (volatile s32 *) src;
        // FAKE: volatile load of the fx32 factor at arg1+0x74 forces it to be the first load of this branch.
        s32 t  = ((volatile s32 *) arg1)[29];
        s32 pa = (s32) (((s64) f[2] * t + FX32_HALF) >> FX32_SHIFT);
        s32 pc = (s32) (((s64) f[3] * t + FX32_HALF) >> FX32_SHIFT);
        span   = (pc >> FX32_SHIFT) - (pa >> FX32_SHIFT);
    }
    start = func_02062b10((u16) (src->hi0 - src->lo0));
    start = start + src->lo0;
    end   = func_02062b10((u16) (src->hi1 - src->lo1));
    end += src->lo1;
    rate     = FX_Div(end - start, span << FX32_SHIFT);
    e->rate  = rate;
    e->start = start;
    e->flag  = mkH(0).v;
}
