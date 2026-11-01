#include "m2c.h"

typedef volatile u32 vu32;

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
    s32 v[12];
} Mtx43;

typedef struct {
    s32 v[16];
} Mtx44;

extern u32 data_02101398[28];
extern M2C_UNK NNS_G3dGlb;
extern M2C_UNK data_027e0cd0;
extern M2C_UNK data_027e0cf4;

void NNS_G3dGeBufferOP_N(s32 op, void *param, s32 count);
void NNS_G3dGetCurrentMtx(void *pos, void *nrm);
void *NNS_G3dGlbGetInvV(void);
s32 G3X_GetClipMtx(void *dest);
void NNS_G3dGeFlushBuffer(void);

enum {
    GE_OP_MTX_MODE     = 0x10,
    GE_OP_MTX_STORE    = 0x13,
    GE_OP_MTX_RESTORE  = 0x14,
    GE_OP_MTX_LOAD_4x4 = 0x16,
    GE_OP_MTX_MULT_4x4 = 0x18,
    GE_OP_MTX_MULT_4x3 = 0x19,
    GE_OP_MTX_MULT_3x3 = 0x1a,
    GE_OP_MTX_TRANS    = 0x1c,
    GE_OP_TEXCOORD     = 0x22,
};

enum {
    MTX_MODE_PROJECTION      = 0,
    MTX_MODE_POSITION_VECTOR = 2,
    MTX_MODE_TEXTURE         = 3,
    MTX_STACK_SAVE_SLOT      = 0x1e,
};

enum {
    REG_G3_MTX_MODE     = 0x4000440,
    REG_G3_MTX_PUSH     = 0x4000444,
    REG_G3_MTX_POP      = 0x4000448,
    REG_G3_MTX_IDENTITY = 0x4000454,
};

enum {
    TEXGEN_MASK   = 0xc0000000,
    TEXGEN_VERTEX = 0xc0000000,
};

