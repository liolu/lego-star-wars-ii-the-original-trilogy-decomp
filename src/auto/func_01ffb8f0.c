#include "m2c.h"

typedef struct {
    /* 0x00 */ u32 flag;
    /* 0x04 */ u32 prmMatColor0;
    /* 0x08 */ u32 prmMatColor1;
    /* 0x0C */ u32 prmPolygonAttr;
    /* 0x10 */ u32 prmTexImage;
    /* 0x14 */ u32 prmTexPltt;
    /* 0x18 */ s32 scaleS;
    /* 0x1C */ s32 scaleT;
    /* 0x20 */ s16 rotSin;
    /* 0x22 */ s16 rotCos;
    /* 0x24 */ s32 transS;
    /* 0x28 */ s32 transT;
    /* 0x2C */ u16 origWidth;
    /* 0x2E */ u16 origHeight;
    /* 0x30 */ s32 magW;
    /* 0x34 */ s32 magH;
} MatResult;

typedef struct {
    /* 0x00 */ u16 itemTag;
    /* 0x02 */ u16 size;
    /* 0x04 */ u32 diffAmb;
    /* 0x08 */ u32 specEmi;
    /* 0x0C */ u32 polyAttr;
    /* 0x10 */ u32 polyAttrMask;
    /* 0x14 */ u32 texImageParam;
    /* 0x18 */ u32 texImageParamMask;
    /* 0x1C */ u16 texPlttBase;
    /* 0x1E */ u16 flag;
    /* 0x20 */ u16 origWidth;
    /* 0x22 */ u16 origHeight;
    /* 0x24 */ s32 magW;
    /* 0x28 */ s32 magH;
} MatData;

typedef struct {
    /* 0x00 */ u8 pad00[8];
    /* 0x08 */ void *anmMat;
    /* 0x0C */ s32 (*funcBlendMat)(MatResult *, void *, u32);
    /* 0x10 */ u8 pad10[0x38 - 0x10];
    /* 0x38 */ MatResult *recMatAnm;
    /* 0x3C */ u32 anmMatBits[1];
} RenderObj;

typedef struct Rs Rs;
struct Rs {
    /* 0x00 */ u8 pad00[4];
    /* 0x04 */ RenderObj *pRenderObj;
    /* 0x08 */ u32 flag;
    /* 0x0C */ void (*cbVecFunc[32])(Rs *);
    /* 0x8C */ u8 cbVecTiming[32];
    /* 0xAC */ u8 pad_ac;
    /* 0xAD */ u8 currentMat;
    /* 0xAE */ u8 padAE[2];
    /* 0xB0 */ MatResult *pMatAnmResult;
    /* 0xB4 */ u8 padB4[8];
    /* 0xBC */ u32 isMatCached[7];
    /* 0xD8 */ u8 *pResMat;
    /* 0xDC */ u8 padDC[0xF0 - 0xDC];
    /* 0xF0 */ void (*funcTexMtx)(MatResult *);
    /* 0xF4 */ MatResult tmpMatAnmResult;
};

typedef struct {
    MatResult matCache[1];
} RSOnGlb;

typedef struct {
    u8 pad[0x94];
    u32 prmMatColor0;
    u32 prmMatColor1;
    u32 prmPolygonAttr;
} Glb;

enum {
    SBC_MAT  = 4,                /* callback slot of the material command */
    TIMING_A = 1,               /* callback before the result is computed */
    TIMING_B = 2,               /* callback before the result is sent to the GE */
    TIMING_C = 3,               /* callback after everything */

    RSFLAG_MAT_HIDDEN = 0x2,    /* set when the material's polygon alpha is 0 */
    RSFLAG_MAT_VALID  = 0x8,     /* currentMat holds a material index */
    RSFLAG_SKIP       = 0x40,         /* set by a callback to skip the next step */
    RSFLAG_RECORD     = 0x80,       /* recompute and record into recMatAnm */
    RSFLAG_NO_GE      = 0x100,       /* do not emit geometry commands */

    OPT_REUSE       = 0x20,           /* a cached result may be reused */
    OPT_REUSE_STORE = 0x40,     /* reuse, else compute into the global cache */

    MATFLAG_TEXMTX_USE = 0x1,   /* texture matrix data follows the MatData */
    MATFLAG_SCALE_ONE  = 0x2,    /* no scale pair stored */
    MATFLAG_ROT_ZERO   = 0x4,     /* no sin/cos pair stored */
    MATFLAG_TRANS_ZERO = 0x8,   /* no translation pair stored */
    MATFLAG_WIREFRAME  = 0x20,

