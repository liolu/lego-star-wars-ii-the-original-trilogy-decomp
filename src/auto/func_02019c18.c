#include "m2c.h"

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct {
    s32 m[12];
} Mtx43;

typedef struct {
    u8 pad0[0xfc];
    u32 flag;               /* 0xFC */
    u8 pad100[0x240 - 0x100];
    Vec3 camPos;            /* 0x240 */
    Vec3 camUp;             /* 0x24C */
    Vec3 camTarget;         /* 0x258 */
} Glb;

typedef struct {
    u8 pad0[0x8c];
    s32 rot[9];             /* 0x8C */
    Vec3 trans;             /* 0xB0 */
    Vec3 scale;             /* 0xBC */
    u16 ang[4];             /* 0xC8 */
    u8 rotDirty;            /* 0xD0 */
    u8 rotChanged;          /* 0xD1 */
    u8 padD2[0xf5 - 0xd2];
    u8 screenSpace;            /* 0xF5 */
    u8 billboard;           /* 0xF6 */
} Obj;

extern Glb NNS_G3dGlb;
extern u8 data_027e0c1c[]; /* global projection matrix (4x4) */
extern u8 data_027e0c60[]; /* global camera matrix (4x3) */
extern u8 data_027e0cd0[]; /* global base rotation (3x3) */
extern const Vec3 data_020e5d20;
extern const Vec3 data_020e5d2c;
extern const Mtx43 data_020e5d38;

extern void NNS_G3dGeBufferOP_N(s32 op, void *param, s32 count);
extern u8 *func_020aaea8(void);
extern void *func_020ace3c(void *cam);
extern void *func_020acf58(void *cam);
extern void func_01ffac98(void *m, Vec3 *in, Vec3 *out);
extern void func_02019f74(Obj *obj);
extern void func_020c4304(Vec3 *scale);
extern void func_020c42dc(Vec3 *trans);
extern void func_01ffa6f8(void *rot, void *ang);
extern void func_020c4234(void);
extern void MI_Copy36B(const void *src, void *dst);
extern void MI_Copy64B(const void *src, void *dst);
extern void MTX_Copy44To43_(const void *src, void *dst);
extern void G3i_OrthoW_(s32 t, s32 b, s32 l, s32 r, s32 n, s32 f, s32 scaleW, s32 draw, void *mtx);
extern void G3i_LookAt_(const Vec3 *camPos, const Vec3 *camUp, const Vec3 *target, s32 draw, void *mtx);

/* Geometry-engine command ids (DS hardware) and the parameters used here. */
enum {
    GE_OP_MTX_MODE     = 0x10,
    GE_OP_MTX_PUSH     = 0x11,
    GE_OP_MTX_POP      = 0x12,
    GE_OP_MTX_IDENTITY = 0x15,
    GE_OP_MTX_LOAD_4x3 = 0x17,
};

enum {
    MTX_MODE_PROJECTION      = 0,
    MTX_MODE_POSITION_VECTOR = 2,
    FX32_ONE                 = 0x1000,
    SCREEN_W_FX              = 0x100000, /* 256 pixels, fx32 */
    SCREEN_H_FX              = 0xc0000,  /* 192 pixels, fx32 */
    /* NNS_G3dGlb.flag bits cleared when the cached matrices go stale. */
    GLB_FLAGS_PROJ_CHANGED   = 0x50,
    GLB_FLAGS_CAMERA_CHANGED = 0xe8,
    GLB_FLAGS_BASE_CHANGED   = 0xa4,
};

static inline void GeMtxMode(s32 mode) {
    s32 x = mode;
    NNS_G3dGeBufferOP_N(GE_OP_MTX_MODE, &x, 1);
}

static inline void VecCopy(const Vec3 *s, Vec3 *d) {
    d->x = s->x;
    d->y = s->y;
    d->z = s->z;
}

