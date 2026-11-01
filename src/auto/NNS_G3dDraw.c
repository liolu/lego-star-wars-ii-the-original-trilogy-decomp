#include "m2c.h"

typedef struct RenderState {
    u8 data[0x188];
} RenderState;

typedef struct RenderObj {
    u32 flag;
    void *resMdl;
    void *anmMat;
    void *funcBBM;
    void *anmJnt;
    void *funcBBJ;
    void *anmVis;
    void *funcBBV;
    u8 unk20[0x3c - 0x20];
    u32 hintMatAnmExist[2];
    u32 hintJntAnmExist[2];
    u32 hintVisAnmExist[2];
} RenderObj;

extern RenderState *NNS_G3dRS;

extern void MIi_CpuClear32(u32 data, void *dest, u32 size);
extern void updateHintVec_(u32 *vec, void *anm);
extern void G3d_InitRenderState(RenderState *rs, RenderObj *obj);

enum {
    RENDEROBJ_FLAG_HINT_OBSOLETE = 0x10,
};

/*
 * Draws a render object. If the object's animation hint bit vectors are
 * marked stale, clears the three 8-byte vectors and rebuilds each one from
 * its attached animation (when present), then clears the stale flag.
 * Rendering runs against the global render state NNS_G3dRS; when none is
 * installed, a stack-local state is installed for the duration of the call
 * and the global is reset to NULL afterwards.
 */
void NNS_G3dDraw(RenderObj *obj) {
    if ((obj->flag & RENDEROBJ_FLAG_HINT_OBSOLETE) == RENDEROBJ_FLAG_HINT_OBSOLETE) {
        MIi_CpuClear32(0, obj->hintMatAnmExist, sizeof(obj->hintMatAnmExist));
        MIi_CpuClear32(0, obj->hintJntAnmExist, sizeof(obj->hintJntAnmExist));
        MIi_CpuClear32(0, obj->hintVisAnmExist, sizeof(obj->hintVisAnmExist));
        if (obj->anmMat) {
            updateHintVec_(obj->hintMatAnmExist, obj->anmMat);
        }
        if (obj->anmJnt) {
            updateHintVec_(obj->hintJntAnmExist, obj->anmJnt);
        }
        if (obj->anmVis) {
            updateHintVec_(obj->hintVisAnmExist, obj->anmVis);
        }
        obj->flag &= ~RENDEROBJ_FLAG_HINT_OBSOLETE;
    }

    if (NNS_G3dRS) {
        G3d_InitRenderState(NNS_G3dRS, obj);
    } else {
        RenderState rs;
        NNS_G3dRS = &rs;
        G3d_InitRenderState(NNS_G3dRS, obj);
        NNS_G3dRS = 0;
    }
}
