#include "m2c.h"

typedef struct Vec3 {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct JntAnmResult {
    u32 flag;
    Vec3 scale;
    Vec3 scaleEx0;
} JntAnmResult;

typedef struct RenderState {
    u8 pad[0xc4];
    u32 isScaleCacheOne[8];
} RenderState;

typedef struct ScaleCacheInv {
    Vec3 inv;
    Vec3 s;
} ScaleCacheInv;

typedef struct ScaleCacheWord {
    s32 v;
    s32 pad[5];
} ScaleCacheWord;

extern RenderState *NNS_G3dRS;
extern ScaleCacheInv data_02113468[];
extern ScaleCacheWord data_0211346c[];
extern ScaleCacheWord data_02113470[];

static inline void BitVecSet(u32 *vec, u32 idx) {
    vec[idx >> 5] |= 1 << (idx & 31);
}

static inline void BitVecReset(u32 *vec, u32 idx) {
    vec[idx >> 5] &= ~(1 << (idx & 31));
}

static inline u32 BitVecCheck(const u32 *vec, u32 idx) {
    return vec[idx >> 5] & (1 << (idx & 31));
}

enum {
    SRTFLAG_SCALE_ONE = 0x4, /* srtflag: joint scale is (1, 1, 1) */

    NODEDESC_MAYASSC_APPLY  = 0x1,  /* cmd[3]: apply the parent's inverse scale */
    NODEDESC_MAYASSC_PARENT = 0x2, /* cmd[3]: node is a parent; cache its scale */

    RESULTFLAG_SCALE_ONE    = 0x1,
    RESULTFLAG_SCALEEX0_ONE = 0x8,
    RESULTFLAG_SCALEEX1_ONE = 0x10,
    RESULTFLAG_MAYA_SSC     = 0x20,
};

/*
 * Maya segment-scale-compensate scale handler (entry in G3d_gScaleHandlers).
 * p: joint scale x, y, z followed by the inverse scale x, y, z (fx32).
 * cmd: node-description command; cmd[1] = this node ID, cmd[2] = parent node
 * ID, cmd[3] = Maya SSC flags.
 * 1. Store the scale (or mark it as one) in the result.
 * 2. If this node is a parent, record in the render state whether its scale
 *    is one, and otherwise cache its inverse scale per node ID.
 * 3. If SSC applies, copy the parent's cached inverse scale into scaleEx0
 *    (or mark it as one); scaleEx1 is always one.
 */
void NNSi_G3dGetJointScaleMaya(JntAnmResult *pResult, const s32 *p, const u8 *cmd, u32 srtflag) {
    u32 nodeFlag = cmd[3];

    if (srtflag & SRTFLAG_SCALE_ONE) {
        pResult->flag |= RESULTFLAG_SCALE_ONE;
        if (nodeFlag & NODEDESC_MAYASSC_PARENT) {
            BitVecSet(NNS_G3dRS->isScaleCacheOne, cmd[1]);
        }
    } else {
        pResult->scale.x = p[0];
        pResult->scale.y = p[1];
        pResult->scale.z = p[2];
        if (nodeFlag & NODEDESC_MAYASSC_PARENT) {
            u32 nodeID = cmd[1];
            BitVecReset(NNS_G3dRS->isScaleCacheOne, nodeID);
            data_02113468[nodeID].inv.x = p[3];
            data_0211346c[nodeID].v     = p[4];
            data_02113470[nodeID].v     = p[5];
        }
    }

    if (nodeFlag & NODEDESC_MAYASSC_APPLY) {
        u32 parentID = cmd[2];
        pResult->flag |= RESULTFLAG_MAYA_SSC;
        if (BitVecCheck(NNS_G3dRS->isScaleCacheOne, parentID)) {
            pResult->flag |= RESULTFLAG_SCALEEX0_ONE;
        } else {
            pResult->scaleEx0 = data_02113468[parentID].inv;
        }
    }
    pResult->flag |= RESULTFLAG_SCALEEX1_ONE;
}
