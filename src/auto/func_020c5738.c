#include "m2c.h"

typedef struct {
    u8 pad0[0x10];
    u32 prmTexImage;
    u8 pad14[0x18];
    u16 origWidth;
    u16 origHeight;
} MatAnmResult;

typedef struct {
    u16 sizeUnit;
    u16 ofsName;
    u32 offset;
} DictEntry;

typedef struct {
    u8 pad0[4];
    u8 revision;
    u8 numEntry;
    u16 ofsEntry;
} ResDict;

typedef struct {
    u8 pad0[4];
    ResDict dict;
} ResMat;

typedef struct {
    u8 pad0[0x1e];
    u16 flag;
    u8 pad20[0xc];
    u32 effectMtx[1];
} ResMatData;

typedef struct Sbc Sbc;
struct Sbc {
    u8 *c;
    u32 unk4;
    u32 flag;
    void (*cbVecFunc[16])(Sbc *);
    u8 pad4c[0x40];
    u8 cbVecTiming[16];
    u8 pad9c[0x14];
    MatAnmResult *pMatAnmResult;
    u8 padb4[0x24];
    ResMat *pResMat;
};

typedef struct {
    s32 v[9];
} Mtx33;

extern u32 data_02101398[2];
extern M2C_UNK NNS_G3dGlb;
extern M2C_UNK data_027e0c60;
extern M2C_UNK data_027e0cd0;

void NNS_G3dGeBufferOP_N(s32 op, void *param, s32 count);
void NNS_G3dGetCurrentMtx(void *pos, Mtx33 *nrm);

enum {
    GE_OP_MTX_MODE     = 0x10,
    GE_OP_MTX_MULT_4x4 = 0x18,
    GE_OP_MTX_MULT_3x3 = 0x1a,
    GE_OP_MTX_SCALE    = 0x1b,
    GE_OP_TEXCOORD     = 0x22,
};

enum {
    MTX_MODE_POSITION_VECTOR = 2,
    MTX_MODE_TEXTURE         = 3,
};

enum {
    TEXGEN_MASK   = 0xc0000000,
    TEXGEN_NORMAL = 0x80000000,
};

enum {
    SBC_CMD_INDEX = 12,
    RS_FLAG_SKIP  = 0x40,
};

/* Runs the per-command callback when its timing slot equals `when`; the
 * callback may set the skip flag to suppress the following stage. */
static inline s32 CallBackCheck(Sbc *rs, s32 cmd, s32 *pTiming, s32 when) {
    if (*pTiming == when) {
        rs->flag &= ~RS_FLAG_SKIP;
        rs->cbVecFunc[cmd](rs);
        *pTiming = rs->cbVecFunc[cmd] ? rs->cbVecTiming[cmd] : 0;
        return rs->flag & RS_FLAG_SKIP;
    }
    return 0;
}

/* Render-state command handler (pointer-table entry in data_02101498; record
 * length 3 bytes, record byte 1 = material index). For a material whose
 * state flag has bit 0 set and bit 0x200 clear, it switches the texture
 * coordinate generation of the current material result to normal-vector
 * mode (re-sending the TexImageParam command if it changed), then loads a
 * texture matrix: fx32 scale (width << 15, -height << 15, 16.0), texcoord
 * (width << 3, height << 3) in 1/16-texel units, the material's optional
 * effect matrix (flag 0x2000), and the
 * normal (3x3) part of the current position matrix, optionally premultiplied
 * by the two global 3x3 matrices selected by NNS_G3dGlb flag bits 0/1. The
 * callback in slot 12 may run before each of the three stages. */
