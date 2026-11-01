#include "m2c.h"

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} VecFx32;

typedef struct {
    u32 flag;
    VecFx32 scale;
} NNSG3dJntAnmResult;

enum {
    SRTFLAG_SCALE_ONE       = 0x4,
    RESULTFLAG_SCALE_ONE    = 0x1,
    RESULTFLAG_SCALEEX0_ONE = 0x8,
    RESULTFLAG_SCALEEX1_ONE = 0x10,
};

/*
 * Basic (non-Maya, non-SI3D) joint scale handler, reached through the
 * G3d_gScaleHandlers pointer table. If the joint's SRT flags say the scale
 * is identity, only the result's scale-one flag is set; otherwise the
 * fx32 scale vector is copied into the result. In both cases the two
 * extended-scale terms are marked as one, since the basic mode has none.
 */
void NNSi_G3dGetJointScaleBasic(NNSG3dJntAnmResult *pResult, const VecFx32 *p, const u8 *cmd, u32 srtflag) {
    if (srtflag & SRTFLAG_SCALE_ONE) {
        pResult->flag |= RESULTFLAG_SCALE_ONE;
    } else {
        pResult->scale.x = p->x;
        pResult->scale.y = p->y;
        pResult->scale.z = p->z;
    }
    pResult->flag |= RESULTFLAG_SCALEEX0_ONE | RESULTFLAG_SCALEEX1_ONE;
}
