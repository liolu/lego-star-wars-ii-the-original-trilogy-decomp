#include "m2c.h"

typedef struct {
    s16 x, y, z;
} Pt16;

extern Pt16 *func_02067b48(void *, s32);
extern s32 func_02067b54(void *);
extern s32 FX_Div(s32, s32);

typedef struct {
    s32 v;
} Fx;

/* hardware square-root unit registers */
enum {
    SQRTCNT     = 0x040002b0,
    SQRT_RESULT = 0x040002b4,
    SQRT_PARAM  = 0x040002b8
};
enum {
    SQRTCNT_BUSY = 0x8000
};
enum {
    FX32_ONE = 0x1000
};

// FAKE: s64 return for an s32 value (stored back into s32); the extra conversion makes mwcc fold the prev shift as sub ...,
// lsl #12
static inline s64 fx64(s16 v) {
    return v << 12;
}

static inline Fx fx(s32 v) {
    Fx r;
    r.v = v;
    return r;
}

static inline s32 fx_mul_fx(s32 a, Fx b) {
    return (s32) (((s64) a * b.v + 0x800) >> 12);
}

/* Walks the path points pairwise (i-1, i), summing segment lengths (hardware sqrt of the
 * squared fx32 delta) until the sum reaches FxMul(*p1, *p2); then stores the segment's two
 * point indices and the remaining distance divided by the segment length. */
void func_020a5af8(void *self, u8 *out_start, u8 *out_end, s32 *out_frac, s32 *p1, s32 *p2) {
    Pt16 *cur;
    u32 i;
    Pt16 *prev;
    s32 total;
    u32 j;
    s32 acc;
    s64 sum;
    s32 dx, dy, dz;
    s32 len;

    total = (s32) (((s64) *p1 * *p2 + 0x800) >> 12);
    acc   = 0;
    // FAKE: u8 counter kept in a u32 (i = (i + 1) & 0xff); a u8 i with i++ changes the loop code (+0x50, +0x15c)
    for (i = 1; i < func_02067b54(M2C_FIELD(self, void **, 0x21c)); i = (i + 1) & 0xff) {
        j    = i - 1;
        prev = func_02067b48(M2C_FIELD(self, void **, 0x21c), j);
        cur  = func_02067b48(M2C_FIELD(self, void **, 0x21c), i);
        dz   = fx64(cur->z);
        dy   = fx64(cur->y);
        dx   = fx64(cur->x);
        dz -= prev->z << 12;
        dy -= prev->y << 12;
        dx -= prev->x << 12;
        sum = (s64) dx * dx;
        sum += (s64) dy * dy;
        sum += (s64) dz * dz;
        *(volatile u16 *) SQRTCNT = 1;
        *(u64 *) SQRT_PARAM       = sum << 2;
        while (*(volatile u16 *) SQRTCNT & SQRTCNT_BUSY) {
        }
        len = ((s32) * (volatile u32 *) SQRT_RESULT + 1) >> 1;
        if (len < 0) {
            len = fx_mul_fx(len, fx(-FX32_ONE));
        }
        acc += len;
        if (acc >= total) {
            *out_start = j;
            *out_end   = i;
            *out_frac  = FX_Div(total - (acc - len), len);
            return;
        }
    }
}
