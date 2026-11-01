#include "m2c.h"

/* Geometry-engine command ids (DS hardware) and the parameters used here. */
enum {
    GE_OP_MTX_MODE     = 0x10,
    GE_OP_MTX_IDENTITY = 0x15,
    GE_OP_TEXCOORD     = 0x22,
    GE_OP_VTX_16       = 0x23,
    GE_OP_VTX_XY       = 0x25,
    GE_OP_POLYGON_ATTR = 0x29,
    GE_OP_PLTT_BASE    = 0x2b,
    GE_OP_DIF_AMB      = 0x30,
    GE_OP_SPE_EMI      = 0x31,
    GE_OP_BEGIN_VTXS   = 0x40,
    GE_OP_END_VTXS     = 0x41,
};

enum {
    MTX_MODE_POSITION_VECTOR = 2,
    MTX_MODE_TEXTURE         = 3,
    BEGIN_QUADS              = 1,
    TEXFMT_PLTT4             = 2,      /* 4-colour palette: palette base uses 8-byte units */
    DIF_AMB_WHITE            = 0x7fffffff,
    SPE_EMI_WHITE            = 0xffff,
    POLY_ATTR_BASE           = 0x80c0, /* both faces shown, fog bit; alpha goes in bits 16..20 */
    /* Packed fx16 vertex pairs (x low, y high): -0.5 = 0xf800, +0.5 = 0x0800. */
    VTX_XY_MINUS_MINUS = 0xf800f800,
    VTX_XY_PLUS_MINUS  = 0xf8000800,
    VTX_XY_PLUS_PLUS   = 0x08000800,
    VTX_XY_MINUS_PLUS  = 0x0800f800,
};

typedef struct {
    u8 pad0[0xc];
    s32 fmt; /* 0x0C: texture format */
} TexInfo;

typedef struct {
    u8 pad0[0x8];
    u32 key; /* 0x08: palette key, low 16 bits = address / 8 */
} PlttInfo;

typedef struct {
    u8 pad0[0xd4];
    TexInfo *tex;   /* 0xD4: texture sent with func_0205a148 */
    PlttInfo *pltt; /* 0xD8: optional palette */
    s32 s0;         /* 0xDC: texcoords, fx32 */
    s32 t0;         /* 0xE0 */
    s32 s1;         /* 0xE4 */
    s32 t1;         /* 0xE8 */
    u8 padEC[0xf4 - 0xec];
    u8 alpha;       /* 0xF4: polygon alpha */
    u8 flip;        /* 0xF5: nonzero = t0 at the -0.5 y edge, else t1 */
} TexQuad;

extern void func_0205a148(TexInfo *tex);
extern void NNS_G3dGeBufferOP_N(s32 op, void *param, s32 count);

static inline void GeCmd1(s32 op, u32 v) {
    u32 x = v;
    NNS_G3dGeBufferOP_N(op, &x, 1);
}

/* fx32 texcoords to the packed fx16 (s low, t high) TEXCOORD parameter. */
static inline void GeTexCoord(s32 s, s32 t) {
    u32 x = (u16) (s16) (s >> 8) | ((u32) (u16) (s16) (t >> 8) << 16);
    NNS_G3dGeBufferOP_N(GE_OP_TEXCOORD, &x, 1);
}

static inline void GeVtx(u32 xy, u32 z) {
    u32 d[2];
    d[0] = xy;
    d[1] = z;
    NNS_G3dGeBufferOP_N(GE_OP_VTX_16, d, 2);
}

/* Draws a textured unit quad (x, y in -0.5..+0.5, z 0) with an identity texture matrix,
 * white material, the quad's alpha and its four texcoords; flip chooses the t order. */
void func_02019f74(TexQuad *quad) {
    if (quad->tex != NULL) {
        func_0205a148(quad->tex);
    }
    if (quad->pltt != NULL) {
        GeCmd1(GE_OP_PLTT_BASE, ((quad->pltt->key & 0xffff) << 3) >> (4 - (quad->tex->fmt == TEXFMT_PLTT4 ? 1 : 0)));
    }
    GeCmd1(GE_OP_MTX_MODE, MTX_MODE_TEXTURE);
    NNS_G3dGeBufferOP_N(GE_OP_MTX_IDENTITY, NULL, 0);
    GeCmd1(GE_OP_MTX_MODE, MTX_MODE_POSITION_VECTOR);
    GeCmd1(GE_OP_DIF_AMB, DIF_AMB_WHITE);
    GeCmd1(GE_OP_SPE_EMI, SPE_EMI_WHITE);
    GeCmd1(GE_OP_POLYGON_ATTR, POLY_ATTR_BASE | ((u32) quad->alpha << 16));
    GeCmd1(GE_OP_BEGIN_VTXS, BEGIN_QUADS);
    if (quad->flip) {
        GeTexCoord(quad->s0, quad->t0);
        GeVtx(VTX_XY_MINUS_MINUS, 0);
        GeTexCoord(quad->s1, quad->t0);
        GeCmd1(GE_OP_VTX_XY, VTX_XY_PLUS_MINUS);
        GeTexCoord(quad->s1, quad->t1);
        GeCmd1(GE_OP_VTX_XY, VTX_XY_PLUS_PLUS);
        GeTexCoord(quad->s0, quad->t1);
        GeCmd1(GE_OP_VTX_XY, VTX_XY_MINUS_PLUS);
    } else {
        GeTexCoord(quad->s0, quad->t1);
        GeVtx(VTX_XY_MINUS_MINUS, 0);
        GeTexCoord(quad->s1, quad->t1);
        GeCmd1(GE_OP_VTX_XY, VTX_XY_PLUS_MINUS);
        GeTexCoord(quad->s1, quad->t0);
        GeCmd1(GE_OP_VTX_XY, VTX_XY_PLUS_PLUS);
        GeTexCoord(quad->s0, quad->t0);
        GeCmd1(GE_OP_VTX_XY, VTX_XY_MINUS_PLUS);
    }
    NNS_G3dGeBufferOP_N(GE_OP_END_VTXS, NULL, 0);
}