static inline void GlbLookAt(const Vec3 *camPos, const Vec3 *camUp, const Vec3 *target) {
    VecCopy(camPos, &NNS_G3dGlb.camPos);
    VecCopy(camUp, &NNS_G3dGlb.camUp);
    VecCopy(target, &NNS_G3dGlb.camTarget);
    G3i_LookAt_(camPos, camUp, target, 0, data_027e0c60);
    NNS_G3dGlb.flag &= ~GLB_FLAGS_CAMERA_CHANGED;
}

static inline void GePopMtx(s32 n) {
    s32 x = n;
    NNS_G3dGeBufferOP_N(GE_OP_MTX_POP, &x, 1);
}

/* Loads the object's base scale, translation and rotation (rebuilding the rotation
 * matrix from its angles when marked dirty) into the global state, flushes it and
 * draws the textured quad. */
static inline void DrawObj(Obj *obj) {
    func_020c4304(&obj->scale);
    func_020c42dc(&obj->trans);
    if (obj->rotDirty) {
        obj->rotChanged = 1;
        obj->rotDirty   = 0;
        func_01ffa6f8(obj->rot, obj->ang);
    }
    MI_Copy36B(obj->rot, data_027e0cd0);
    NNS_G3dGlb.flag &= ~GLB_FLAGS_BASE_CHANGED;
    func_020c4234();
    func_02019f74(obj);
}

/* Draws an object's textured quad in one of three ways:
 * - billboard: a 4x3 matrix from the template data_020e5d38 with the object's position
 *   transformed into camera space as translation and scale.x / scale.y on the diagonal,
 *   loaded directly as the position matrix;
 * - screenSpace: pushes the projection, sets a 256x192 orthographic projection and a
 *   camera at the origin, draws with the object's base transform, then pops and restores
 *   the projection and camera matrices from the current camera;
 * - otherwise: draws with the object's base transform under the current camera. */
void func_02019c18(Obj *obj) {
    if (obj->billboard) {
        Mtx43 m = data_020e5d38;
        func_01ffac98(func_020ace3c(func_020aaea8() + 0x4c), &obj->trans, (Vec3 *) &m.m[9]);
        m.m[0] = obj->scale.x;
        m.m[4] = obj->scale.y;
        GeMtxMode(MTX_MODE_POSITION_VECTOR);
        NNS_G3dGeBufferOP_N(GE_OP_MTX_LOAD_4x3, &m, 12);
        func_02019f74(obj);
        return;
    }
    if (obj->screenSpace) {
        GeMtxMode(MTX_MODE_PROJECTION);
        NNS_G3dGeBufferOP_N(GE_OP_MTX_PUSH, NULL, 0);
        G3i_OrthoW_(0, SCREEN_H_FX, 0, SCREEN_W_FX, FX32_ONE, -FX32_ONE, FX32_ONE, 0, data_027e0c1c);
        NNS_G3dGlb.flag &= ~GLB_FLAGS_PROJ_CHANGED;
        {
            Vec3 pos    = {0, 0, 0};
            Vec3 up     = data_020e5d2c;
            Vec3 target = data_020e5d20;
            GlbLookAt(&pos, &up, &target);
        }
        GeMtxMode(MTX_MODE_POSITION_VECTOR);
        NNS_G3dGeBufferOP_N(GE_OP_MTX_IDENTITY, NULL, 0);
        DrawObj(obj);
        GeMtxMode(MTX_MODE_PROJECTION);
        GePopMtx(1);
        GeMtxMode(MTX_MODE_POSITION_VECTOR);
        MI_Copy64B(func_020acf58(func_020aaea8() + 0x4c), data_027e0c1c);
        NNS_G3dGlb.flag &= ~GLB_FLAGS_PROJ_CHANGED;
        MTX_Copy44To43_(func_020ace3c(func_020aaea8() + 0x4c), data_027e0c60);
        NNS_G3dGlb.flag &= ~GLB_FLAGS_CAMERA_CHANGED;
    } else {
        GeMtxMode(MTX_MODE_POSITION_VECTOR);
        DrawObj(obj);
    }
}
