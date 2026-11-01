#include "m2c.h"

typedef struct {
    /* 0x00 */ u32 flag;
    /* 0x04 */ s32 scale[9];
    /* 0x28 */ s32 rot[9];
    /* 0x4C */ s32 trans[3];
} JntResult; /* 0x58 */

typedef struct Rs Rs;

typedef struct {
    /* 0x00 */ u8 pad00[0x10];
    /* 0x10 */ void *anmJnt;
    /* 0x14 */ s32 (*funcBlendJnt)(JntResult *, void *, u32);
    /* 0x18 */ u8 pad18[0x34 - 0x18];
    /* 0x34 */ JntResult *recJntAnm;
} RenderObj;

typedef struct {
    u16 flag;
    s16 m0;
} Srt;

struct Rs {
    /* 0x00 */ u8 *c;
    /* 0x04 */ RenderObj *pRenderObj;
    /* 0x08 */ u32 flag;
    /* 0x0C */ u8 pad0C[0x24 - 0x0C];
    /* 0x24 */ void (*cbFunc)(Rs *);
    /* 0x28 */ u8 pad28[0x92 - 0x28];
    /* 0x92 */ u8 cbTiming;
    /* 0x93 */ u8 pad93[0xAE - 0x93];
    /* 0xAE */ u8 currentNodeDesc;
    /* 0xAF */ u8 padAF[0xB4 - 0xAF];
    /* 0xB4 */ JntResult *pJntAnmResult;
    /* 0xB8 */ u8 padB8[0xD4 - 0xB8];
    /* 0xD4 */ u8 *pResNodeInfo;
    /* 0xD8 */ u8 padD8[0xE8 - 0xD8];
    /* 0xE8 */ void (*funcJntScale)(JntResult *, u8 *, u8 *, u32);
    /* 0xEC */ void (*funcJntMtx)(JntResult *);
    /* 0xF0 */ u8 padF0[0x12C - 0xF0];
    /* 0x12C */ JntResult tmpJntAnmResult;
};

extern void NNS_G3dGeBufferOP_N(s32 op, void *param, s32 count);
extern void func_020cf518(void *p);
extern const u8 pivotUtil_[][4];
extern const u8 data_020f88a1[][4];
extern const u8 data_020f88a2[][4];
extern const u8 data_020f88a3[][4];
extern u8 data_027e02bc[];

enum {
    RSFLAG_NODEDESC_SEEN = 0x10,  /* set whenever a node-description command runs */
    RSFLAG_CB_SKIP       = 0x40,  /* set by the callback to skip the following step */
    RSFLAG_RECORD        = 0x80,  /* recompute into the recorded result array */
    RSFLAG_NO_GECMD      = 0x100, /* suppress geometry-engine commands */
    RSFLAG_SKIP_NODE     = 0x400, /* only keep the matrix-stack bookkeeping */
};

enum {
    OPT_RESTORE       = 0x40, /* operand byte 4 is a matrix-stack slot to restore */
    OPT_STORE         = 0x20, /* operand byte 4 is a matrix-stack slot to store */
    OPT_RESTORE_STORE = 0x60, /* restore from byte 5, store to byte 4 */
};

enum {
    GEOP_MTX_STORE   = 0x13,
    GEOP_MTX_RESTORE = 0x14,
};

enum {
    CB_TIMING_NONE = 0,
    CB_TIMING_A    = 1, /* before the joint result is computed */
    CB_TIMING_B    = 2, /* before the joint matrix is sent */
    CB_TIMING_C    = 3, /* after the joint matrix is sent */
};

enum {
    SRTFLAG_TRANS_ZERO  = 0x1,
    SRTFLAG_ROT_ZERO    = 0x2,
    SRTFLAG_PIVOT       = 0x8,
    SRTFLAG_PIVOT_MASK  = 0xF0,
    SRTFLAG_PIVOT_SHIFT = 4,
    SRTFLAG_PIVOT_MINUS = 0x100,
    SRTFLAG_NEG_C       = 0x200,
    SRTFLAG_NEG_D       = 0x400,
};

enum {
    RESULTFLAG_ROT_ZERO   = 0x2,
    RESULTFLAG_TRANS_ZERO = 0x4,
};

enum {
    FX32_ONE       = 0x1000,
    NODE_INFO_DICT = 0x6, /* u16 offset of the node dictionary in pResNodeInfo */
    BASE_CMD_LEN   = 4,
};

/* Callback timing for this command, NONE when no callback is installed. */
static inline u32 cb_timing(Rs *rs) {
    return rs->cbFunc ? rs->cbTiming : 0;
}

/* Runs the callback at `want`, refreshes the timing, returns its skip request. */
static inline u32 cb_check(Rs *rs, u32 *timing, u32 want) {
    if (*timing == want) {
        rs->flag &= ~RSFLAG_CB_SKIP;
        rs->cbFunc(rs);
        *timing = cb_timing(rs);
        return rs->flag & RSFLAG_CB_SKIP;
    }
    return 0;
}

/*
 * Node-description command handler (entry in the command table data_02101498).
 * Operand byte 1 is the node index. Unless the node is skipped it optionally
 * restores a matrix-stack slot, builds the joint result (from the record,
 * the blend callback, or the node's packed translation/rotation data,
 * including the pivot-compressed rotation form), hands it to funcJntScale
 * and funcJntMtx, optionally stores the matrix to a slot, and finally emits
 * a matrix store for nodes with a nonzero entry in data_027e02bc. The
 * callback is run at timing A/B/C and can skip each step.
 */