    RESFLAG_SCALE_ONE  = 0x1,
    RESFLAG_ROT_ZERO   = 0x2,
    RESFLAG_TRANS_ZERO = 0x4,
    RESFLAG_TEXMTX_SET = 0x8,
    RESFLAG_TEXMTX_ANY = 0x18,  /* texture matrix set by the data or a blend */
    RESFLAG_WIREFRAME  = 0x20,

    POLYATTR_ALPHA_MASK = 0x1F0000,
    COLOR_MASK_SHIFT0   = 6,      /* mat->flag bits 6-8 select the diffAmb mask */
    COLOR_MASK_SHIFT1   = 9,      /* mat->flag bits 9-11 select the specEmi mask */
    COLOR_MASK_SEL      = 7,

    /* packed GE opcodes: DIF_AMB, SPE_EMI, POLYGON_ATTR / TEXIMAGE_PARAM, PLTT_BASE */
    GE_OPS_COLOR_POLY = 0x293130,
    GE_OPS_TEX        = 0x2B2A,
    GE_PARAM_COUNT    = 6,
};

extern RSOnGlb NNS_G3dRSOnGlb;
extern Glb NNS_G3dGlb;
extern const u32 data_020f8880[]; /* colour masks selected by mat->flag fields */

void NNS_G3dGeBufferOP_N(u32 op, const u32 *args, u32 count);

static inline u32 bitvec_check(const u32 *vec, u32 idx) {
    return vec[idx >> 5] & (1 << (idx & 31));
}

static inline void bitvec_set(u32 *vec, u32 idx) {
    vec[idx >> 5] |= 1 << (idx & 31);
}

/* Callback helpers: run slot `cmd`'s callback at its timing, refresh the
 * timing (the callback may change it) and report whether it asked to skip. */
static inline u32 cb_check_a(Rs *rs, u32 cmd, s32 *pTiming) {
    s32 timing = rs->cbVecFunc[cmd] ? rs->cbVecTiming[cmd] : 0;
    *pTiming   = timing;
    if (timing == TIMING_A) {
        rs->flag &= ~RSFLAG_SKIP;
        rs->cbVecFunc[cmd](rs);
        *pTiming = rs->cbVecFunc[cmd] ? rs->cbVecTiming[cmd] : 0;
        return rs->flag & RSFLAG_SKIP;
    }
    return 0;
}

static inline u32 cb_check_b(Rs *rs, u32 cmd, s32 *pTiming) {
    if (*pTiming == TIMING_B) {
        rs->flag &= ~RSFLAG_SKIP;
        rs->cbVecFunc[cmd](rs);
        *pTiming = rs->cbVecFunc[cmd] ? rs->cbVecTiming[cmd] : 0;
        return rs->flag & RSFLAG_SKIP;
    }
    return 0;
}

static inline void cb_check_c(Rs *rs, u32 cmd, s32 timing) {
    if (timing == TIMING_C) {
        rs->flag &= ~RSFLAG_SKIP;
        rs->cbVecFunc[cmd](rs);
    }
}

/*
 * Material command (NitroSystem G3D SBC MAT, default handler). Selects
 * material idxMat and builds its result: reused from the render object's
 * record or the global cache when allowed, otherwise computed from mat
 * (colours and polygon attributes merged with NNS_G3dGlb under masks, texture
 * params, optional texture matrix data, optional blend callback). Unless
 * skipped, sends colour/polygon/texture commands to the GE buffer, or marks
 * the material hidden when its alpha is 0. Callbacks run at timings A/B/C.
 */
