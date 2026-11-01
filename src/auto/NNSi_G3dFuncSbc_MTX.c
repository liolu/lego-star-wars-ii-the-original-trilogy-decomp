#include "m2c.h"

typedef struct NNSG3dRS NNSG3dRS;
typedef void (*NNSG3dSbcCallBackFunc)(NNSG3dRS *rs);

struct NNSG3dRS {
    /* 0x00 */ const u8 *c;
    /* 0x04 */ u32 unk4;
    /* 0x08 */ u32 flag;
    /* 0x0C */ NNSG3dSbcCallBackFunc cbVecFunc[32];
    /* 0x8C */ u8 cbVecTiming[32];
};

enum {
    RSFLAG_NODE_VISIBLE     = 0x001,
    RSFLAG_SKIP             = 0x040,
    RSFLAG_OPT_NOGECMD      = 0x100,
    RSFLAG_OPT_SKIP_SBCDRAW = 0x200,
    SBC_MTX                 = 3,
    CALLBACK_TIMING_NONE    = 0,
    CALLBACK_TIMING_A       = 1,
    CALLBACK_TIMING_C       = 3,
    G3OP_RESTORE_MTX        = 0x14,
};

void NNS_G3dGeBufferOP_N(s32 op, void *param, s32 count);

static inline u32 CheckFuncCallBack(NNSG3dRS *rs, u32 cmd) {
    return rs->cbVecFunc[cmd] ? rs->cbVecTiming[cmd] : CALLBACK_TIMING_NONE;
}

static inline u32 CallBackCheck_A(NNSG3dRS *rs, u32 cmd, u32 *pTiming) {
    u32 timing = CheckFuncCallBack(rs, cmd);
    *pTiming   = timing;
    if (timing == CALLBACK_TIMING_A) {
        rs->flag &= ~RSFLAG_SKIP;
        (*rs->cbVecFunc[cmd])(rs);
        *pTiming = CheckFuncCallBack(rs, cmd);
        return rs->flag & RSFLAG_SKIP;
    }
    return 0;
}

static inline void CallBackCheck_C(NNSG3dRS *rs, u32 cmd, u32 timing) {
    if (timing == CALLBACK_TIMING_C) {
        rs->flag &= ~RSFLAG_SKIP;
        (*rs->cbVecFunc[cmd])(rs);
    }
}

/*
 * SBC (structure byte code) MTX command handler, dispatched from the SBC
 * command table (data_02101498). The command is 2 bytes: opcode, then a
 * matrix-stack index. Unless drawing is skipped or the node is invisible:
 * run a timing-A callback (which may set the skip flag), then, if neither
 * skipped nor GE commands disabled, queue a geometry MTX_RESTORE (0x14) of
 * that stack index; finally run a timing-C callback. Always advances the
 * byte-code pointer past the command. `opt` is unused.
 */
void NNSi_G3dFuncSbc_MTX(NNSG3dRS *rs, u32 opt) {
    if (!(rs->flag & RSFLAG_OPT_SKIP_SBCDRAW) && (rs->flag & RSFLAG_NODE_VISIBLE)) {
        u32 timing;
        if (!CallBackCheck_A(rs, SBC_MTX, &timing)) {
            u32 arg = rs->c[1];
            if (!(rs->flag & RSFLAG_OPT_NOGECMD)) {
                NNS_G3dGeBufferOP_N(G3OP_RESTORE_MTX, &arg, 1);
            }
        }
        CallBackCheck_C(rs, SBC_MTX, timing);
    }
    rs->c += 2;
}
