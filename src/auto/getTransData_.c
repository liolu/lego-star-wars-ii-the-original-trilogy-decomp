#include "m2c.h"

typedef s32 fx32;
typedef s16 fx16;
typedef s64 fx64;

/* Layout of the per-channel translation info word (pData[0]). */
enum {
    FX32_SHIFT              = 12,                     /* fx32 fractional bits */
    TINFO_STEP_MASK         = 0xc0000000,        /* nonzero: keys are sampled every 2 or 4 frames */
    TINFO_STEP_2            = 0x40000000,           /* set: step 2; clear (with step mask): step 4 */
    TINFO_FX16ARRAY         = 0x20000000,        /* key array holds fx16 values, else fx32 */
    TINFO_LAST_INTERP_MASK  = 0x1fff0000, /* last frame still interpolated between keys */
    TINFO_LAST_INTERP_SHIFT = 16,
};

/*
 * Fetch the joint translation value for `frame` (fx32 frame number) into *pVal.
 * pData[0] is the info word, pData[1] the byte offset of the key array from
 * pAnmSRTInfo. Key index = whole frame / step. Frames between keys are linearly
 * interpolated (1/2 for midpoints, 1/4 and 3/4 for step-4 odd frames); frames
 * past the last interpolated frame read the stored key directly.
 */

void getTransData_(fx32 *pVal, fx32 frame, const u32 *pData, const void *pAnmSRTInfo) {
    u32 last_interp;
    u32 idx;
    u32 idx_sub;
    u32 frame_         = (u32) (frame >> FX32_SHIFT);
    const void *pArray = (const void *) ((const u8 *) pAnmSRTInfo + pData[1]);
    u32 info           = pData[0];

    if (!(info & TINFO_STEP_MASK)) {
        idx = frame_;
        goto TRANS_NONINTERP;
    }

    last_interp = (info & TINFO_LAST_INTERP_MASK) >> TINFO_LAST_INTERP_SHIFT;

    if (info & TINFO_STEP_2) {
        if (frame_ & 1) {
            if (frame_ > last_interp) {
                idx = (last_interp >> 1) + 1;
                goto TRANS_NONINTERP;
            } else {
                idx = frame_ >> 1;
                goto TRANS_INTERP_2;
            }
        } else {
            idx = frame_ >> 1;
            goto TRANS_NONINTERP;
        }
    } else {
        if (frame_ & 3) {
            if (frame_ > last_interp) {
                idx = (last_interp >> 2) + (frame_ & 3);
                goto TRANS_NONINTERP;
            }

            if (frame_ & 1) {
                fx32 v, v_sub;
                if (frame_ & 2) {
                    idx_sub = frame_ >> 2;
                    idx     = idx_sub + 1;
                } else {
                    idx     = frame_ >> 2;
                    idx_sub = idx + 1;
                }

                if (info & TINFO_FX16ARRAY) {
                    const fx16 *p_fx16 = (const fx16 *) pArray;
                    v                  = p_fx16[idx];
                    v_sub              = p_fx16[idx_sub];
                    *pVal              = (v + v + v + v_sub) >> 2;
                } else {
                    const fx32 *p_fx32 = (const fx32 *) pArray;
                    v                  = p_fx32[idx];
                    v_sub              = p_fx32[idx_sub];
                    *pVal              = (fx32) (((fx64) v + v + v + v_sub) >> 2);
                }
                return;
            } else {
                idx = frame_ >> 2;
                goto TRANS_INTERP_2;
            }
        } else {
            idx = frame_ >> 2;
            goto TRANS_NONINTERP;
        }
    }

TRANS_INTERP_2:
    if (info & TINFO_FX16ARRAY) {
        const fx16 *p_fx16 = (const fx16 *) pArray;
        *pVal              = (p_fx16[idx] + p_fx16[idx + 1]) >> 1;
    } else {
        const fx32 *p_fx32 = (const fx32 *) pArray;
        *pVal              = (p_fx32[idx] >> 1) + (p_fx32[idx + 1] >> 1);
    }
    return;

TRANS_NONINTERP:
    if (info & TINFO_FX16ARRAY) {
        const fx16 *p_fx16 = (const fx16 *) pArray;
        *pVal              = p_fx16[idx];
    } else {
        const fx32 *p_fx32 = (const fx32 *) pArray;
        *pVal              = p_fx32[idx];
    }
    return;
}