void func_01ffb8f0(Rs *rs, u32 opt, const MatData *mat, u32 idxMat) {
    s32 timing;

    rs->currentMat = idxMat;
    rs->flag |= RSFLAG_MAT_VALID;
    rs->pMatAnmResult = &rs->tmpMatAnmResult;

    if (!cb_check_a(rs, SBC_MAT, &timing)) {
        MatResult *res;
        s32 matFlag;
        MatResult *rec = rs->pRenderObj->recMatAnm;

        if (rec != NULL && !(rs->flag & RSFLAG_RECORD)) {
            res = &rec[idxMat];
        } else if ((opt == OPT_REUSE || opt == OPT_REUSE_STORE) && bitvec_check(rs->isMatCached, idxMat)) {
            if (rec != NULL) {
                res = &rec[idxMat];
            } else {
                res = &NNS_G3dRSOnGlb.matCache[idxMat];
            }
        } else {
            if (rec != NULL) {
                bitvec_set(rs->isMatCached, idxMat);
                res = &rs->pRenderObj->recMatAnm[idxMat];
            } else if (opt == OPT_REUSE_STORE) {
                bitvec_set(rs->isMatCached, idxMat);
                res = &NNS_G3dRSOnGlb.matCache[idxMat];
            } else {
                res = &rs->tmpMatAnmResult;
            }

            res->flag = 0;
            {
                u8 *data   = rs->pResMat;
                u8 *list   = data + 4;
                u8 *tbl    = list + *(u16 *) (data + 0xA);
                MatData *m = (MatData *) (data + *(u32 *) (tbl + *(u16 *) tbl * idxMat + 4));
                if (m->flag & MATFLAG_WIREFRAME) {
                    res->flag |= RESFLAG_WIREFRAME;
                }
            }
            matFlag = mat->flag;
            {
                Glb *glb = &NNS_G3dGlb;
                u32 mask;
                mask                = data_020f8880[(matFlag >> COLOR_MASK_SHIFT0) & COLOR_MASK_SEL];
                res->prmMatColor0   = (glb->prmMatColor0 & ~mask) | (mat->diffAmb & mask);
                mask                = data_020f8880[(matFlag >> COLOR_MASK_SHIFT1) & COLOR_MASK_SEL];
                res->prmMatColor1   = (glb->prmMatColor1 & ~mask) | (mat->specEmi & mask);
                res->prmPolygonAttr = (glb->prmPolygonAttr & ~mat->polyAttrMask) | (mat->polyAttr & mat->polyAttrMask);
            }
            res->prmTexImage = mat->texImageParam;
            res->prmTexPltt  = mat->texPlttBase;

            if (matFlag & MATFLAG_TEXMTX_USE) {
                const u8 *p = (const u8 *) mat + 0x2C;
                if (!(matFlag & MATFLAG_SCALE_ONE)) {
                    res->scaleS = ((const s32 *) p)[0];
                    res->scaleT = ((const s32 *) p)[1];
                    p += 8;
                } else {
                    res->flag |= RESFLAG_SCALE_ONE;
                }
                if (!(mat->flag & MATFLAG_ROT_ZERO)) {
                    res->rotSin = ((const s16 *) p)[0];
                    res->rotCos = ((const s16 *) p)[1];
                    p += 4;
                } else {
                    res->flag |= RESFLAG_ROT_ZERO;
                }
                if (!(mat->flag & MATFLAG_TRANS_ZERO)) {
                    res->transS = ((const s32 *) p)[0];
                    res->transT = ((const s32 *) p)[1];
                } else {
                    res->flag |= RESFLAG_TRANS_ZERO;
                }
                res->flag |= RESFLAG_TEXMTX_SET;
            }

            {
                RenderObj *obj = rs->pRenderObj;
                if (obj->anmMat != NULL && bitvec_check(obj->anmMatBits, idxMat)) {
                    obj->funcBlendMat(res, obj->anmMat, idxMat);
                }
            }

            if (res->flag & RESFLAG_TEXMTX_ANY) {
                res->origWidth  = mat->origWidth;
                res->origHeight = mat->origHeight;
                res->magW       = mat->magW;
                res->magH       = mat->magH;
            }
        }
        rs->pMatAnmResult = res;
    }

    if (!cb_check_b(rs, SBC_MAT, &timing)) {
        MatResult *res = rs->pMatAnmResult;
        if (res->prmPolygonAttr & POLYATTR_ALPHA_MASK) {
            if (res->flag & RESFLAG_WIREFRAME) {
                res->prmPolygonAttr &= ~POLYATTR_ALPHA_MASK;
            }
            rs->flag &= ~RSFLAG_MAT_HIDDEN;
            if (!(rs->flag & RSFLAG_NO_GE)) {
                u32 cmd[7];
                cmd[0] = GE_OPS_COLOR_POLY;
                cmd[1] = res->prmMatColor0;
                cmd[2] = res->prmMatColor1;
                cmd[3] = res->prmPolygonAttr;
                cmd[4] = GE_OPS_TEX;
                cmd[5] = res->prmTexImage;
                cmd[6] = res->prmTexPltt;
                NNS_G3dGeBufferOP_N(cmd[0], &cmd[1], GE_PARAM_COUNT);
                if (res->flag & RESFLAG_TEXMTX_ANY) {
                    rs->funcTexMtx(res);
                }
            }
        } else {
            rs->flag |= RSFLAG_MAT_HIDDEN;
        }
    }

    cb_check_c(rs, SBC_MAT, timing);
}
