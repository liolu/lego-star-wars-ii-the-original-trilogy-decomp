#include "m2c.h"

typedef struct VecFx32 {
    s32 x;
    s32 y;
    s32 z;
} VecFx32;

typedef struct MtxFx33 {
    s32 m[3][3];
} MtxFx33;

/* Joint animation result: flag word, scale, two extra scale vectors, rotation, translation. */
typedef struct NNSG3dJntAnmResult {
    u32 flag;
    VecFx32 scale;
    VecFx32 scaleEx0;
    VecFx32 scaleEx1;
    MtxFx33 rot;
    VecFx32 trans;
} NNSG3dJntAnmResult;

enum {
    JNTANM_RESULTFLAG_SCALE_ONE  = 0x01,
    JNTANM_RESULTFLAG_ROT_ZERO   = 0x02,
    JNTANM_RESULTFLAG_TRANS_ZERO = 0x04,
};

enum {
    G3OP_MTX_MULT_4x3 = 0x19,
    G3OP_MTX_MULT_3x3 = 0x1a,
    G3OP_MTX_SCALE    = 0x1b,
    G3OP_MTX_TRANS    = 0x1c,
};

/* Parameter word counts of the geometry commands above. */
enum {
    G3OP_MTX_MULT_4x3_NPARAMS = 12,
    G3OP_MTX_MULT_3x3_NPARAMS = 9,
    G3OP_MTX_SCALE_NPARAMS    = 3,
    G3OP_MTX_TRANS_NPARAMS    = 3,
};

void NNS_G3dGeBufferOP_N(s32 op, void *param, s32 count);

/*
 * Basic SRT handler (entry of G3d_gSRTTransformHandlers): pushes one joint's
 * animation result onto the current matrix via the geometry command buffer.
 * Rotation and translation are sent as one 4x3 multiply, or alone as a 3x3
 * multiply / a translate when the other is flagged zero; scale is sent last
 * unless flagged as one.
 */
void NNSi_G3dSendJointSRTBasic(const NNSG3dJntAnmResult *result) {
    if (!(result->flag & JNTANM_RESULTFLAG_TRANS_ZERO)) {
        if (!(result->flag & JNTANM_RESULTFLAG_ROT_ZERO)) {
            /* rotation + translation are contiguous: 12 words */
            NNS_G3dGeBufferOP_N(G3OP_MTX_MULT_4x3, (void *) &result->rot, G3OP_MTX_MULT_4x3_NPARAMS);
        } else {
            NNS_G3dGeBufferOP_N(G3OP_MTX_TRANS, (void *) &result->trans, G3OP_MTX_TRANS_NPARAMS);
        }
    } else {
        if (!(result->flag & JNTANM_RESULTFLAG_ROT_ZERO)) {
            NNS_G3dGeBufferOP_N(G3OP_MTX_MULT_3x3, (void *) &result->rot, G3OP_MTX_MULT_3x3_NPARAMS);
        }
    }
    if (!(result->flag & JNTANM_RESULTFLAG_SCALE_ONE)) {
        NNS_G3dGeBufferOP_N(G3OP_MTX_SCALE, (void *) &result->scale, G3OP_MTX_SCALE_NPARAMS);
    }
}
