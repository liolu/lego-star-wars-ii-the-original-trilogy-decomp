#include "m2c.h"

typedef struct NNSG3dRS NNSG3dRS;
typedef void (*NNSG3dSbcCallBackFunc)(NNSG3dRS *rs);

typedef struct NNSG3dVisAnmResult {
    s32 isVisible;
} NNSG3dVisAnmResult;

typedef s32 (*NNSG3dFuncAnmBlendVis)(NNSG3dVisAnmResult *pResult, void *pAnmObj, u32 dataIdx);

typedef struct NNSG3dRenderObj {
    u32 flag;                         /* 0x00 */
    void *resMdl;                     /* 0x04 */
    void *anmMat;                     /* 0x08 */
    void *funcBlendMat;               /* 0x0C */
    void *anmJnt;                     /* 0x10 */
    void *funcBlendJnt;               /* 0x14 */
    void *anmVis;                     /* 0x18 */
    NNSG3dFuncAnmBlendVis funcBlendVis; /* 0x1C */
    u8 unk20[0x4C - 0x20];
    u32 anmVisMask[1];                /* 0x4C */
} NNSG3dRenderObj;

struct NNSG3dRS {
    const u8 *c;                      /* 0x00 */
    NNSG3dRenderObj *pRenderObj;      /* 0x04 */
    u32 flag;                         /* 0x08 */
    NNSG3dSbcCallBackFunc cbVecFunc[32]; /* 0x0C */
    u8 cbVecTiming[32];               /* 0x8C */
    u8 currentNode;                   /* 0xAC */
    u8 unkAD[0xB8 - 0xAD];
    NNSG3dVisAnmResult *pVisAnmResult; /* 0xB8 */
    u8 unkBC[0x184 - 0xBC];
    NNSG3dVisAnmResult tmpVisAnmResult; /* 0x184 */
};

enum {
    NNS_G3D_RSFLAG_NODE_VISIBLE       = 0x00000001,
    NNS_G3D_RSFLAG_CURRENT_NODE_VALID = 0x00000004,
    NNS_G3D_RSFLAG_SKIP               = 0x00000040,
    NNS_G3D_RSFLAG_OPT_SKIP_SBCDRAW   = 0x00000200,
    NNS_G3D_SBC_NODE                  = 2,
    NNS_G3D_SBC_CALLBACK_TIMING_NONE  = 0,
    NNS_G3D_SBC_CALLBACK_TIMING_A     = 1,
    NNS_G3D_SBC_CALLBACK_TIMING_B     = 2,
    NNS_G3D_SBC_CALLBACK_TIMING_C     = 3,
};

static inline u32 GetCallBackTiming(NNSG3dRS *rs, u8 cmd) {
    return rs->cbVecFunc[cmd] ? rs->cbVecTiming[cmd] : NNS_G3D_SBC_CALLBACK_TIMING_NONE;
}

static inline s32 CallBackCheck_A(NNSG3dRS *rs, u8 cmd, u32 *pTiming) {
    *pTiming = GetCallBackTiming(rs, cmd);
    if (*pTiming == NNS_G3D_SBC_CALLBACK_TIMING_A) {
        rs->flag &= ~NNS_G3D_RSFLAG_SKIP;
        (*rs->cbVecFunc[cmd])(rs);
        *pTiming = GetCallBackTiming(rs, cmd);
        return rs->flag & NNS_G3D_RSFLAG_SKIP;
    }
    return 0;
}

static inline s32 CallBackCheck_B(NNSG3dRS *rs, u8 cmd, u32 *pTiming) {
    if (*pTiming == NNS_G3D_SBC_CALLBACK_TIMING_B) {
        rs->flag &= ~NNS_G3D_RSFLAG_SKIP;
        (*rs->cbVecFunc[cmd])(rs);
        *pTiming = GetCallBackTiming(rs, cmd);
        return rs->flag & NNS_G3D_RSFLAG_SKIP;
    }
    return 0;
}

static inline void CallBackCheck_C(NNSG3dRS *rs, u8 cmd, u32 timing) {
    if (timing == NNS_G3D_SBC_CALLBACK_TIMING_C) {
        rs->flag &= ~NNS_G3D_RSFLAG_SKIP;
        (*rs->cbVecFunc[cmd])(rs);
    }
}

/*
 * SBC (structure byte code) NODE command handler, 3 bytes: [op, nodeIdx, visFlag].
 * Unless drawing is skipped (flag 0x200): records the current node, points the
 * visibility result at the render state's scratch result, then
 *  - timing-A callback (may set SKIP to suppress the default work),
 *  - default: blend visibility from the bound animation when its mask bit for
 *    this node is set, otherwise take bit 0 of the command's visFlag byte,
 *  - timing-B callback, then copy the visibility into NODE_VISIBLE in rs->flag,
 *  - timing-C callback.
 * Always advances the command pointer past the 3-byte command; opt is unused.
 */
void NNSi_G3dFuncSbc_NODE(NNSG3dRS *rs, u32 opt) {
    if (!(rs->flag & NNS_G3D_RSFLAG_OPT_SKIP_SBCDRAW)) {
        u32 cbTiming;
        u32 curNode = rs->c[1];

        rs->currentNode = curNode;
        rs->flag |= NNS_G3D_RSFLAG_CURRENT_NODE_VALID;
        rs->pVisAnmResult = &rs->tmpVisAnmResult;

        if (!CallBackCheck_A(rs, NNS_G3D_SBC_NODE, &cbTiming)) {
            NNSG3dRenderObj *ro = rs->pRenderObj;
            if (!(ro->anmVis && (ro->anmVisMask[curNode >> 5] & (1 << (curNode & 31))) &&
                  (*ro->funcBlendVis)(rs->pVisAnmResult, ro->anmVis, curNode))) {
                rs->pVisAnmResult->isVisible = rs->c[2] & 1;
            }
        }

        if (!CallBackCheck_B(rs, NNS_G3D_SBC_NODE, &cbTiming)) {
            if (rs->pVisAnmResult->isVisible) {
                rs->flag |= NNS_G3D_RSFLAG_NODE_VISIBLE;
            } else {
                rs->flag &= ~NNS_G3D_RSFLAG_NODE_VISIBLE;
            }
        }

        CallBackCheck_C(rs, NNS_G3D_SBC_NODE, cbTiming);
    }
    rs->c += 3;
}