void func_020c5738(Sbc *rs) {
    Mtx33 m;
    s32 timing;

    if (!(rs->flag & 0x200) && (rs->flag & 1)) {
        MatAnmResult *res = rs->pMatAnmResult;
        if ((res->prmTexImage & TEXGEN_MASK) != TEXGEN_NORMAL) {
            res->prmTexImage &= ~TEXGEN_MASK;
            rs->pMatAnmResult->prmTexImage |= TEXGEN_NORMAL;
            data_02101398[1] = rs->pMatAnmResult->prmTexImage;
            NNS_G3dGeBufferOP_N(data_02101398[0], &data_02101398[1], 1);
        }
        {
            u32 arg = MTX_MODE_TEXTURE;
            NNS_G3dGeBufferOP_N(GE_OP_MTX_MODE, &arg, 1);
        }
        timing = rs->cbVecFunc[SBC_CMD_INDEX] ? rs->cbVecTiming[SBC_CMD_INDEX] : 0;
        if (!CallBackCheck(rs, SBC_CMD_INDEX, &timing, 1)) {
            s32 w = rs->pMatAnmResult->origWidth;
            s32 h = rs->pMatAnmResult->origHeight;
            {
                s32 scale[3];
                scale[0] = w << 15;
                scale[1] = -h << 15;
                scale[2] = 0x10000;
                NNS_G3dGeBufferOP_N(GE_OP_MTX_SCALE, scale, 3);
            }
            {
                u32 arg = (u16) (s16) (w << 3) | ((u32) (u16) (s16) (h << 3) << 16);
                NNS_G3dGeBufferOP_N(GE_OP_TEXCOORD, &arg, 1);
            }
        }
        if (!CallBackCheck(rs, SBC_CMD_INDEX, &timing, 2)) {
            ResMat *mat      = rs->pResMat;
            DictEntry *e     = (DictEntry *) ((u8 *) &mat->dict + mat->dict.ofsEntry);
            ResMatData *data = (ResMatData *) ((u8 *) mat + ((DictEntry *) ((u8 *) e + e->sizeUnit * rs->c[1]))->offset);
            if (data->flag & 0x2000) {
                u8 *p = (u8 *) data->effectMtx;
                if (!(data->flag & 2)) {
                    p += 8;
                }
                if (!(data->flag & 4)) {
                    p += 4;
                }
                if (!(data->flag & 8)) {
                    p += 8;
                }
                NNS_G3dGeBufferOP_N(GE_OP_MTX_MULT_4x4, p, 0x10);
            }
        }
        if (!CallBackCheck(rs, SBC_CMD_INDEX, &timing, 3)) {
            {
                u32 arg = MTX_MODE_POSITION_VECTOR;
                NNS_G3dGeBufferOP_N(GE_OP_MTX_MODE, &arg, 1);
            }
            NNS_G3dGetCurrentMtx(NULL, &m);
            {
                u32 arg = MTX_MODE_TEXTURE;
                NNS_G3dGeBufferOP_N(GE_OP_MTX_MODE, &arg, 1);
            }
            if (M2C_FIELD(&NNS_G3dGlb, u32 *, 0xfc) & 1) {
                NNS_G3dGeBufferOP_N(GE_OP_MTX_MULT_3x3, &data_027e0c60, 9);
                NNS_G3dGeBufferOP_N(GE_OP_MTX_MULT_3x3, &data_027e0cd0, 9);
                NNS_G3dGeBufferOP_N(GE_OP_MTX_MULT_3x3, &m, 9);
            } else if (M2C_FIELD(&NNS_G3dGlb, u32 *, 0xfc) & 2) {
                NNS_G3dGeBufferOP_N(GE_OP_MTX_MULT_3x3, &data_027e0c60, 9);
                NNS_G3dGeBufferOP_N(GE_OP_MTX_MULT_3x3, &m, 9);
            } else {
                NNS_G3dGeBufferOP_N(GE_OP_MTX_MULT_3x3, &m, 9);
            }
        }
        {
            u32 arg = MTX_MODE_POSITION_VECTOR;
            NNS_G3dGeBufferOP_N(GE_OP_MTX_MODE, &arg, 1);
        }
    }
    rs->c += 3;
}
