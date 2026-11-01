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

typedef struct NNSG3dJntAnmResult {
    u32 flag;
    VecFx32 scale;
    VecFx32 scaleEx0;
    VecFx32 scaleEx1;
    MtxFx33 rot;
    VecFx32 trans;
} NNSG3dJntAnmResult;

typedef struct NNSG3dResAnmHeader {
    u8 category0;
    u8 revision;
    u16 category1;
} NNSG3dResAnmHeader;

typedef struct NNSG3dResJntAnm {
    NNSG3dResAnmHeader anmHeader;
    u16 numFrame;
    u16 numNode;
    u32 annFlag;
    u32 ofsRot3;
    u32 ofsRot5;
    u16 ofsTag[1];
} NNSG3dResJntAnm;

typedef void (*NNSG3dGetJointScale)(NNSG3dJntAnmResult *pResult, const fx32 *p, const u8 *cmd, u32 srtflag);

typedef struct RenderState {
    const u8 *c;
    u8 pad[0xe4];
    NNSG3dGetJointScale funcJntScale;
} RenderState;

extern RenderState *NNS_G3dRS;

extern void getTransData_(fx32 *pVal, fx32 frame, const u32 *pData, const NNSG3dResJntAnm *pAnm);
extern void getTransDataEx_(fx32 *pVal, fx32 frame, const u32 *pData, const NNSG3dResJntAnm *pAnm);
extern void getRotData_(MtxFx33 *pRot, fx32 frame, const u32 *pData, const NNSG3dResJntAnm *pAnm);
extern void getRotDataEx_(MtxFx33 *pRot, fx32 frame, const u32 *pData, const NNSG3dResJntAnm *pAnm);
extern BOOL getRotDataByIdx_(MtxFx33 *pRot, const void *pArrayRot3, const void *pArrayRot5, u32 info);
extern void getScaleData_(fx32 *pVal, fx32 frame, const u32 *pData, const NNSG3dResJntAnm *pAnm);
extern void getScaleDataEx_(fx32 *pVal, fx32 frame, const u32 *pData, const NNSG3dResJntAnm *pAnm);
extern void getMdlTrans_(NNSG3dJntAnmResult *pResult);
extern void getMdlRot_(NNSG3dJntAnmResult *pResult);
extern void getMdlScale_(NNSG3dJntAnmResult *pResult);

enum {
    FX32_SHIFT               = 12,
    FX32_DEC_MASK            = 0xfff,
    ANM_OPTION_INTERPOLATION = 0x1,

    SRTINFO_IDENTITY   = 0x0001,
    SRTINFO_TRANS_ZERO = 0x0002,
    SRTINFO_TRANS_MDL  = 0x0004,
    SRTINFO_TX_CONST   = 0x0008,
    SRTINFO_TY_CONST   = 0x0010,
    SRTINFO_TZ_CONST   = 0x0020,
    SRTINFO_ROT_ZERO   = 0x0040,
    SRTINFO_ROT_MDL    = 0x0080,
    SRTINFO_ROT_CONST  = 0x0100,
    SRTINFO_SCALE_ONE  = 0x0200,
    SRTINFO_SCALE_MDL  = 0x0400,
    SRTINFO_SX_CONST   = 0x0800,
    SRTINFO_SY_CONST   = 0x1000,
    SRTINFO_SZ_CONST   = 0x2000,

    RESULTFLAG_SCALE_ONE  = 0x1,
    RESULTFLAG_ROT_ZERO   = 0x2,
    RESULTFLAG_TRANS_ZERO = 0x4,

