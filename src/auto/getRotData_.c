#include "m2c.h"

typedef s32 fx32;
typedef s32 BOOL;

typedef struct VecFx32 {
    fx32 x, y, z;
} VecFx32;

typedef union MtxFx33 {
    struct {
        fx32 _00, _01, _02;
        fx32 _10, _11, _12;
        fx32 _20, _21, _22;
    };
    fx32 m[3][3];
    fx32 a[9];
} MtxFx33;

typedef struct NNSG3dResAnmHeader {
    u8 category0;
    u8 revision;
    u16 category1;
} NNSG3dResAnmHeader;

typedef struct NNSG3dResJntAnm {
    NNSG3dResAnmHeader anmHeader;
    u16 numFrame;
    u16 numNode;
    u32 flag;
    u32 ofsRot3;
    u32 ofsRot5;
} NNSG3dResJntAnm;

enum {
    SRTINFO_STEP_MASK         = 0xc0000000, /* 0: one record per frame */
    SRTINFO_STEP_2            = 0x40000000, /* set: one record per 2 frames, else per 4 */
    SRTINFO_LAST_INTERP_MASK  = 0x1fff0000, /* last frame that may still blend two records */
    SRTINFO_LAST_INTERP_SHIFT = 16,
    FX32_SHIFT                = 12, /* 20.12 fixed point */
};

BOOL getRotDataByIdx_(MtxFx33 *pRot, const void *pivotRecords, const void *packedRecords, u16 info);
void VEC_Normalize(const VecFx32 *src, VecFx32 *dst);

/* axb = a x b in 20.12 fixed point (32-bit products). */
static inline void vecCross_(const VecFx32 *a, const VecFx32 *b, VecFx32 *axb) {
    fx32 x, y, z;

    x = (a->y * b->z - a->z * b->y) >> FX32_SHIFT;
    y = (a->z * b->x - a->x * b->z) >> FX32_SHIFT;
    z = (a->x * b->y - a->y * b->x) >> FX32_SHIFT;

    axb->x = x;
    axb->y = y;
    axb->z = z;
}

/*
 * Rotation track of a joint animation at time Frame (20.12, whole frames used).
 * pData[0] is the track info word, pData[1] the offset (from pJntAnm) of its
 * u16 record-index array; records are decoded by getRotDataByIdx_.
 *
 * Step 1 uses index = frame. Step 2/4 store one index per 2/4 frames:
 * frames on a step use it directly; frames past the last-interp frame read
 * the tail entries directly; otherwise two neighbouring records are summed
 * 1:1 (half step) or 3:1 toward the nearer one (quarter steps, step 4), and
 * rows 0 and 1 are renormalized. Row 2 is blended the same way unless a
 * record came from the packed form (which leaves row 2 unset); then, as for
 * a single packed record, it is rebuilt as row0 x row1.
 */
void getRotData_(MtxFx33 *pRot, fx32 Frame, const u32 *pData, const NNSG3dResJntAnm *pJntAnm) {
    u32 frame           = (u32) (Frame >> FX32_SHIFT);
    const u16 *pDataRot = (const u16 *) ((const u8 *) pJntAnm + pData[1]);
    u32 info            = pData[0];
    u32 idx;
    u32 idx_sub;
    u32 last_interp;
    const void *pArray3 = (const u8 *) pJntAnm + pJntAnm->ofsRot3; /* pivot records */
    const void *pArray5 = (const u8 *) pJntAnm + pJntAnm->ofsRot5; /* packed records */

    if (!(info & SRTINFO_STEP_MASK)) {
        idx = frame;
        goto getrot_nointerp;
    }

    last_interp = (info & SRTINFO_LAST_INTERP_MASK) >> SRTINFO_LAST_INTERP_SHIFT;

    if (info & SRTINFO_STEP_2) {
        if (frame & 1) {
            if (frame > last_interp) {
                idx = (last_interp >> 1) + 1;
                goto getrot_nointerp;
            } else {
                idx = frame >> 1;
                goto getrot_interp_50;
            }
        } else {
            idx = frame >> 1;
            goto getrot_nointerp;
        }
    } else {
        if (frame & 3) {
            if (frame > last_interp) {
                idx = (last_interp >> 2) + (frame & 3);
                goto getrot_nointerp;
            }

            if (frame & 1) {
                MtxFx33 r;
                BOOL doCross = 0;

                if (frame & 2) {
                    idx_sub = frame >> 2;
                    idx     = idx_sub + 1;
                } else {
                    idx     = frame >> 2;
                    idx_sub = idx + 1;
                }

                doCross |= getRotDataByIdx_(pRot, pArray3, pArray5, pDataRot[idx]);
                doCross |= getRotDataByIdx_(&r, pArray3, pArray5, pDataRot[idx_sub]);

                /* 3:1 toward pDataRot[idx]; scale is dropped by normalization */
                pRot->_00 = pRot->_00 * 3 + r._00;
                pRot->_01 = pRot->_01 * 3 + r._01;
                pRot->_02 = pRot->_02 * 3 + r._02;
                pRot->_10 = pRot->_10 * 3 + r._10;
                pRot->_11 = pRot->_11 * 3 + r._11;
                pRot->_12 = pRot->_12 * 3 + r._12;

                VEC_Normalize((VecFx32 *) &pRot->_00, (VecFx32 *) &pRot->_00);
                VEC_Normalize((VecFx32 *) &pRot->_10, (VecFx32 *) &pRot->_10);

                if (!doCross) {
                    pRot->_20 = pRot->_20 * 3 + r._20;
                    pRot->_21 = pRot->_21 * 3 + r._21;
                    pRot->_22 = pRot->_22 * 3 + r._22;
                    VEC_Normalize((VecFx32 *) &pRot->_20, (VecFx32 *) &pRot->_20);
                } else {
                    vecCross_((VecFx32 *) &pRot->_00, (VecFx32 *) &pRot->_10, (VecFx32 *) &pRot->_20);
                }
                return;
            } else {
                idx = frame >> 2;
                goto getrot_interp_50;
            }
        } else {
            idx = frame >> 2;
            goto getrot_nointerp;
        }
    }

getrot_interp_50: {
    MtxFx33 r;
    BOOL doCross = 0;

    doCross |= getRotDataByIdx_(pRot, pArray3, pArray5, pDataRot[idx]);
    doCross |= getRotDataByIdx_(&r, pArray3, pArray5, pDataRot[idx + 1]);

    pRot->_00 += r._00;
    pRot->_01 += r._01;
    pRot->_02 += r._02;
    pRot->_10 += r._10;
    pRot->_11 += r._11;
    pRot->_12 += r._12;

    VEC_Normalize((VecFx32 *) &pRot->_00, (VecFx32 *) &pRot->_00);
    VEC_Normalize((VecFx32 *) &pRot->_10, (VecFx32 *) &pRot->_10);

    if (!doCross) {
        pRot->_20 += r._20;
        pRot->_21 += r._21;
        pRot->_22 += r._22;
        VEC_Normalize((VecFx32 *) &pRot->_20, (VecFx32 *) &pRot->_20);
    } else {
        vecCross_((VecFx32 *) &pRot->_00, (VecFx32 *) &pRot->_10, (VecFx32 *) &pRot->_20);
    }
}
    return;

getrot_nointerp:
    if (getRotDataByIdx_(pRot, pArray3, pArray5, pDataRot[idx])) {
        vecCross_((VecFx32 *) &pRot->_00, (VecFx32 *) &pRot->_10, (VecFx32 *) &pRot->_20);
    }
}
