#include "m2c.h"

typedef struct {
    u16 flag;
    s16 _00;
} ResNodeData;

typedef struct {
    u16 sizeUnit;
    u16 ofsName;
    u8 data[4];
} ResDictEntry;

typedef struct {
    u8 revision;
    u8 numEntry;
    u16 sizeDictBlk;
    u16 dummy_;
    u16 ofsEntry;
} ResDict;

typedef struct {
    s32 a[9];
} Mtx33;

typedef struct {
    u32 flag;
    s32 scale[3];
    s32 scaleEx0[3];
    s32 scaleEx1[3];
    Mtx33 rot;
    s32 trans[3];
} JntAnmResult;

typedef struct {
    const u8 *c;
    u8 pad[0xd0];
    const ResDict *pResNodeInfo;
} RenderState;

extern RenderState *NNS_G3dRS;
extern const u8 pivotUtil_[][4];
extern void func_020cf518(void *p);

static inline const void *getResDataByIdx(const ResDict *dict, u32 idx) {
    const ResDictEntry *entry = (const ResDictEntry *) ((const u8 *) dict + dict->ofsEntry);
    return &entry->data[0] + entry->sizeUnit * idx;
}

enum {
    SRTFLAG_TRANS_ZERO     = 0x0001,     /* node stores no translation (3 words skipped) */
    SRTFLAG_ROT_ZERO       = 0x0002,       /* node rotation is identity */
    SRTFLAG_PIVOT_EXIST    = 0x0008,    /* rotation stored compressed as pivot + 2 values */
    SRTFLAG_IDXPIVOT_MASK  = 0x00f0,  /* pivot element index (0..8) */
    SRTFLAG_IDXPIVOT_SHIFT = 4,
    SRTFLAG_PIVOT_MINUS    = 0x0100,    /* pivot element is -1.0 instead of +1.0 */
    SRTFLAG_SIGN_REVC      = 0x0200,      /* third stored element is -B */
    SRTFLAG_SIGN_REVD      = 0x0400,      /* fourth stored element is -A */
    RESULTFLAG_ROT_ZERO    = 0x0002,
    FX32_ONE               = 0x1000
};

/*
 * Reads the model's rest rotation for the current joint into pResult->rot.
 * The node index is the byte after the current SBC command; the node record
 * is looked up in the model's node dictionary. Its flag word says whether a
 * translation block precedes the rotation, whether the rotation is identity
 * (only a result flag is set), or whether it is stored as a pivot-compressed
 * 3x3 matrix (zeroed, then 1 pivot element = +/-1.0 and 4 elements built
 * from two fx16 values A, B via pivotUtil_). Otherwise the 9 fx16 elements
 * are widened to fx32 (the first lives in the node header).
 */
void getMdlRot_(JntAnmResult *pResult) {
    u32 idxPivot;
    const ResDict *info    = NNS_G3dRS->pResNodeInfo;
    const ResNodeData *pNd = (const ResNodeData *) ((const u8 *) info + *(const u32 *) getResDataByIdx(info, NNS_G3dRS->c[1]));
    const s16 *p           = (const s16 *) ((const u8 *) pNd + sizeof(ResNodeData));

    if (!(pNd->flag & SRTFLAG_TRANS_ZERO)) {
        p += 6;
    }

    if (!(pNd->flag & SRTFLAG_ROT_ZERO)) {
        if (pNd->flag & SRTFLAG_PIVOT_EXIST) {
            s32 A, B;
            A        = p[0];
            B        = p[1];
            idxPivot = (pNd->flag & SRTFLAG_IDXPIVOT_MASK) >> SRTFLAG_IDXPIVOT_SHIFT;
            func_020cf518(&pResult->rot); /* zero 36 bytes */
            pResult->rot.a[idxPivot]                = (pNd->flag & SRTFLAG_PIVOT_MINUS) ? -FX32_ONE : FX32_ONE;
            pResult->rot.a[pivotUtil_[idxPivot][0]] = A;
            pResult->rot.a[pivotUtil_[idxPivot][1]] = B;
            pResult->rot.a[pivotUtil_[idxPivot][2]] = (pNd->flag & SRTFLAG_SIGN_REVC) ? -B : B;
            pResult->rot.a[pivotUtil_[idxPivot][3]] = (pNd->flag & SRTFLAG_SIGN_REVD) ? -A : A;
        } else {
            pResult->rot.a[0] = pNd->_00;
            pResult->rot.a[1] = p[0];
            pResult->rot.a[2] = p[1];
            pResult->rot.a[3] = p[2];
            pResult->rot.a[4] = p[3];
            pResult->rot.a[5] = p[4];
            pResult->rot.a[6] = p[5];
            pResult->rot.a[7] = p[6];
            pResult->rot.a[8] = p[7];
        }
    } else {
        pResult->flag |= RESULTFLAG_ROT_ZERO;
    }
}
