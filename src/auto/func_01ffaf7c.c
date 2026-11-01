#include "m2c.h"

typedef struct {
    s32 x, y, z;
} Vec;

typedef struct {
    /* 0x00 */ u32 flag;
    /* 0x04 */ Vec scale;
    /* 0x10 */ Vec scaleEx0;
    /* 0x1C */ Vec scaleEx1;
    /* 0x28 */ s32 rot[9];
    /* 0x4C */ Vec trans;
} JntResult;

struct AnmObj;
typedef void (*AnmFunc)(JntResult *, struct AnmObj *, u32);

typedef struct AnmObj {
    /* 0x00 */ s32 frame;
    /* 0x04 */ s32 ratio;
    /* 0x08 */ void *resAnm;
    /* 0x0C */ AnmFunc funcAnm;
    /* 0x10 */ struct AnmObj *next;
    /* 0x14 */ void *resTex;
    /* 0x18 */ u8 priority;
    /* 0x19 */ u8 numMapData;
    /* 0x1A */ u16 mapData[1];
} AnmObj;

enum {
    MAP_STATE_MASK   = 0x300,     /* mapData state bits */
    MAP_STATE_ACTIVE = 0x100,   /* state value that selects the object for this joint */
    MAP_INDEX_MASK   = 0xFF,      /* per-object data index passed to funcAnm */
    FX_ONE           = 0x1000,            /* 1.0 in 20.12 fixed point */
    FX_SHIFT         = 12,
};

enum {
    RESULT_SCALE_ONE    = 0x1,     /* scale is (1,1,1): blend adds the weight */
    RESULT_ROT_IDENTITY = 0x2,  /* rot is identity: blend adds the weight to the diagonal */
    RESULT_TRANS_ZERO   = 0x4,    /* trans is zero: nothing to blend */
    RESULT_SCALEEX0_ONE = 0x8,
    RESULT_SCALEEX1_ONE = 0x10,
};

extern s32 FX_Div(s32, s32);
extern void VEC_CrossProduct(const Vec *a, const Vec *b, Vec *axb);
extern void VEC_Normalize(const Vec *src, Vec *dst);
extern void MIi_CpuClearFast(u32 data, void *destp, u32 size);

/*
 * Default joint-animation blend (entry of NNS_G3dFuncBlendJntDefault table).
 * With a single animation object, calls its funcAnm directly if the joint's
 * mapData entry is active. Otherwise sums the weights (ratio) of all active
 * objects in the chain; if only one is active it is evaluated directly.
 * With several, res is cleared (flag = all ones), each active object with a
 * positive weight is evaluated into a temporary, and its scale, scaleEx0,
 * scaleEx1, trans and the first two rotation rows are accumulated weighted
 * by ratio / sum (fixed point). The flags are ANDed together. Finally the
 * rotation is re-orthonormalised: row2 = row0 x row1, rows 0 and 2
 * normalised, row1 = row2 x row0. Returns 1 if a result was written.
 */
s32 func_01ffaf7c(JntResult *res, AnmObj *anm, u32 idx) {
    AnmObj *p;
    s32 sum;
    s32 num;
    AnmObj *last;

    if (anm->next == NULL) {
        u32 map = anm->mapData[idx];
        if ((map & MAP_STATE_MASK) != MAP_STATE_ACTIVE) {
            return 0;
        }
        anm->funcAnm(res, anm, map & MAP_INDEX_MASK);
        return 1;
    }

    sum = 0;
    num = 0;
    p   = anm;
    do {
        if ((p->mapData[idx] & MAP_STATE_MASK) == MAP_STATE_ACTIVE) {
            sum += p->ratio;
            num++;
            last = p;
        }
        p = p->next;
    } while (p);

    if (sum == 0) {
        return 0;
    }

    if (num == 1) {
        last->funcAnm(res, last, last->mapData[idx] & MAP_INDEX_MASK);
        return 1;
    } else {
        JntResult r;
        p = anm;
        MIi_CpuClearFast(0, res, sizeof(JntResult));
        res->flag = (u32) -1;
        do {
            u16 map = p->mapData[idx];
            if ((map & MAP_STATE_MASK) == MAP_STATE_ACTIVE && p->ratio > 0) {
                s32 ratio;
                p->funcAnm(&r, p, map & MAP_INDEX_MASK);
                if (sum == FX_ONE) {
                    ratio = p->ratio;
                } else {
                    ratio = FX_Div(p->ratio, sum);
                }

                if (r.flag & RESULT_SCALE_ONE) {
                    res->scale.x += ratio;
                    res->scale.y += ratio;
                    res->scale.z += ratio;
                } else {
                    res->scale.x += (ratio * r.scale.x) >> FX_SHIFT;
                    res->scale.y += (ratio * r.scale.y) >> FX_SHIFT;
                    res->scale.z += (ratio * r.scale.z) >> FX_SHIFT;
                }
                if (r.flag & RESULT_SCALEEX0_ONE) {
                    res->scaleEx0.x += ratio;
                    res->scaleEx0.y += ratio;
                    res->scaleEx0.z += ratio;
                } else {
                    res->scaleEx0.x += (ratio * r.scaleEx0.x) >> FX_SHIFT;
                    res->scaleEx0.y += (ratio * r.scaleEx0.y) >> FX_SHIFT;
                    res->scaleEx0.z += (ratio * r.scaleEx0.z) >> FX_SHIFT;
                }
                if (r.flag & RESULT_SCALEEX1_ONE) {
                    res->scaleEx1.x += ratio;
                    res->scaleEx1.y += ratio;
                    res->scaleEx1.z += ratio;
                } else {
                    res->scaleEx1.x += (ratio * r.scaleEx1.x) >> FX_SHIFT;
                    res->scaleEx1.y += (ratio * r.scaleEx1.y) >> FX_SHIFT;
                    res->scaleEx1.z += (ratio * r.scaleEx1.z) >> FX_SHIFT;
                }
                if (!(r.flag & RESULT_TRANS_ZERO)) {
                    res->trans.x += (s32) (((s64) ratio * r.trans.x) >> FX_SHIFT);
                    res->trans.y += (s32) (((s64) ratio * r.trans.y) >> FX_SHIFT);
                    res->trans.z += (s32) (((s64) ratio * r.trans.z) >> FX_SHIFT);
                }
                if (!(r.flag & RESULT_ROT_IDENTITY)) {
                    res->rot[0] += (ratio * r.rot[0]) >> FX_SHIFT;
                    res->rot[1] += (ratio * r.rot[1]) >> FX_SHIFT;
                    res->rot[2] += (ratio * r.rot[2]) >> FX_SHIFT;
                    res->rot[3] += (ratio * r.rot[3]) >> FX_SHIFT;
                    res->rot[4] += (ratio * r.rot[4]) >> FX_SHIFT;
                    res->rot[5] += (ratio * r.rot[5]) >> FX_SHIFT;
                } else {
                    res->rot[0] += ratio;
                    res->rot[4] += ratio;
                }
                res->flag &= r.flag;
            }
            p = p->next;
        } while (p);

        VEC_CrossProduct((Vec *) &res->rot[0], (Vec *) &res->rot[3], (Vec *) &res->rot[6]);
        VEC_Normalize((Vec *) &res->rot[0], (Vec *) &res->rot[0]);
        VEC_Normalize((Vec *) &res->rot[6], (Vec *) &res->rot[6]);
        VEC_CrossProduct((Vec *) &res->rot[6], (Vec *) &res->rot[0], (Vec *) &res->rot[3]);
        return 1;
    }
}
