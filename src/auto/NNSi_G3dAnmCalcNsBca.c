#include "m2c.h"

typedef s32 fx32;

typedef struct NNSG3dResAnmHeader {
    u8 category0;
    u8 revision;
    u16 category1;
} NNSG3dResAnmHeader;

typedef struct NNSG3dResJntAnm {
    NNSG3dResAnmHeader anmHeader;
    u16 numFrame;
    u16 flag;
} NNSG3dResJntAnm;

typedef struct NNSG3dAnmObj {
    fx32 frame;
    fx32 ratio;
    void *resAnm;
} NNSG3dAnmObj;

typedef struct NNSG3dJntAnmResult NNSG3dJntAnmResult;

enum {
    FX32_SHIFT = 12,
};

void getJntSRTAnmResult_(const NNSG3dResJntAnm *pJntAnm, u32 dataIdx, fx32 Frame, NNSG3dJntAnmResult *pResult);

/*
 * Joint (.nsbca) animation calc entry from the NNS_G3dFuncAnmJntNsBcaDefault
 * table: clamps the animation object's fx32 frame to [0, numFrame - 1/4096]
 * (frame count shifted to fx32) and fetches the SRT result for joint dataIdx.
 */
void NNSi_G3dAnmCalcNsBca(NNSG3dJntAnmResult *pResult, const NNSG3dAnmObj *pAnmObj, u32 dataIdx) {
    fx32 frame                 = pAnmObj->frame;
    const NNSG3dResJntAnm *anm = (const NNSG3dResJntAnm *) pAnmObj->resAnm;

    if (frame >= (anm->numFrame << FX32_SHIFT)) {
        frame = (anm->numFrame << FX32_SHIFT) - 1;
    } else if (frame < 0) {
        frame = 0;
    }

    getJntSRTAnmResult_(anm, dataIdx, frame, pResult);
}
