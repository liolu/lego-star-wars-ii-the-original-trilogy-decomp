#include "m2c.h"

/* Joint animation result: flag bits, then scale, the two Maya
 * segment-scale-compensation scale vectors, a 3x3 rotation and a
 * translation (rotation and translation are contiguous, so one
 * 12-word MTX_MULT_4x3 send covers both). */
typedef struct NNSG3dJntAnmResult {
    u32 flag;
    s32 scale[3];
    s32 scaleEx0[3];
    s32 scaleEx1[3];
    s32 rot[9];
    s32 trans[3];
} NNSG3dJntAnmResult;

enum {
    JNTANM_FLAG_SCALE_ONE      = 0x01,
    JNTANM_FLAG_ROT_ZERO       = 0x02,
    JNTANM_FLAG_TRANS_ZERO     = 0x04,
    JNTANM_FLAG_SCALEEX0_ONE   = 0x08,
    JNTANM_FLAG_MAYA_SSC_APPLY = 0x20,
};

/* Geometry engine command ids (DS hardware) and their word counts. */
enum {
    GE_OP_MTX_MULT_4x3 = 0x19,
    GE_OP_MTX_MULT_3x3 = 0x1a,
    GE_OP_MTX_SCALE    = 0x1b,
    GE_OP_MTX_TRANS    = 0x1c,
    GE_LEN_VEC         = 3,
    GE_LEN_MTX33       = 9,
    GE_LEN_MTX43       = 12,
};

void NNS_G3dGeBufferOP_N(s32 op, void *param, s32 count);

/*
 * Sends one joint's SRT to the geometry buffer, Maya style:
 * 1. With segment scale compensation active (and scaleEx0 not identity),
 *    send any pending translation first, then apply scaleEx0.
 * 2. Rotation: a 4x3 multiply also carrying a still-pending translation,
 *    else a 3x3 multiply; with no rotation, send any pending translation.
 * 3. Finally apply the joint scale unless it is identity.
 */
void NNSi_G3dSendJointSRTMaya(NNSG3dJntAnmResult *result) {
    int transPending = 0;

    if (!(result->flag & JNTANM_FLAG_TRANS_ZERO)) {
        transPending = 1;
    }

    if ((result->flag & JNTANM_FLAG_MAYA_SSC_APPLY) && !(result->flag & JNTANM_FLAG_SCALEEX0_ONE)) {
        if (transPending) {
            NNS_G3dGeBufferOP_N(GE_OP_MTX_TRANS, result->trans, GE_LEN_VEC);
            transPending = 0;
        }
        NNS_G3dGeBufferOP_N(GE_OP_MTX_SCALE, result->scaleEx0, GE_LEN_VEC);
    }

    if (!(result->flag & JNTANM_FLAG_ROT_ZERO)) {
        if (transPending) {
            NNS_G3dGeBufferOP_N(GE_OP_MTX_MULT_4x3, result->rot, GE_LEN_MTX43);
        } else {
            NNS_G3dGeBufferOP_N(GE_OP_MTX_MULT_3x3, result->rot, GE_LEN_MTX33);
        }
    } else if (transPending) {
        NNS_G3dGeBufferOP_N(GE_OP_MTX_TRANS, result->trans, GE_LEN_VEC);
    }

    if (!(result->flag & JNTANM_FLAG_SCALE_ONE)) {
        NNS_G3dGeBufferOP_N(GE_OP_MTX_SCALE, result->scale, GE_LEN_VEC);
    }
}
