/* compiler: 2.0/sp1p5 -O4,p -lang c99 -ipa file -inline on,noauto */
#include "m2c.h"

typedef s32 fx32;
typedef s64 fx64;
typedef s32 BOOL;

typedef struct {
    fx32 _00, _01, _02;
    fx32 _10, _11, _12;
    fx32 _20, _21, _22;
} MtxFx33;

typedef struct {
    fx32 _00, _01, _02;
    fx32 _10, _11, _12;
    fx32 _20, _21, _22;
    fx32 _30, _31, _32;
} MtxFx43;

typedef struct {
    fx32 _00, _01, _02, _03;
    fx32 _10, _11, _12, _13;
    fx32 _20, _21, _22, _23;
    fx32 _30, _31, _32, _33;
} MtxFx44;

typedef struct {
    MtxFx43 invM;
    MtxFx33 invN;
} NNSG3dEvpMtx;

typedef struct {
    u32 size;
    u32 ofsSbc;
    u32 ofsMat;
    u32 ofsShp;
    u32 ofsEvpMtx;
} NNSG3dResMdl;

typedef struct {
    u32 flag;
    NNSG3dResMdl *resMdl;
} NNSG3dRenderObj;

typedef struct {
    u8 *c;
    NNSG3dRenderObj *pRenderObj;
    u8 pad[0xc4];
    u32 isEvpCached[1];
} NNSG3dRS;

typedef struct {
    MtxFx44 m;
    MtxFx33 n;
} EvpCache;

/* data_02113a9c is the .n member of data_02113a5c[0] (same 100-byte stride). */
typedef struct {
    MtxFx33 n;
    MtxFx44 next_m;
} EvpCacheN;

extern EvpCache data_02113a5c[];
extern EvpCacheN data_02113a9c[];

static inline BOOL BitVecCheck(const u32 *vec, u32 idx) {
    return (BOOL) (vec[idx >> 5] & (1 << (idx & 31)));
}

static inline void BitVecSet(u32 *vec, u32 idx) {
    vec[idx >> 5] |= 1 << (idx & 31);
}

enum {
    REG_G3_MTX_MODE     = 0x04000440,
    REG_G3_MTX_STORE    = 0x0400044C,
    REG_G3_MTX_RESTORE  = 0x04000450,
    REG_G3_MTX_IDENTITY = 0x04000454,
};

enum {
    GX_MTXMODE_PROJECTION      = 0,
    GX_MTXMODE_POSITION        = 1,
    GX_MTXMODE_POSITION_VECTOR = 2,
};

enum {
    FX32_SHIFT = 12,
    /* ratio byte is in 1/256 units; << 4 turns it into fx32 (4096 = 1.0) */
    RATIO_TO_FX32_SHIFT = 4,
    /* bytes: header (c[0], c[1] = dest stack slot, c[2] = count) and per entry */
    NODEMIX_HEADER_SIZE = 3,
    NODEMIX_ENTRY_SIZE  = 3,
};

void MIi_CpuClearFast(u32 data, void *destp, u32 size);
void NNS_G3dGeFlushBuffer(void);
void G3_LoadMtx43(const MtxFx43 *m);
void G3_MultMtx43(const MtxFx43 *m);
void G3_MultMtx33(const MtxFx33 *m);
s32 G3X_GetClipMtx(MtxFx44 *m);
s32 G3X_GetVectorMtx(MtxFx33 *m);

/*
 * SBC NODEMIX: blend several joint matrices into one skinning matrix.
 * Command bytes: c[1] = matrix-stack slot that receives the result,
 * c[2] = entry count, then per entry {stack slot, node index, ratio}.
 * Projection is saved and set to identity, so the clip matrix read back
 * equals the position matrix. For each entry whose node is not yet in the
 * per-render cache bit vector, the joint matrix is restored from its stack
 * slot, multiplied by the node's envelope matrices and read back into the
 * static cache (4x4 position, 3x3 vector). The cached matrices are summed
 * with weight ratio/256; an entry's vector matrix is added one iteration
 * late, after the next entry's matrix commands are issued and before they
 * are read back (the last one after the loop).
 * The sums are loaded as the vector and position matrices, projection is
 * restored, the result is stored to slot c[1] and c advances past the command.
 */
