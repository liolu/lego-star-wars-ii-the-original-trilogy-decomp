#include "m2c.h"

typedef volatile u32 vu32;
typedef volatile u64 vu64;

typedef struct {
    s32 _00, _01, _02, _03;
    s32 _10, _11, _12, _13;
    s32 _20, _21, _22, _23;
    s32 _30, _31, _32, _33;
} MtxFx44;

enum {
    FX32_ONE            = 0x1000, /* 1.0 in 20.12 fixed point */
    REG_MTX_MODE        = 0x04000440, /* geometry engine: matrix mode select */
    REG_MTX_LOAD_4x4    = 0x04000458, /* geometry engine: load current matrix, 16 words */
    REG_DIV_NUMER       = 0x04000290, /* hardware divider: 64-bit numerator */
    REG_DIV_DENOM       = 0x04000298, /* hardware divider: 64-bit denominator */
    MTX_MODE_PROJECTION = 0
};

/* func_020c9344: set the divider to 64/32 mode and start FX32_ONE / denom (64-bit result, 32 fraction bits). */
extern void func_020c9344(s32 denom);
/* func_020c92e8: wait for the divider and return its 64-bit quotient. */
extern s64 func_020c92e8(void);

/* Start FX32_ONE / denom on the divider without changing its mode (already 64/32 from func_020c9344). */
static inline void FX_InvAsyncImm(s32 denom) {
    *(vu64 *) REG_DIV_NUMER = (u64) FX32_ONE << 32;
    *(vu64 *) REG_DIV_DENOM = (u32) denom;
}

/* v32 * v64c, where v64c has 32 fraction bits, rounded back to 20.12. */
static inline s32 FX_Mul32x64c(s32 v32, s64 v64c) {
    s64 tmp = v64c * v32 + 0x80000000LL;
    return (s32) (tmp >> 32);
}

/*
 * Build an orthographic projection for the box left..right, top..bottom, near_z..far_z, with every term
 * scaled by scale_w (FX32_ONE = no scaling):
 *   _00 = 2*w/(right-left)   _11 = 2*w/(top-bottom)   _22 = 2*w/(near_z-far_z)   _33 = w
 *   _30 = -(right+left)*w/(right-left)   _31 = -(top+bottom)*w/(top-bottom)   _32 = (far_z+near_z)*w/(near_z-far_z)
 * all other terms 0. If draw is set, the matrix is loaded into the geometry engine's projection matrix; if
 * mtx is non-NULL it is also stored there. The three reciprocals are pipelined on the hardware divider:
 * each one is started before the previous result is used.
 */
void G3i_OrthoW_(s32 top, s32 bottom, s32 left, s32 right, s32 near_z, s32 far_z, s32 scale_w, s32 draw, MtxFx44 *mtx) {
    s64 inv_width, inv_height, inv_depth;
    s32 tmp1, tmp2, tmp3;
    vu32 *fifo;

    func_020c9344(right - left);

    if (draw) {
        *(vu32 *) REG_MTX_MODE = MTX_MODE_PROJECTION;
        fifo                   = (vu32 *) REG_MTX_LOAD_4x4;
    }

    if (mtx) {
        mtx->_01 = 0;
        mtx->_02 = 0;
        mtx->_03 = 0;
        mtx->_10 = 0;
        mtx->_12 = 0;
        mtx->_13 = 0;
        mtx->_20 = 0;
        mtx->_21 = 0;
        mtx->_23 = 0;
        mtx->_33 = scale_w;
    }

    /* Row 0: x scale. */
    inv_width = func_020c92e8();
    FX_InvAsyncImm(top - bottom);
    if (scale_w != FX32_ONE) {
        inv_width = (inv_width * scale_w) / FX32_ONE;
    }
    tmp1 = FX_Mul32x64c(FX32_ONE * 2, inv_width);
    if (draw) {
        *fifo = tmp1;
        *fifo = 0;
        *fifo = 0;
        *fifo = 0;
        *fifo = 0;
    }
    if (mtx) {
        mtx->_00 = tmp1;
    }

    /* Row 1: y scale. */
    inv_height = func_020c92e8();
    FX_InvAsyncImm(near_z - far_z);
    if (scale_w != FX32_ONE) {
        inv_height = (inv_height * scale_w) / FX32_ONE;
    }
    tmp1 = FX_Mul32x64c(FX32_ONE * 2, inv_height);
    if (draw) {
        *fifo = tmp1;
        *fifo = 0;
        *fifo = 0;
        *fifo = 0;
        *fifo = 0;
    }
    if (mtx) {
        mtx->_11 = tmp1;
    }

    /* Row 2: z scale. */
    inv_depth = func_020c92e8();
    if (scale_w != FX32_ONE) {
        inv_depth = (inv_depth * scale_w) / FX32_ONE;
    }
    tmp1 = FX_Mul32x64c(FX32_ONE * 2, inv_depth);
    if (draw) {
        *fifo = tmp1;
        *fifo = 0;
    }
    if (mtx) {
        mtx->_22 = tmp1;
    }

    /* Row 3: translation, then w. */
    tmp1 = FX_Mul32x64c(-(right + left), inv_width);
    tmp2 = FX_Mul32x64c(-(top + bottom), inv_height);
    tmp3 = FX_Mul32x64c(far_z + near_z, inv_depth);
    if (draw) {
        *fifo = tmp1;
        *fifo = tmp2;
        *fifo = tmp3;
        *fifo = scale_w;
    }
    if (mtx) {
        mtx->_30 = tmp1;
        mtx->_31 = tmp2;
        mtx->_32 = tmp3;
    }
}