void func_01ffbf00(Rs *rs, u32 opt) {
    u32 cmdLen = BASE_CMD_LEN;
    u32 idx    = rs->c[1];
    u32 timing;
    JntResult *res;

    rs->currentNodeDesc = idx;
    rs->flag |= RSFLAG_NODEDESC_SEEN;

    if (rs->flag & RSFLAG_SKIP_NODE) {
        if (opt == OPT_RESTORE || opt == OPT_RESTORE_STORE) {
            cmdLen++;
        }
        if (opt == OPT_STORE || opt == OPT_RESTORE_STORE) {
            cmdLen++;
            if (!(rs->flag & RSFLAG_NO_GECMD)) {
                u32 arg = rs->c[4];
                NNS_G3dGeBufferOP_N(GEOP_MTX_RESTORE, &arg, 1);
            }
        }
        rs->c += cmdLen;
        return;
    }

    if (opt == OPT_RESTORE || opt == OPT_RESTORE_STORE) {
        u32 arg;
        cmdLen++;
        if (opt == OPT_RESTORE) {
            arg = rs->c[4];
        } else {
            arg = rs->c[5];
        }
        if (!(rs->flag & RSFLAG_NO_GECMD)) {
            NNS_G3dGeBufferOP_N(GEOP_MTX_RESTORE, &arg, 1);
        }
    }

    rs->pJntAnmResult = &rs->tmpJntAnmResult;
    timing            = cb_timing(rs);
    if (!cb_check(rs, &timing, CB_TIMING_A)) {
        u32 useRec;
        if (rs->pRenderObj->recJntAnm) {
            res    = &rs->pRenderObj->recJntAnm[idx];
            useRec = !(rs->flag & RSFLAG_RECORD);
        } else {
            res    = &rs->tmpJntAnmResult;
            useRec = 0;
        }
        if (!useRec) {
            res->flag = 0;
            if (!rs->pRenderObj->anmJnt || !rs->pRenderObj->funcBlendJnt(res, rs->pRenderObj->anmJnt, idx)) {
                u8 *data = rs->pResNodeInfo;
                u8 *dict = data + *(u16 *) (data + NODE_INFO_DICT);
                u8 *p;
                Srt *srt = (Srt *) (data + *(u32 *) (dict + 4 + *(u16 *) dict * idx));
                p        = (u8 *) (srt + 1);

                if (srt->flag & SRTFLAG_TRANS_ZERO) {
                    res->flag |= RESULTFLAG_TRANS_ZERO;
                } else {
                    res->trans[0] = ((s32 *) p)[0];
                    res->trans[1] = ((s32 *) p)[1];
                    res->trans[2] = ((s32 *) p)[2];
                    p += 12;
                }

                if (srt->flag & SRTFLAG_ROT_ZERO) {
                    res->flag |= RESULTFLAG_ROT_ZERO;
                } else if (srt->flag & SRTFLAG_PIVOT) {
                    s32 idxPivot = (srt->flag & SRTFLAG_PIVOT_MASK) >> SRTFLAG_PIVOT_SHIFT;
                    s32 B;
                    s32 A;
                    A = ((s16 *) p)[0];
                    B = ((s16 *) p)[1];
                    func_020cf518(res->rot);
                    res->rot[idxPivot]                   = (srt->flag & SRTFLAG_PIVOT_MINUS) ? -FX32_ONE : FX32_ONE;
                    res->rot[pivotUtil_[idxPivot][0]]    = A;
                    res->rot[data_020f88a1[idxPivot][0]] = B;
                    res->rot[data_020f88a2[idxPivot][0]] = (srt->flag & SRTFLAG_NEG_C) ? -B : B;
                    res->rot[data_020f88a3[idxPivot][0]] = (srt->flag & SRTFLAG_NEG_D) ? -A : A;
                    p += 4;
                } else {
                    res->rot[0] = srt->m0;
                    res->rot[1] = ((s16 *) p)[0];
                    res->rot[2] = ((s16 *) p)[1];
                    res->rot[3] = ((s16 *) p)[2];
                    res->rot[4] = ((s16 *) p)[3];
                    res->rot[5] = ((s16 *) p)[4];
                    res->rot[6] = ((s16 *) p)[5];
                    res->rot[7] = ((s16 *) p)[6];
                    res->rot[8] = ((s16 *) p)[7];
                    p += 16;
                }
                rs->funcJntScale(res, p, rs->c, srt->flag);
            }
        }
        rs->pJntAnmResult = res;
    }

    if (!cb_check(rs, &timing, CB_TIMING_B) && !(rs->flag & RSFLAG_NO_GECMD)) {
        rs->funcJntMtx(rs->pJntAnmResult);
    }
    rs->pJntAnmResult = 0;

    {
        u32 skip = 0;
        if (timing == CB_TIMING_C) {
            rs->flag &= ~RSFLAG_CB_SKIP;
            rs->cbFunc(rs);
            skip = rs->flag & RSFLAG_CB_SKIP;
        }
        if (opt == OPT_STORE || opt == OPT_RESTORE_STORE) {
            cmdLen++;
            if (!skip && !(rs->flag & RSFLAG_NO_GECMD)) {
                u32 arg = rs->c[4];
                NNS_G3dGeBufferOP_N(GEOP_MTX_STORE, &arg, 1);
            }
        }
    }
    {
        u32 arg = data_027e02bc[idx];
        if (arg) {
            NNS_G3dGeBufferOP_N(GEOP_MTX_STORE, &arg, 1);
        }
    }
    rs->c += cmdLen;
}
