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
    s32 unk0;
    Vec pos;
    u8 pad10[0x20];
    s32 radius;
} Obj;

enum {
    REG_DIVCNT_ADDR      = 0x04000280,
    REG_DIV_NUMER_ADDR   = 0x04000290,
    REG_DIV_DENOM_ADDR   = 0x04000298,
    REG_DIV_RESULT_ADDR  = 0x040002A0,
    REG_SQRTCNT_ADDR     = 0x040002B0,
    REG_SQRT_RESULT_ADDR = 0x040002B4,
    REG_SQRT_PARAM_ADDR  = 0x040002B8,
    DIV_MODE_64_64       = 2,
    SQRT_MODE_64         = 1,
    HW_BUSY              = 0x8000,
    FX32_ONE             = 0x1000,
    FX32_HALF            = 0x800,
};

extern Vec data_020ff9b4;
extern s32 FX_Div(s32 num, s32 den);
void func_0200a34c(s32 *out, Fx b, Fx c);

static inline s32 fx_mul(s32 a, s32 b) {
    return (s32) (((s64) a * b + FX32_HALF) >> 12);
}

/*
 * Intersects a ray with a circle on the XZ plane and returns the unit
 * outward normal at the hit point.
 *
 * The direction is flattened (y = 0) and normalised with the inlined
 * hardware sqrt/divide sequence (falling back to data_020ff9b4 when it is
 * zero). With d = origin - centre the hit distance s solves
 * s^2 + 2(dir.d) s + (|d|^2 - r^2) = 0; func_0200a34c returns the nearest
 * positive root. The hit point minus the centre, divided by the radius
 * (fx32), is written to out with y = 0.
 */
void func_0200913c(Vec *out, Vec *origin, Vec dir, Obj *circle) {
    s64 lenSq;
    s32 sqrtLen;
    s64 recip;
    s32 dx;
    s32 dz;
    Fx b;
    Fx c;
    s32 dist;
    s32 hitX;
    s32 hitZ;
    s32 invRadius;

    dir.y = 0;
    if ((M2C_FIELD(&dir, s32 *, 0) == 0) && (M2C_FIELD(&dir, s32 *, 4) == 0) && (M2C_FIELD(&dir, s32 *, 8) == 0)) {
        if (&dir != &data_020ff9b4) {
            dir.x = data_020ff9b4.x;
            dir.y = data_020ff9b4.y;
            dir.z = data_020ff9b4.z;
        }
    } else {
        lenSq = (s64) dir.x * dir.x;
        lenSq += (s64) dir.y * dir.y;
        lenSq += (s64) dir.z * dir.z;

        (*(vu16 *) REG_DIVCNT_ADDR)    = DIV_MODE_64_64;
        (*(vu64 *) REG_DIV_NUMER_ADDR) = 1LL << 56;
        (*(vu64 *) REG_DIV_DENOM_ADDR) = lenSq;

        (*(vu16 *) REG_SQRTCNT_ADDR)   = SQRT_MODE_64;
        (*(u64 *) REG_SQRT_PARAM_ADDR) = lenSq << 2;

        while ((*(vu16 *) REG_SQRTCNT_ADDR) & HW_BUSY) {
        }
        sqrtLen = (s32) (*(vu32 *) REG_SQRT_RESULT_ADDR);

        while ((*(vu16 *) REG_DIVCNT_ADDR) & HW_BUSY) {
        }
        recip = (s64) (*(u64 *) REG_DIV_RESULT_ADDR);

        recip *= sqrtLen;

        dir.x = (s32) ((recip * dir.x + (1LL << 44)) >> 45);
        dir.y = (s32) ((recip * dir.y + (1LL << 44)) >> 45);
        dir.z = (s32) ((recip * dir.z + (1LL << 44)) >> 45);
    }

    dz  = origin->z - circle->pos.z;
    dx  = origin->x - circle->pos.x;
    b.v = (fx_mul(dir.x, dx) + fx_mul(dir.z, dz)) * 2;
    c.v = fx_mul(dx, dx) + fx_mul(dz, dz) - fx_mul(circle->radius, circle->radius);
    func_0200a34c(&dist, b, c);

    hitX      = origin->x + fx_mul(dist, dir.x);
    hitZ      = origin->z + fx_mul(dist, dir.z);
    invRadius = FX_Div(FX32_ONE, circle->radius);

    out->x = 0;
    out->y = 0;
    out->z = 0;
    hitX -= circle->pos.x;
    out->x = fx_mul(hitX, invRadius);
    out->y = 0;
    hitZ -= circle->pos.z;
    out->z = fx_mul(hitZ, invRadius);
}
