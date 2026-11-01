#include "m2c.h"

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

/* 4x4 fixed-point matrix; rows 0..2 hold the linear part, row 3 the translation. */
typedef struct {
    s32 m[4][4];
} Mtx44;

enum {
    FX_SHIFT = 12, /* fractional bits of the fixed-point format */
};

/*
 * Transforms the point v by mtx as a row vector (out = v * mtx): each output
 * component is the 64-bit dot product of v with a matrix column over rows
 * 0..2, shifted down by FX_SHIFT, plus the translation in row 3. out is
 * cleared first and each component is written, then has the translation added.
 */
void func_01ffabe4(Vec3 *out, Mtx44 *mtx, Vec3 *v) {
    s32 x = v->x;
    s32 y = v->y;
    s32 z = v->z;

    out->x = 0;
    out->y = 0;
    out->z = 0;
    out->x = (s32) (((s64) x * mtx->m[0][0] + (s64) y * mtx->m[1][0] + (s64) z * mtx->m[2][0]) >> FX_SHIFT);
    out->x += mtx->m[3][0];
    out->y = (s32) (((s64) x * mtx->m[0][1] + (s64) y * mtx->m[1][1] + (s64) z * mtx->m[2][1]) >> FX_SHIFT);
    out->y += mtx->m[3][1];
    out->z = (s32) (((s64) x * mtx->m[0][2] + (s64) y * mtx->m[1][2] + (s64) z * mtx->m[2][2]) >> FX_SHIFT);
    out->z += mtx->m[3][2];
}