void NNSi_G3dFuncSbc_NODEMIX(NNSG3dRS *rs, u32 opt) {
    struct {
        MtxFx43 m;
        MtxFx33 n;
    } sum;
    NNSG3dResMdl *resMdl = rs->pRenderObj->resMdl;
    fx64 weight          = 0;
    u8 *p                = rs->c;
    NNSG3dEvpMtx *evpMtx = (NNSG3dEvpMtx *) ((u8 *) resMdl + resMdl->ofsEvpMtx);
    u32 numEntries       = p[2];
    u32 i;
    MtxFx44 *posMtx;
    const MtxFx33 *vecMtx;

    p += NODEMIX_HEADER_SIZE;
    MIi_CpuClearFast(0, &sum, sizeof(sum));
    NNS_G3dGeFlushBuffer();

    *(volatile u32 *) REG_G3_MTX_MODE     = GX_MTXMODE_PROJECTION;
    *(volatile u32 *) REG_G3_MTX_STORE    = 1;
    *(volatile u32 *) REG_G3_MTX_IDENTITY = 0;
    *(volatile u32 *) REG_G3_MTX_MODE     = GX_MTXMODE_POSITION_VECTOR;

    for (i = 0; i < numEntries; ++i) {
        u32 idxNode   = p[1];
        BOOL isCached = BitVecCheck(&rs->isEvpCached[0], idxNode);
        posMtx        = &data_02113a5c[idxNode].m;

        if (!isCached) {
            BitVecSet(&rs->isEvpCached[0], idxNode);
            *(volatile u32 *) REG_G3_MTX_RESTORE = p[0];
            *(volatile u32 *) REG_G3_MTX_MODE    = GX_MTXMODE_POSITION;
            G3_MultMtx43(&evpMtx[idxNode].invM);
        }

        if (i != 0) {
            sum.n._00 += (weight * vecMtx->_00) >> FX32_SHIFT;
            sum.n._01 += (weight * vecMtx->_01) >> FX32_SHIFT;
            sum.n._02 += (weight * vecMtx->_02) >> FX32_SHIFT;
            sum.n._10 += (weight * vecMtx->_10) >> FX32_SHIFT;
            sum.n._11 += (weight * vecMtx->_11) >> FX32_SHIFT;
            sum.n._12 += (weight * vecMtx->_12) >> FX32_SHIFT;
            sum.n._20 += (weight * vecMtx->_20) >> FX32_SHIFT;
            sum.n._21 += (weight * vecMtx->_21) >> FX32_SHIFT;
            sum.n._22 += (weight * vecMtx->_22) >> FX32_SHIFT;
        }

        if (!isCached) {
            while (G3X_GetClipMtx(posMtx))
                ;
            *(volatile u32 *) REG_G3_MTX_MODE = GX_MTXMODE_POSITION_VECTOR;
            G3_MultMtx33(&evpMtx[idxNode].invN);
        }

        weight = p[2] << RATIO_TO_FX32_SHIFT;
        sum.m._00 += (weight * posMtx->_00) >> FX32_SHIFT;
        sum.m._01 += (weight * posMtx->_01) >> FX32_SHIFT;
        sum.m._02 += (weight * posMtx->_02) >> FX32_SHIFT;
        sum.m._10 += (weight * posMtx->_10) >> FX32_SHIFT;
        sum.m._11 += (weight * posMtx->_11) >> FX32_SHIFT;
        sum.m._12 += (weight * posMtx->_12) >> FX32_SHIFT;
        sum.m._20 += (weight * posMtx->_20) >> FX32_SHIFT;
        sum.m._21 += (weight * posMtx->_21) >> FX32_SHIFT;
        sum.m._22 += (weight * posMtx->_22) >> FX32_SHIFT;
        sum.m._30 += (weight * posMtx->_30) >> FX32_SHIFT;
        sum.m._31 += (weight * posMtx->_31) >> FX32_SHIFT;
        sum.m._32 += (weight * posMtx->_32) >> FX32_SHIFT;

        p += NODEMIX_ENTRY_SIZE;
        vecMtx = &data_02113a9c[idxNode].n;
        if (!isCached) {
            while (G3X_GetVectorMtx((MtxFx33 *) vecMtx))
                ;
        }
    }

    sum.n._00 += (weight * vecMtx->_00) >> FX32_SHIFT;
    sum.n._01 += (weight * vecMtx->_01) >> FX32_SHIFT;
    sum.n._02 += (weight * vecMtx->_02) >> FX32_SHIFT;
    sum.n._10 += (weight * vecMtx->_10) >> FX32_SHIFT;
    sum.n._11 += (weight * vecMtx->_11) >> FX32_SHIFT;
    sum.n._12 += (weight * vecMtx->_12) >> FX32_SHIFT;
    sum.n._20 += (weight * vecMtx->_20) >> FX32_SHIFT;
    sum.n._21 += (weight * vecMtx->_21) >> FX32_SHIFT;
    sum.n._22 += (weight * vecMtx->_22) >> FX32_SHIFT;

    G3_LoadMtx43((const MtxFx43 *) &sum.n);
    *(volatile u32 *) REG_G3_MTX_MODE = GX_MTXMODE_POSITION;
    G3_LoadMtx43(&sum.m);

    *(volatile u32 *) REG_G3_MTX_MODE    = GX_MTXMODE_PROJECTION;
    *(volatile u32 *) REG_G3_MTX_RESTORE = 1;
    *(volatile u32 *) REG_G3_MTX_MODE    = GX_MTXMODE_POSITION_VECTOR;
    *(volatile u32 *) REG_G3_MTX_STORE   = rs->c[1];
    rs->c += NODEMIX_ENTRY_SIZE * (rs->c[2] + 1);
}
