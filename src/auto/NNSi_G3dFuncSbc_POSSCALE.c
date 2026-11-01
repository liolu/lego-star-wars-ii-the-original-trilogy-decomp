#include "m2c.h"

typedef struct {
    s32 x, y, z;
} VecFx32;

typedef struct NNSG3dRS {
    /* 0x00 */ u8 *c;
    /* 0x04 */ u32 unk04;
    /* 0x08 */ u32 flag;
    /* 0x0C */ u8 unk0C[0xE0 - 0x0C];
    /* 0xE0 */ s32 posScale;
    /* 0xE4 */ s32 invPosScale;
} NNSG3dRS;

void NNS_G3dGeBufferOP_N(s32 op, void *param, s32 count);

enum {
    RS_FLAG_NO_GE_CMD_A = 0x100, /* either flag suppresses the scale command */
    RS_FLAG_NO_GE_CMD_B = 0x200,
    G3OP_SCALE          = 0x1B,           /* geometry engine MTX_SCALE command */
    G3OP_SCALE_NPARAMS  = 3,
};

/*
 * SBC (model render bytecode) handler for the POSSCALE opcode.
 * Unless the render state suppresses geometry commands, multiplies the
 * current matrix by a uniform scale: the state's position scale when opt is
 * 0, else its inverse position scale (x = y = z, fixed point). Always
 * advances the bytecode pointer past the 1-byte opcode.
 */
void NNSi_G3dFuncSbc_POSSCALE(NNSG3dRS *rs, u32 opt) {
    if (!(rs->flag & RS_FLAG_NO_GE_CMD_A) && !(rs->flag & RS_FLAG_NO_GE_CMD_B)) {
        VecFx32 scale;
        if (opt == 0) {
            scale.x = scale.y = scale.z = rs->posScale;
        } else {
            scale.x = scale.y = scale.z = rs->invPosScale;
        }
        NNS_G3dGeBufferOP_N(G3OP_SCALE, &scale, G3OP_SCALE_NPARAMS);
    }
    rs->c += 1;
}
