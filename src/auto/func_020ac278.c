/* Reconstructed from USA ARM9 at 0x020ac278 (868 bytes).
 *
 * Switches the video setup to one of the VRAM layouts 1..6 (0 and anything
 * else only record the id). When the id differs from the one stored at +0x54
 * it is stored, func_020ac5dc runs, data_02101b78 is reset through
 * func_02053f80/func_02053ee8, the VRAM banks are assigned (func_020ca040 and
 * func_020ca218 take the texture and texture-palette bank masks), DISPCNT's
 * OBJ mapping bits are set, and (except in layout 4) func_020ac624 re-creates
 * the texture/palette VRAM managers with tex4x4Size (it stores that value at
 * +0x4c and passes it to NNS_GfdInitLnkTexVramManager as the 4x4 texture
 * size). With an unchanged id only the managers are rebuilt, when tex4x4Size
 * differs and the id is not 4.
 * VRAM bank masks: A=0x1 ... I=0x100. DISPCNT bits 4 and 20-21 select 1D tile
 * OBJ mapping with a 32 KB boundary; bits 5-6 = 1 select a 256-dot 2D bitmap
 * OBJ area (sub engine, layouts 2 and 3).
 */
#include "m2c.h"

typedef struct {
    u8 pad00[0x4c];
    s32 tex4x4Size;
    u8 pad50[0x4];
    u16 mode;
} Screen;

extern M2C_UNK data_02101b78;

void func_02053ee8(void *arg0, s32 arg1, s32 arg2);
void func_02053f80(void *arg0);
void func_020ac5dc(void);
void func_020ac624(void *arg0, s32 arg1);
void GX_SetBankForBG(s32 bg);
void GX_SetBankForOBJ(s32 obj);
void func_020ca040(s32 tex);
void func_020ca218(s32 texPltt);
void GX_SetBankForSubBG(s32 subBg);
void GX_SetBankForSubOBJ(s32 subObj);
void GX_SetBankForSubBGExtPltt(s32 subBgExtPltt);

enum {
    VRAM_A                = 0x1,
    VRAM_B                = 0x2,
    VRAM_C                = 0x4,
    VRAM_D                = 0x8,
    VRAM_E                = 0x10,
    VRAM_F                = 0x20,
    VRAM_G                = 0x40,
    VRAM_H                = 0x80,
    VRAM_I                = 0x100,
    DISPCNT_OBJ_CHAR_MASK = 0xffcfffef, /* clears bits 4, 20, 21 */
    DISPCNT_OBJ_BMP_MASK  = 0x60,
    OBJ_CHAR_1D_32K       = 0x10,
    OBJ_BMP_2D_W256       = 0x20,
};

static inline void SetOBJVRamModeChar(u32 mode) {
    volatile u32 *dispcnt = (volatile u32 *) 0x04000000;
    *dispcnt              = (*dispcnt & DISPCNT_OBJ_CHAR_MASK) | mode;
}

static inline void SetSubOBJVRamModeChar(u32 mode) {
    volatile u32 *dispcnt = (volatile u32 *) 0x04001000;
    *dispcnt              = (*dispcnt & DISPCNT_OBJ_CHAR_MASK) | mode;
}

static inline void SetSubOBJVRamModeBmp(u32 mode) {
    volatile u32 *dispcnt = (volatile u32 *) 0x04001000;
    *dispcnt              = (*dispcnt & ~DISPCNT_OBJ_BMP_MASK) | mode;
}

void func_020ac278(Screen *self, s32 mode, s32 tex4x4Size) {
    if (mode != self->mode) {
        self->mode = mode;
        func_020ac5dc();
        switch (mode) {
            case 0:
                break;
            case 1:
                func_02053f80(&data_02101b78);
                func_02053ee8(&data_02101b78, 0, 1);
                func_020ca040(VRAM_A | VRAM_B | VRAM_D);
                func_020ca218(VRAM_E);
                GX_SetBankForBG(VRAM_F | VRAM_G);
                GX_SetBankForSubBG(VRAM_C);
                GX_SetBankForSubOBJ(VRAM_I);
                GX_SetBankForSubBGExtPltt(VRAM_H);
                SetOBJVRamModeChar(OBJ_CHAR_1D_32K);
                SetSubOBJVRamModeChar(OBJ_CHAR_1D_32K);
                func_020ac624(self, tex4x4Size);
                return;
            case 6:
                func_02053f80(&data_02101b78);
                func_02053ee8(&data_02101b78, 0, 1);
                func_020ca040(VRAM_A | VRAM_B);
                func_020ca218(VRAM_E);
                GX_SetBankForBG(VRAM_F | VRAM_G);
                GX_SetBankForSubBG(VRAM_C);
                GX_SetBankForSubOBJ(VRAM_D);
                GX_SetBankForSubBGExtPltt(VRAM_H);
                SetOBJVRamModeChar(OBJ_CHAR_1D_32K);
                SetSubOBJVRamModeChar(OBJ_CHAR_1D_32K);
                func_020ac624(self, tex4x4Size);
                return;
            case 2:
                func_02053f80(&data_02101b78);
                func_02053ee8(&data_02101b78, 0, 1);
                func_020ca040(VRAM_A | VRAM_B);
                func_020ca218(VRAM_F);
                GX_SetBankForOBJ(VRAM_G);
                GX_SetBankForBG(VRAM_E);
                SetOBJVRamModeChar(OBJ_CHAR_1D_32K);
                SetSubOBJVRamModeBmp(OBJ_BMP_2D_W256);
                func_020ac624(self, tex4x4Size);
                return;
            case 3:
                func_02053f80(&data_02101b78);
                func_02053ee8(&data_02101b78, 0, 1);
                func_020ca040(VRAM_A);
                GX_SetBankForOBJ(VRAM_B);
                func_020ca218(VRAM_F | VRAM_G);
                GX_SetBankForBG(VRAM_E);
                SetOBJVRamModeChar(OBJ_CHAR_1D_32K);
                SetSubOBJVRamModeBmp(OBJ_BMP_2D_W256);
                func_020ac624(self, tex4x4Size);
                return;
            case 4:
                func_02053f80(&data_02101b78);
                func_02053ee8(&data_02101b78, 0, 0);
                GX_SetBankForBG(VRAM_A);
                GX_SetBankForOBJ(VRAM_B);
                SetOBJVRamModeChar(OBJ_CHAR_1D_32K);
                GX_SetBankForSubBG(VRAM_C);
                GX_SetBankForSubOBJ(VRAM_D);
                SetSubOBJVRamModeChar(OBJ_CHAR_1D_32K);
                GX_SetBankForSubBGExtPltt(VRAM_H);
                return;
            case 5:
                func_02053f80(&data_02101b78);
                func_02053ee8(&data_02101b78, 0, 1);
                func_020ca040(VRAM_A | VRAM_B);
                func_020ca218(VRAM_E);
                GX_SetBankForBG(VRAM_D);
                GX_SetBankForOBJ(VRAM_G);
                GX_SetBankForSubBG(VRAM_C);
                GX_SetBankForSubOBJ(VRAM_I);
                GX_SetBankForSubBGExtPltt(VRAM_H);
                SetOBJVRamModeChar(OBJ_CHAR_1D_32K);
                SetSubOBJVRamModeChar(OBJ_CHAR_1D_32K);
                func_020ac624(self, tex4x4Size);
                return;
        }
    } else if (self->tex4x4Size != tex4x4Size && mode != 4) {
        func_020ac624(self, tex4x4Size);
    }
}
