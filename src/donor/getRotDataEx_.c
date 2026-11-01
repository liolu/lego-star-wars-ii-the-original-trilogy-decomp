/* compiler: 2.0/sp1p5 -O4,p -lang c99 -ipa file -inline on,noauto */
/* func_0207c95c from projets/thrillville-off-the-rails/src/donor/func_0207c95c.c (same code in another game of this
 * workspace), reused by tools/donor_match.py */
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;
typedef int s32;
typedef short s16;
typedef s32 BOOL;
typedef long fx32;
typedef s16 fx16;
typedef struct {
    fx32 x;
    fx32 y;
    fx32 z;
} UnkVecFx32;
extern void VEC_Normalize(const UnkVecFx32 *src, UnkVecFx32 *dst);
typedef struct {
    fx32 m[3][3];
} UnkMtxFx33;
typedef struct {
    u32 info;
    u32 offset;
} UnkJntAnmRInfo;
typedef struct {
    u8 category0;
    u8 revision;
    u16 category1;
    u16 numFrame;
    u16 numNode;
    u32 flag;
    u32 ofsRot3;
    u32 ofsRot5;
} UnkResJntAnm;
extern BOOL getRotDataByIdx_(UnkMtxFx33 *rot, const fx16 *pivotData, const fx16 *rot5Data, u32 info);
static inline void CalcRow2_(UnkMtxFx33 *m) {
    fx32 x = (m->m[0][1] * m->m[1][2] - m->m[0][2] * m->m[1][1]) >> 12;
    fx32 y = (m->m[0][2] * m->m[1][0] - m->m[0][0] * m->m[1][2]) >> 12;
    fx32 z = (m->m[0][0] * m->m[1][1] - m->m[0][1] * m->m[1][0]) >> 12;

    m->m[2][0] = x;
    m->m[2][1] = y;
    m->m[2][2] = z;
}
void getRotDataEx_(UnkMtxFx33 *rot, fx32 frame, const UnkJntAnmRInfo *rinfo, const UnkResJntAnm *anm) {
    u32 ofsRot5;
    u32 frameIdx;
    u32 ofsRot3;
    u32 idx1;
    u32 info;
    fx32 t;
    fx32 w;
    const u16 *data;
    BOOL notDone;
    UnkMtxFx33 tmp0;
    UnkMtxFx33 tmp1;

    data     = (const u16 *) ((const u8 *) anm + rinfo->offset);
    frameIdx = frame >> 12;
    ofsRot3  = anm->ofsRot3;
    ofsRot5  = anm->ofsRot5;
    info     = rinfo->info;

    if (anm->numFrame - 1 == frameIdx) {
        if (info & 0xc0000000) {
            if (info & 0x40000000) {
                frameIdx = (frameIdx & 1) + (frameIdx >> 1);
            } else {
                frameIdx = (frameIdx & 3) + (frameIdx >> 2);
            }
        }
        if (anm->flag & 2) {
            idx1 = 0;
            goto step1;
        }
        if (getRotDataByIdx_(rot, (const fx16 *) ((const u8 *) anm + ofsRot3), (const fx16 *) ((const u8 *) anm + ofsRot5),
                             data[frameIdx])) {
            CalcRow2_(rot);
        }
        return;
    }

    if (info & 0xc0000000) {
        u32 last = (info & 0x1fff0000) >> 16;
        if (info & 0x40000000) {
            if (frameIdx >= last) {
                frameIdx = last >> 1;
                idx1     = frameIdx + 1;
                goto step1;
            }
            frameIdx >>= 1;
            idx1 = frameIdx + 1;
            t    = frame & 0x1fff;
            w    = 2;
        } else {
            if (frameIdx >= last) {
                frameIdx = (frameIdx & 3) + (frameIdx >> 2);
                idx1     = frameIdx + 1;
                goto step1;
            }
            frameIdx >>= 2;
            idx1 = frameIdx + 1;
            t    = frame & 0x3fff;
            w    = 4;
        }
    } else {
        idx1 = frameIdx + 1;
    step1:
        t = frame & 0xfff;
        w = 1;
    }

    notDone = 0;
    notDone |= getRotDataByIdx_(&tmp0, (const fx16 *) ((const u8 *) anm + ofsRot3),
                                (const fx16 *) ((const u8 *) anm + ofsRot5), data[frameIdx]);
    notDone |= getRotDataByIdx_(&tmp1, (const fx16 *) ((const u8 *) anm + ofsRot3),
                                (const fx16 *) ((const u8 *) anm + ofsRot5), data[idx1]);

    rot->m[0][0] = tmp0.m[0][0] * w + (((tmp1.m[0][0] - tmp0.m[0][0]) * t) >> 12);
    rot->m[0][1] = tmp0.m[0][1] * w + (((tmp1.m[0][1] - tmp0.m[0][1]) * t) >> 12);
    rot->m[0][2] = tmp0.m[0][2] * w + (((tmp1.m[0][2] - tmp0.m[0][2]) * t) >> 12);
    rot->m[1][0] = tmp0.m[1][0] * w + (((tmp1.m[1][0] - tmp0.m[1][0]) * t) >> 12);
    rot->m[1][1] = tmp0.m[1][1] * w + (((tmp1.m[1][1] - tmp0.m[1][1]) * t) >> 12);
    rot->m[1][2] = tmp0.m[1][2] * w + (((tmp1.m[1][2] - tmp0.m[1][2]) * t) >> 12);
    VEC_Normalize((UnkVecFx32 *) &rot->m[0][0], (UnkVecFx32 *) &rot->m[0][0]);
    VEC_Normalize((UnkVecFx32 *) &rot->m[1][0], (UnkVecFx32 *) &rot->m[1][0]);

    if (!notDone) {
        rot->m[2][0] = tmp0.m[2][0] * w + (((tmp1.m[2][0] - tmp0.m[2][0]) * t) >> 12);
        rot->m[2][1] = tmp0.m[2][1] * w + (((tmp1.m[2][1] - tmp0.m[2][1]) * t) >> 12);
        rot->m[2][2] = tmp0.m[2][2] * w + (((tmp1.m[2][2] - tmp0.m[2][2]) * t) >> 12);
        VEC_Normalize((UnkVecFx32 *) &rot->m[2][0], (UnkVecFx32 *) &rot->m[2][0]);
    } else {
        CalcRow2_(rot);
    }
}
extern BOOL getRotDataByIdx_(UnkMtxFx33 *rot, const fx16 *pivotData, const fx16 *rot5Data, u32 info);