    SRTFLAG_SCALE_ONE = 0x4,
};

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
 * Joint SRT result for joint dataIdx of a .nsbca animation at fx32 frame
 * Frame (already clamped by NNSi_G3dAnmCalcNsBca). The joint's tag word is
 * found via ofsTag[dataIdx]; the words after it are consumed in order:
 *  - identity tag: result flags scale-one | rot-zero | trans-zero, no data;
 *  - translation x/y/z: one constant word each, or a 2-word key-array
 *    descriptor read by getTransData_ (getTransDataEx_ when the frame has a
 *    fractional part and the animation's interpolation bit is set); or
 *    trans-zero flag / model rest translation (getMdlTrans_);
 *  - rotation: 2-word descriptor (getRotData_/getRotDataEx_), or one index
 *    word into the ofsRot3/ofsRot5 tables (getRotDataByIdx_), whose third
 *    row is rebuilt as row0 x row1; or rot-zero flag / getMdlRot_;
 *  - scale x/y/z: (scale, inverse scale) pairs, constant or keyed, gathered
 *    as 3 scales then 3 inverse scales; or scale-one flag / getMdlScale_
 *    (which returns without the hook below).
 * Finally the render state's joint scale hook gets the result, the scale
 * block, the current SBC command and the model SRT scale-one flag.
 */
void getJntSRTAnmResult_(const NNSG3dResJntAnm *pJntAnm, u32 dataIdx, fx32 Frame, NNSG3dJntAnmResult *pResult) {
    fx32 scale[6]; /* scale x, y, z, then inverse scale x, y, z */
    u32 tag;
    BOOL interp;
    const u32 *pData;
    const u32 *pTag;

    pTag = (const u32 *) ((const u8 *) pJntAnm + pJntAnm->ofsTag[dataIdx]);
    tag  = *pTag;

    if (tag & SRTINFO_IDENTITY) {
        pResult->flag = RESULTFLAG_SCALE_ONE | RESULTFLAG_ROT_ZERO | RESULTFLAG_TRANS_ZERO;
        goto POST_PROCESS;
    }

    pData = pTag + 1;

    interp = (Frame & FX32_DEC_MASK) && (pJntAnm->annFlag & ANM_OPTION_INTERPOLATION);

    pResult->flag = 0;

    if (!(tag & (SRTINFO_TRANS_ZERO | SRTINFO_TRANS_MDL))) {
        if (!(tag & SRTINFO_TX_CONST)) {
            if (interp) {
                getTransDataEx_(&pResult->trans.x, Frame, pData, pJntAnm);
            } else {
                getTransData_(&pResult->trans.x, Frame, pData, pJntAnm);
            }
            pData += 2;
        } else {
            pResult->trans.x = *(const fx32 *) pData++;
        }

        if (!(tag & SRTINFO_TY_CONST)) {
            if (interp) {
                getTransDataEx_(&pResult->trans.y, Frame, pData, pJntAnm);
            } else {
                getTransData_(&pResult->trans.y, Frame, pData, pJntAnm);
            }
            pData += 2;
        } else {
            pResult->trans.y = *(const fx32 *) pData++;
        }

        if (!(tag & SRTINFO_TZ_CONST)) {
            if (interp) {
                getTransDataEx_(&pResult->trans.z, Frame, pData, pJntAnm);
            } else {
                getTransData_(&pResult->trans.z, Frame, pData, pJntAnm);
            }
            pData += 2;
        } else {
            pResult->trans.z = *(const fx32 *) pData++;
        }
    } else {
        if (tag & SRTINFO_TRANS_ZERO) {
            pResult->flag |= RESULTFLAG_TRANS_ZERO;
        } else {
            getMdlTrans_(pResult);
        }
    }

    if (!(tag & (SRTINFO_ROT_ZERO | SRTINFO_ROT_MDL))) {
        if (!(tag & SRTINFO_ROT_CONST)) {
            if (interp) {
                getRotDataEx_(&pResult->rot, Frame, pData, pJntAnm);
            } else {
                getRotData_(&pResult->rot, Frame, pData, pJntAnm);
            }
            pData += 2;
        } else {
            if (getRotDataByIdx_(&pResult->rot, (const u8 *) pJntAnm + pJntAnm->ofsRot3,
                                 (const u8 *) pJntAnm + pJntAnm->ofsRot5, *pData)) {
                vecCross_((const VecFx32 *) &pResult->rot._00, (const VecFx32 *) &pResult->rot._10,
                          (VecFx32 *) &pResult->rot._20);
            }
            pData += 1;
        }
    } else {
        if (tag & SRTINFO_ROT_ZERO) {
            pResult->flag |= RESULTFLAG_ROT_ZERO;
        } else {
            getMdlRot_(pResult);
        }
    }

    if (!(tag & (SRTINFO_SCALE_ONE | SRTINFO_SCALE_MDL))) {
        if (!(tag & SRTINFO_SX_CONST)) {
            fx32 tmp[2];
            if (interp) {
                getScaleDataEx_(tmp, Frame, pData, pJntAnm);
            } else {
                getScaleData_(tmp, Frame, pData, pJntAnm);
            }
            scale[0] = tmp[0];
            scale[3] = tmp[1];
        } else {
            scale[0] = (fx32) pData[0];
            scale[3] = (fx32) pData[1];
        }

        if (!(tag & SRTINFO_SY_CONST)) {
            fx32 tmp[2];
            if (interp) {
                getScaleDataEx_(tmp, Frame, pData + 2, pJntAnm);
            } else {
                getScaleData_(tmp, Frame, pData + 2, pJntAnm);
            }
            scale[1] = tmp[0];
            scale[4] = tmp[1];
        } else {
            scale[1] = (fx32) pData[2];
            scale[4] = (fx32) pData[3];
        }

        if (!(tag & SRTINFO_SZ_CONST)) {
            fx32 tmp[2];
            if (interp) {
                getScaleDataEx_(tmp, Frame, pData + 4, pJntAnm);
            } else {
                getScaleData_(tmp, Frame, pData + 4, pJntAnm);
            }
            scale[2] = tmp[0];
            scale[5] = tmp[1];
        } else {
            scale[2] = (fx32) pData[4];
            scale[5] = (fx32) pData[5];
        }
    } else {
        if (tag & SRTINFO_SCALE_ONE) {
            pResult->flag |= RESULTFLAG_SCALE_ONE;
        } else {
            getMdlScale_(pResult);
            return;
        }
    }

POST_PROCESS:
    (*NNS_G3dRS->funcJntScale)(pResult, scale, NNS_G3dRS->c, (pResult->flag & RESULTFLAG_SCALE_ONE) ? SRTFLAG_SCALE_ONE : 0);
}
