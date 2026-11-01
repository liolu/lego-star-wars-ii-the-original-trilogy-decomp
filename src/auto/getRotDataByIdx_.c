#include "m2c.h"

typedef s32 fx32;
typedef s16 fx16;
typedef s32 BOOL;

typedef union MtxFx33 {
    struct {
        fx32 _00, _01, _02;
        fx32 _10, _11, _12;
        fx32 _20, _21, _22;
    };
    fx32 m[3][3];
    fx32 a[9];
} MtxFx33;

/* Per pivot position: the four matrix slots that receive the two stored
 * values and their (optionally negated) copies. */
extern const u8 pivotUtil_[][4];

enum {
    ROT_INFO_PIVOT      = 0x8000,      /* record lives in the 3-halfword pivot table */
    ROT_INFO_INDEX_MASK = 0x7fff, /* record index within its table */
    PIVOT_POS_MASK      = 0xf,         /* matrix slot holding +/-1.0 */
    PIVOT_NEG           = 0x10,             /* pivot slot is -1.0 */
    PIVOT_NEG_B         = 0x20,           /* third derived slot holds -B */
    PIVOT_NEG_A         = 0x40,           /* fourth derived slot holds -A */
    FX32_ONE            = 0x1000,            /* 1.0 in 20.12 fixed point */
    PACKED_LOW_BITS     = 7,          /* low 3 bits of each packed halfword */
};

/*
 * Decode joint-animation rotation record `info` into the 3x3 matrix pRot.
 *
 * Pivot form (info & ROT_INFO_PIVOT): a 3-halfword record {flags, A, B}.
 * The matrix is cleared, the pivot slot gets +/-1.0, and A, B, +/-B, +/-A
 * go to the four slots listed in pivotUtil_[pivot]. Returns 0.
 *
 * Packed form: a 5-halfword record. Each halfword's upper bits (>> 3) give
 * _11, _00, _01, _02, _10; their low 3 bits, concatenated in that order,
 * form the 13-bit signed value stored in _12. The third row is not written.
 * Returns 1.
 */
BOOL getRotDataByIdx_(MtxFx33 *pRot, const void *pivotRecords, const void *packedRecords, u16 info) {
    if (info & ROT_INFO_PIVOT) {
        const fx16 *data;
        u32 idxPivot;
        fx32 A, B;

        pRot->_22 = 0;
        pRot->_21 = 0;
        pRot->_20 = 0;
        pRot->_12 = 0;
        pRot->_11 = 0;
        pRot->_10 = 0;
        pRot->_02 = 0;
        pRot->_01 = 0;
        pRot->_00 = 0;

        data                             = (const fx16 *) pivotRecords + (info & ROT_INFO_INDEX_MASK) * 3;
        A                                = data[1];
        B                                = data[2];
        idxPivot                         = (u32) (data[0] & PIVOT_POS_MASK);
        pRot->a[idxPivot]                = (data[0] & PIVOT_NEG) ? -FX32_ONE : FX32_ONE;
        pRot->a[pivotUtil_[idxPivot][0]] = A;
        pRot->a[pivotUtil_[idxPivot][1]] = B;
        pRot->a[pivotUtil_[idxPivot][2]] = (data[0] & PIVOT_NEG_B) ? -B : B;
        pRot->a[pivotUtil_[idxPivot][3]] = (data[0] & PIVOT_NEG_A) ? -A : A;
        return 0;
    } else {
        const fx16 *data = (const fx16 *) packedRecords + (info & ROT_INFO_INDEX_MASK) * 5;
        fx16 _12;
        fx16 v;

        v         = data[4];
        pRot->_11 = v >> 3;
        _12       = (fx16) (v & PACKED_LOW_BITS);
        v         = data[0];
        pRot->_00 = v >> 3;
        _12       = (fx16) ((_12 << 3) | (v & PACKED_LOW_BITS));
        v         = data[1];
        pRot->_01 = v >> 3;
        _12       = (fx16) ((_12 << 3) | (v & PACKED_LOW_BITS));
        v         = data[2];
        pRot->_02 = v >> 3;
        _12       = (fx16) ((_12 << 3) | (v & PACKED_LOW_BITS));
        v         = data[3];
        pRot->_10 = v >> 3;
        _12       = (fx16) ((_12 << 3) | (v & PACKED_LOW_BITS));
        pRot->_12 = ((s32) _12 << 19) >> 19;
        return 1;
    }
}