enum {
    SBC_CMD_INDEX = 13,
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

/* TEXCOORD parameter word from two Q12 (fx32) values: each is shifted to the
 * command's signed 1.11.4 format and truncated to 16 bits, S low, T high. */
static inline u32 PackTexCoord(s32 s, s32 t) {
    return (u16) (s16) (s >> 8) | ((u32) (u16) (s16) (t >> 8) << 16);
}

/* Render-state command handler (pointer-table entry in data_02101498; record
 * length 3 bytes, record byte 1 = material index), the projection-mapping
 * counterpart of func_020c5738. For a material whose state flag has bit 0 set
 * and bit 0x200 clear, it saves the current position matrix (read back into
 * curMtx and stored to matrix stack slot 30), switches the texture coordinate
 * generation of the current material result to vertex-source mode
 * (re-sending the TexImageParam command if it changed), and then builds the
 * texture matrix in the position matrix in three stages:
 *   1. load the 4x4 matrix at data_02101398[12..27] with its scale and
 *      translation entries set from the texture's original width/height
 *      (w << 15, -h << 15, w << 15, h << 15);
 *   2. multiply by the material's optional effect matrix (flag 0x2000);
 *   3. multiply by the global terms selected by NNS_G3dGlb flag bits 0/1
 *      (bit 0: translation data_027e0cf4 and 3x3 data_027e0cd0; bit 1: none;
 *      neither: NNS_G3dGlbGetInvV()) and by curMtx, then read the product
 *      back as the clip matrix under an
 *      identity projection (as NNS_G3dGetCurrentMtx does), load it as the
 *      texture matrix and send its translation row (_30, _31) divided by 16
 *      as the texture coordinate.
 * Finally it returns to position-vector mode and restores the saved matrix.
 * The callback in slot 13 may run before each of the three stages. */
void func_020c5a4c(Sbc *rs) {
    Mtx43 curMtx;
    Mtx44 clipMtx;
    s32 timing;

    if (!(rs->flag & 0x200) && (rs->flag & 1)) {
        NNS_G3dGetCurrentMtx(&curMtx, NULL);
        {
            u32 arg = MTX_STACK_SAVE_SLOT;
            NNS_G3dGeBufferOP_N(GE_OP_MTX_STORE, &arg, 1);
        }
        {
            MatAnmResult *res = rs->pMatAnmResult;
            if ((res->prmTexImage & TEXGEN_MASK) != TEXGEN_VERTEX) {
                res->prmTexImage &= ~TEXGEN_MASK;
                rs->pMatAnmResult->prmTexImage |= TEXGEN_VERTEX;
                data_02101398[3] = rs->pMatAnmResult->prmTexImage;
                NNS_G3dGeBufferOP_N(data_02101398[2], &data_02101398[3], 1);
            }
        }
        timing = rs->cbVecFunc[SBC_CMD_INDEX] ? rs->cbVecTiming[SBC_CMD_INDEX] : 0;
        if (!CallBackCheck(rs, SBC_CMD_INDEX, &timing, 1)) {
            s32 w             = rs->pMatAnmResult->origWidth;
            s32 h             = rs->pMatAnmResult->origHeight;
            data_02101398[12] = w << 15;
            data_02101398[17] = -h << 15;
            data_02101398[24] = w << 15;
            data_02101398[25] = h << 15;
            NNS_G3dGeBufferOP_N(GE_OP_MTX_LOAD_4x4, &data_02101398[12], 0x10);
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
            if (M2C_FIELD(&NNS_G3dGlb, u32 *, 0xfc) & 1) {
                NNS_G3dGeBufferOP_N(GE_OP_MTX_TRANS, &data_027e0cf4, 3);
                NNS_G3dGeBufferOP_N(GE_OP_MTX_MULT_3x3, &data_027e0cd0, 9);
                NNS_G3dGeBufferOP_N(GE_OP_MTX_MULT_4x3, &curMtx, 12);
            } else if (M2C_FIELD(&NNS_G3dGlb, u32 *, 0xfc) & 2) {
                NNS_G3dGeBufferOP_N(GE_OP_MTX_MULT_4x3, &curMtx, 12);
            } else {
                NNS_G3dGeBufferOP_N(GE_OP_MTX_MULT_4x3, NNS_G3dGlbGetInvV(), 12);
                NNS_G3dGeBufferOP_N(GE_OP_MTX_MULT_4x3, &curMtx, 12);
            }
            /* Drain the command buffer, then talk to the geometry engine
             * directly: with the projection matrix pushed and set to
             * identity, the clip matrix equals the position matrix. */
            NNS_G3dGeFlushBuffer();
            *(vu32 *) REG_G3_MTX_MODE     = MTX_MODE_PROJECTION;
            *(vu32 *) REG_G3_MTX_PUSH     = 0;
            *(vu32 *) REG_G3_MTX_IDENTITY = 0;
            while (G3X_GetClipMtx(&clipMtx)) {
                /* busy until the geometry engine is idle */
            }
            *(vu32 *) REG_G3_MTX_POP  = 1;
            *(vu32 *) REG_G3_MTX_MODE = MTX_MODE_TEXTURE;
            NNS_G3dGeBufferOP_N(GE_OP_MTX_LOAD_4x4, &clipMtx, 0x10);
            {
                u32 arg = PackTexCoord(clipMtx.v[12] >> 4, clipMtx.v[13] >> 4);
                NNS_G3dGeBufferOP_N(GE_OP_TEXCOORD, &arg, 1);
            }
        }
        {
            u32 arg = MTX_MODE_POSITION_VECTOR;
            NNS_G3dGeBufferOP_N(GE_OP_MTX_MODE, &arg, 1);
        }
        {
            u32 arg = MTX_STACK_SAVE_SLOT;
            NNS_G3dGeBufferOP_N(GE_OP_MTX_RESTORE, &arg, 1);
        }
    }
    rs->c += 3;
}
