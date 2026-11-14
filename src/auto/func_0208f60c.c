#include "m2c.h"

typedef struct Vec3 {
    s32 x, y, z;
} Vec3;

typedef struct SVec3 {
    s16 x, y, z;
} SVec3;

typedef struct Fx {
    s32 v;
} Fx;

/* G3D model set: the entry offset at +0xe leads (via +0xc) to the model data. */
typedef struct MdlSet {
    u8 pad0[0xe];
    u16 ofsEntry;
} MdlSet;

/* G3D model; the box fields are those of the model info block at +0x14
   (same layout as MdlInfo boxW/boxH/boxD/boxPosScale in func_020c09b0). */
typedef struct Mdl {
    u8 pad0[0x2c];
    SVec3 boxPos;
    SVec3 boxSize;
    Fx boxPosScale;
} Mdl;

typedef struct Ctx {
    u8 pad0[0xdc];
    MdlSet *mdlSet;
} Ctx;

typedef struct Obj {
    Ctx *ctx;
    Vec3 boxMin;
    Vec3 boxMax;
} Obj;

/* Q12 fixed point: a product is rounded by adding half a unit, then shifted down. */
enum {
    FX_SHIFT = 12,
    FX_HALF  = 0x800
};

static inline s32 fxmul(s32 a, s32 b) {
    return (s32) (((s64) a * b + FX_HALF) >> FX_SHIFT);
}

static inline s32 fx_mul(s32 a, Fx b) {
    return (s32) (((s64) a * b.v + FX_HALF) >> FX_SHIFT);
}

static inline void vec_scale_x(Vec3 *v, const Fx *s) {
    v->x = fx_mul(v->x, *s);
}

static inline void vec_scale_xy(Vec3 *v, const Fx *s) {
    vec_scale_x(v, s);
    v->y = fx_mul(v->y, *s);
}

static inline void vec_scale(Vec3 *v, const Fx *s) {
    vec_scale_xy(v, s);
    v->z = fx_mul(v->z, *s);
}

/*
 * Sets obj's box from its G3D model (model set at obj->ctx + 0xdc, model via the
 * set's entry offset): boxMin = boxPos * boxPosScale, then
 * boxMax = boxSize * boxPosScale + boxMin (Q12 multiplies, rounded).
 */
void func_0208f60c(Obj *obj) {
    MdlSet *set;
    Mdl *mdl;
    Fx scale;
    s32 x, y, z;

    set           = obj->ctx->mdlSet;
    mdl           = (Mdl *) ((u8 *) set + *(s32 *) ((u8 *) set + set->ofsEntry + 0xc));
    scale         = mdl->boxPosScale;
    z             = mdl->boxPos.z;
    y             = mdl->boxPos.y;
    x             = mdl->boxPos.x;
    obj->boxMin.x = x;
    obj->boxMin.y = y;
    obj->boxMin.z = z;
    vec_scale(&obj->boxMin, &scale);
    z = mdl->boxSize.z;
    y = mdl->boxSize.y;
    x = mdl->boxSize.x;
    // FAKE: dead store (mwcc drops it), but the extra use of x reaches register allocation and gives the
    // target colours in this block (x r4, y r6, push {r4,r5,r6,lr}); without it x lands in r3
    obj->boxMax.x = x;
    obj->boxMax.x = fxmul(x, scale.v);
    obj->boxMax.y = fxmul(y, scale.v);
    obj->boxMax.z = fxmul(z, scale.v);
    obj->boxMax.x += obj->boxMin.x;
    obj->boxMax.y += obj->boxMin.y;
    obj->boxMax.z += obj->boxMin.z;
}
