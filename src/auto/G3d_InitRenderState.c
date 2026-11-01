#include "m2c.h"

typedef struct ResMdl {
    u32 size;           /* 0x00 */
    u32 ofsSbc;         /* 0x04 */
    u32 ofsMat;         /* 0x08 */
    u32 ofsShp;         /* 0x0c */
    u32 ofsEvpMtx;      /* 0x10 */
    u8 sbcBufSize;      /* 0x14 */
    u8 scalingRule;     /* 0x15 */
    u8 texMtxMode;      /* 0x16 */
    u8 numNode;         /* 0x17 */
    u8 numMat;          /* 0x18 */
    u8 numShp;          /* 0x19 */
    u8 firstUnusedMtxStackID; /* 0x1a */
    u8 dummy_;          /* 0x1b */
    s32 posScale;       /* 0x1c */
    s32 invPosScale;    /* 0x20 */
    u8 pad[0x1c];       /* 0x24 */
    u8 nodeInfo[1];     /* 0x40 */
} ResMdl;

typedef struct RS RS;
typedef void (*RSCallback)(RS *rs);

typedef struct RenderObj {
    u32 flag;           /* 0x00 */
    ResMdl *resMdl;     /* 0x04 */
    u8 pad08[0x18];     /* 0x08 */
    RSCallback cbFunc;  /* 0x20 */
    u8 cbCmd;           /* 0x24 */
    u8 cbTiming;        /* 0x25 */
    u8 pad26[2];
    RSCallback cbInitFunc; /* 0x28 */
    void *ptrUser;      /* 0x2c */
    u8 *ptrUserSbc;     /* 0x30 */
} RenderObj;

struct RS {
    u8 *c;                  /* 0x00 */
    RenderObj *pRenderObj;  /* 0x04 */
    u32 flag;               /* 0x08 */
    RSCallback cbVecFunc[32]; /* 0x0c */
    u8 cbVecTiming[32];     /* 0x8c */
    u8 pad_ac[0x18];        /* 0xac */
    u32 unk_c4;             /* 0xc4 */
    u8 pad_c8[0xc];         /* 0xc8 */
    void *pResNodeInfo;     /* 0xd4 */
    void *pResMat;          /* 0xd8 */
    void *pResShp;          /* 0xdc */
    s32 posScale;           /* 0xe0 */
    s32 invPosScale;        /* 0xe4 */
    void *funcJntScale;     /* 0xe8 */
    void *funcJntMtx;       /* 0xec */
    void *funcTexMtx;       /* 0xf0 */
    u8 pad_f4[0x188 - 0xf4];
};

extern void MIi_CpuClearFast(u32 data, void *destp, u32 size);
extern void G3d_RenderSBCCommands(RS *rs);
extern void *G3d_gScaleHandlers[];
extern void *G3d_gSRTTransformHandlers[];
extern void *G3d_gTextureHandlers[];

enum {
    SBC_CALLBACK_SLOTS = 0x20, /* entries in RS::cbVecFunc (0x0c..0x8c) */
    RS_FLAG_INIT       = 0x1,        /* initial RS::flag value */
    OBJ_OPT_0          = 0x1,           /* RenderObj::flag option bits, copied to RS */
    OBJ_OPT_1          = 0x2,
    OBJ_OPT_2          = 0x4,
    OBJ_OPT_3          = 0x8,
    RS_OPT_0           = 0x80,           /* RS::flag bits mirroring OBJ_OPT_0..3 */
    RS_OPT_1           = 0x100,
    RS_OPT_2           = 0x200,
    RS_OPT_3           = 0x400,
};

/*
 * Prepares a render state for drawing one render object, then runs its
 * SBC (structure byte code) program.
 * 1. Zero the whole 0x188-byte state; set flag and the word at 0xc4 to 1.
 * 2. Command stream: the object's user SBC if set, else the model's own SBC
 *    (model base + ofsSbc).
 * 3. Cache model sections (node info at +0x40, materials, shapes), the
 *    per-model handler tables indexed by the model's scaling rule and
 *    texture-matrix mode, and the position scale pair.
 * 4. Install the object's single command callback if its command index is
 *    within the callback table.
 * 5. Mirror the object's four option bits into RS::flag, run the object's
 *    init callback, execute the SBC, then clear the object's option bit 0.
 */
void G3d_InitRenderState(RS *rs, RenderObj *obj) {
    MIi_CpuClearFast(0, rs, sizeof(RS));
    rs->unk_c4 = 1;
    rs->flag   = RS_FLAG_INIT;
    if (obj->ptrUserSbc) {
        rs->c = obj->ptrUserSbc;
    } else {
        rs->c = (u8 *) obj->resMdl + obj->resMdl->ofsSbc;
    }
    rs->pRenderObj   = obj;
    rs->pResNodeInfo = obj->resMdl->nodeInfo;
    rs->pResMat      = (u8 *) obj->resMdl + obj->resMdl->ofsMat;
    rs->pResShp      = (u8 *) obj->resMdl + obj->resMdl->ofsShp;
    rs->funcJntScale = G3d_gScaleHandlers[obj->resMdl->scalingRule];
    rs->funcJntMtx   = G3d_gSRTTransformHandlers[obj->resMdl->scalingRule];
    rs->funcTexMtx   = G3d_gTextureHandlers[obj->resMdl->texMtxMode];
    rs->posScale     = obj->resMdl->posScale;
    rs->invPosScale  = obj->resMdl->invPosScale;

    if (obj->cbFunc && obj->cbCmd < SBC_CALLBACK_SLOTS) {
        rs->cbVecFunc[obj->cbCmd]   = obj->cbFunc;
        rs->cbVecTiming[obj->cbCmd] = obj->cbTiming;
    }

    if (obj->flag & OBJ_OPT_0) {
        rs->flag |= RS_OPT_0;
    }
    if (obj->flag & OBJ_OPT_1) {
        rs->flag |= RS_OPT_1;
    }
    if (obj->flag & OBJ_OPT_2) {
        rs->flag |= RS_OPT_2;
    }
    if (obj->flag & OBJ_OPT_3) {
        rs->flag |= RS_OPT_3;
    }

    if (obj->cbInitFunc) {
        obj->cbInitFunc(rs);
    }

    G3d_RenderSBCCommands(rs);
    obj->flag &= ~OBJ_OPT_0;
}
