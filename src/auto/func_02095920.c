#include "m2c.h"

typedef struct {
    s32 x, y, z;
} Vec3;
typedef struct {
    s32 m[4][4];
} Mtx44;
typedef struct Ent {
    Vec3 pos;
    u8 pad0c[0x14];
    u16 flags;
    u8 b22;
    u8 b23;
    u8 b24;
    u8 b25;
} Ent;
typedef struct {
    s32 m[12];
} Blk;

extern u8 *func_020aaea8(void);
s32 func_020ace3c(s32 arg0);
void func_01ffac98(Mtx44 *m, Vec3 *in, Vec3 *out);
void NNS_G3dGeBufferOP_N(s32 op, void *param, s32 count);

/* NDS geometry engine command numbers (GBATEK) and fixed parameter words. */
enum {
    GX_OP_MTX_LOAD_4x3   = 0x17,
    GX_OP_TEXCOORD       = 0x22,
    GX_OP_VTX_16         = 0x23,
    GX_OP_VTX_XY         = 0x25,
    GX_OP_POLYGON_ATTR   = 0x29,
    GX_OP_TEXIMAGE_PARAM = 0x2a,
    GX_OP_PLTT_BASE      = 0x2b,
    GX_OP_BEGIN_VTXS     = 0x40,
    GX_OP_END_VTXS       = 0x41,
    GX_BEGIN_QUADS       = 1,
    /* 64x64 256-colour texture, S/T repeat, colour 0 transparent, texcoord source */
    TEXIMAGE_BITS = 0x71b30000,
    /* polygon ID 0x3f and mode bits; alpha goes in bits 16-20 */
    POLYGON_ATTR_BITS = 0x3f0008e0,
    ALPHA_OPAQUE      = 0x1f,
    ENT_FLAG_ALPHA    = 8,
    /* quad corners as packed s16 (x | y << 16) vertex words */
    QUAD_VTX0_XY = 0xdf322,
    QUAD_VTX1_XY = 0xd0cde,
    QUAD_VTX2_XY = 0x19c90cde,
    QUAD_VTX3_XY = 0x19c9f322,
    /* tile sizes / offsets in fx32 texels */
    TILE_GRID3    = 3,
    TILE_SIZE_4x4 = 0x10000,
    TILE_SIZE_3x3 = 0x15000,
    TILE3_S2      = 0x2b000,
    TILE3_T2      = 0x2a000,
};

/* Packs two fx32 texel coordinates into a TEXCOORD word (s16 1/16-texel units). */
static inline u32 PACK_TC(s32 s, s32 t) {
    return (u32) (u16) (s16) (s >> 8) | ((u32) (u16) (s16) (t >> 8) << 16);
}

/*
 * Draws one textured quad (a tile of a 64x64 texture atlas) at e->pos.
 * mtx is a 4x3 matrix whose translation row is overwritten with e->pos
 * transformed by the matrix from func_020ace3c, then loaded as the current
 * matrix. tex_addr / pltt_addr are VRAM byte offsets (shifted to the
 * TEXIMAGE_PARAM / PLTT_BASE units). Alpha is e->b22 (clamped to 31) when
 * flag 8 is set, else opaque. e->b23 == 3 selects a 3x3 grid of 21-texel
 * tiles indexed by e->b24 (0..8); otherwise a 4x4 grid of 16-texel tiles.
 * self is unused.
 */
void func_02095920(u8 *self, Ent *e, Blk *mtx, u32 tex_addr, u32 pltt_addr) {
    u32 teximage;
    u32 pltt_base;
    u32 polyattr;
    u32 begin;
    u32 tc0;
    s32 vtx[2];
    u32 tc1;
    u32 xy1;
    u32 tc2;
    u32 xy2;
    u32 tc3;
    u32 xy3;
    s32 alpha;
    s32 tile_s;
    s32 tile_t;
    s32 tile_size;

    func_01ffac98((Mtx44 *) func_020ace3c((s32) (func_020aaea8() + 0x4c)), &e->pos, (Vec3 *) &mtx->m[9]);
    NNS_G3dGeBufferOP_N(GX_OP_MTX_LOAD_4x3, mtx, 12);
    teximage = TEXIMAGE_BITS | (tex_addr >> 3);
    NNS_G3dGeBufferOP_N(GX_OP_TEXIMAGE_PARAM, &teximage, 1);
    pltt_base = pltt_addr >> 4;
    NNS_G3dGeBufferOP_N(GX_OP_PLTT_BASE, &pltt_base, 1);
    alpha = ALPHA_OPAQUE;
    if (e->flags & ENT_FLAG_ALPHA) {
        if (e->b22 < ALPHA_OPAQUE) {
            alpha = e->b22;
        }
    }
    polyattr = POLYGON_ATTR_BITS | (alpha << 16);
    NNS_G3dGeBufferOP_N(GX_OP_POLYGON_ATTR, &polyattr, 1);
    tile_s    = 0;
    tile_t    = 0;
    tile_size = TILE_SIZE_4x4;
    if (e->b23 == TILE_GRID3) {
        tile_size = TILE_SIZE_3x3;
        switch (e->b24) {
            case 0:
                break;
            case 1:
                tile_s = tile_size;
                break;
            case 2:
                tile_s = TILE3_S2;
                break;
            case 3:
                tile_t = tile_size;
                break;
            case 4:
                tile_s = tile_size;
                tile_t = tile_size;
                break;
            case 5:
                tile_t = tile_size;
                tile_s = TILE3_S2;
                break;
            case 6:
                tile_t = TILE3_T2;
                break;
            case 7:
                tile_s = tile_size;
                tile_t = TILE3_T2;
                break;
            case 8:
                tile_s = TILE3_S2;
                tile_t = TILE3_T2;
                break;
        }
    } else {
        tile_s = (e->b24 & 3) << 16;
        tile_t = (e->b24 >> 2) << 16;
    }
    begin = GX_BEGIN_QUADS;
    NNS_G3dGeBufferOP_N(GX_OP_BEGIN_VTXS, &begin, 1);
    tc0 = PACK_TC(tile_s, tile_t + tile_size);
    NNS_G3dGeBufferOP_N(GX_OP_TEXCOORD, &tc0, 1);
    vtx[0] = QUAD_VTX0_XY;
    vtx[1] = 0;
    NNS_G3dGeBufferOP_N(GX_OP_VTX_16, vtx, 2);
    tc1 = PACK_TC(tile_s + tile_size, tile_t + tile_size);
    NNS_G3dGeBufferOP_N(GX_OP_TEXCOORD, &tc1, 1);
    xy1 = QUAD_VTX1_XY;
    NNS_G3dGeBufferOP_N(GX_OP_VTX_XY, &xy1, 1);
    tc2 = PACK_TC(tile_s + tile_size, tile_t);
    NNS_G3dGeBufferOP_N(GX_OP_TEXCOORD, &tc2, 1);
    xy2 = QUAD_VTX2_XY;
    NNS_G3dGeBufferOP_N(GX_OP_VTX_XY, &xy2, 1);
    tc3 = PACK_TC(tile_s, tile_t);
    NNS_G3dGeBufferOP_N(GX_OP_TEXCOORD, &tc3, 1);
    xy3 = QUAD_VTX3_XY;
    NNS_G3dGeBufferOP_N(GX_OP_VTX_XY, &xy3, 1);
    NNS_G3dGeBufferOP_N(GX_OP_END_VTXS, NULL, 0);
}
