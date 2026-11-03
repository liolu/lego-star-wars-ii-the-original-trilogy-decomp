#include "m2c.h"

/* One hardware OAM entry: attr0 (low half) and attr1 (high half) as one word, then attr2. */
typedef struct {
    u32 attr01;
    u16 attr2;
    u16 affine;
} OamEntry;

extern s32 data_027e02a4;
extern u8 data_027e0874[];

extern u8 *func_020aaea8(void);
extern u32 func_020ac6a8(void *arg0, void *arg1);
extern void func_020ac7bc(void *arg0);
extern void func_0205f168(void *arg0, s32 index);
extern void func_0205f520(void *arg0, s32 arg1);
extern void func_020ce878(void);
extern void func_020ca724(void);
extern void func_020ca738(void);
extern void GX_SetBankForSubOBJ(s32 subObj);
extern void GX_SetBankForSubBG(s32 subBg);

/* NDS I/O registers (main engine 0x0400xxxx, sub engine 0x0400_1xxx) and sub-engine OAM. */
enum {
    REG_IME          = 0x4000208,
    REG_POWCNT1      = 0x4000304,
    REG_DISPCAPCNT   = 0x4000064,
    REG_SWAP_BUFFERS = 0x4000540,
    REG_DB_DISPCNT   = 0x4001000,
    REG_DB_BG3CNT    = 0x400100e,
    REG_DB_BG3PA     = 0x4001030,
    REG_DB_BG3PB     = 0x4001032,
    REG_DB_BG3PC     = 0x4001034,
    REG_DB_BG3PD     = 0x4001036,
    REG_DB_BG3X      = 0x4001038,
    REG_DB_BG3Y      = 0x400103c,
    OAM_SUB          = 0x7000400
};

enum {
    POWCNT1_DISPLAY_SWAP = 0x8000,  /* bit 15: 1 = main engine on the top screen */
    VRAM_SUB_OBJ_128_D   = 0x8,       /* GX_SetBankForSubOBJ: bank D as sub OBJ */
    VRAM_SUB_BG_128_C    = 0x4         /* GX_SetBankForSubBG: bank C as sub BG */
};

static inline u8 irq_disable(void) {
    u16 old                     = (*(volatile u16 *) REG_IME);
    (*(volatile u16 *) REG_IME) = 0;
    return old;
}

static inline void irq_restore(u8 old) {
    u16 prev                    = (*(volatile u16 *) REG_IME);
    (*(volatile u16 *) REG_IME) = old;
    (void) prev;
}

/* SWAP_BUFFERS: bit 0 = manual translucent-polygon sort, bit 1 = depth mode (0 Z-buffer, 1 W-buffer). */
typedef enum {
    BUF_Z = 0,
    BUF_W = 1
} BufMode;
static inline void swap_buffers(s32 am, s32 zw) {
    (*(volatile u32 *) REG_SWAP_BUFFERS) = am | (zw << 1);
}

/*
 * One rendering pass for pass `index` (0 or 1): stores the index in data_027e02a4, runs the
 * pass callees (func_0205f168 only when flags[index] is set), then requests a 3D buffer swap
 * with manual sorting and Z or W depth chosen by the byte at +0x1b0 of func_020aaea8().
 */
static inline void update_scene(void *arg0, u8 *flags, s32 index) {
    data_027e02a4 = index;
    func_020ac6a8(data_027e0874, func_020aaea8() + 0x4c);
    if (flags[index] != 0) {
        func_0205f168(arg0, index);
    }
    func_0205f520(arg0, index);
    func_020ac7bc(data_027e0874);
    swap_buffers(1, func_020aaea8()[0x1b0] != 0 ? BUF_Z : BUF_W);
    func_020ce878();
}

/*
 * Two-pass dual-screen 3D setup. After each pass, with IME off, the main engine is moved
 * to the other screen and display capture (256x192, EVA 16) targets the other VRAM bank;
 * the sub engine shows the bank captured last:
 *   pass 0: main on top, capture into VRAM C; sub engine shows VRAM D as 12 bitmap OBJs
 *           (64x64, alpha 15) tiled 4 x 3 over 256x192 (DB_DISPCNT mode 5, OBJ on,
 *           256-dot-wide 2D bitmap OBJ mapping);
 *   pass 1: main on bottom, capture into VRAM D; sub engine shows VRAM C as BG3, a 256x256
 *           direct-colour bitmap with an identity affine matrix (1.0 = 0x100) at origin 0,0.
 */
void func_0205f1f0(void *arg0, u8 *flags) {
    u8 ime;
    s32 i;
    s32 j;
    s32 row_first;  /* OAM index of the first entry in grid row i */
    s32 obj_y;      /* attr0 Y coordinate of grid row i */

    update_scene(arg0, flags, 0);
    ime = irq_disable();
    (*(volatile u16 *) REG_POWCNT1) |= POWCNT1_DISPLAY_SWAP;
    func_020ca724();
    GX_SetBankForSubOBJ(VRAM_SUB_OBJ_128_D);
    (*(volatile u32 *) REG_DISPCAPCNT) = 0x80320010;  /* enable, 256x192, write block C, EVA 16 */
    (*(volatile u32 *) REG_DB_DISPCNT) = 0x11025;     /* display on, OBJ on, bitmap OBJ 256 wide, mode 5 */
    for (i = 0; i < 3; i++) {
        obj_y     = i * 64;
        row_first = i * 4;
        for (j = 0; j < 4; j++) {
            /* attr0: Y, bitmap OBJ mode (0x0c00); attr1: X = j * 64, size 64x64 (0xc000) */
            ((OamEntry *) OAM_SUB + row_first)[j].attr01 = obj_y | ((j * 64) << 16) | 0xc0000c00;
            /* attr2: tile = (y / 8) * 32 + x / 8, alpha 15 (0xf000; bit 16 is dropped by the u16 store) */
            ((OamEntry *) OAM_SUB + row_first)[j].attr2 = (i * 0x100 + j * 8) | 0x1f000;
        }
    }
    irq_restore(ime);

    update_scene(arg0, flags, 1);
    ime = irq_disable();
    (*(volatile u16 *) REG_POWCNT1) &= ~POWCNT1_DISPLAY_SWAP;
    func_020ca738();
    GX_SetBankForSubBG(VRAM_SUB_BG_128_C);
    (*(volatile u32 *) REG_DISPCAPCNT) = 0x80330010;  /* enable, 256x192, write block D, EVA 16 */
    (*(volatile u32 *) REG_DB_DISPCNT) = 0x10805;     /* display on, BG3 on, mode 5 */
    (*(volatile u16 *) REG_DB_BG3PA)   = 0x100;
    (*(volatile u16 *) REG_DB_BG3PB)   = 0;
    (*(volatile u16 *) REG_DB_BG3PC)   = 0;
    (*(volatile u16 *) REG_DB_BG3PD)   = 0x100;
    (*(volatile u32 *) REG_DB_BG3X)    = 0;
    (*(volatile u32 *) REG_DB_BG3Y)    = 0;
    /* keep priority and mosaic (0x43); 256x256 (0x4000) direct-colour bitmap (0x84) */
    (*(volatile u16 *) REG_DB_BG3CNT) = ((*(volatile u16 *) REG_DB_BG3CNT) & 0x43) | 0x84 | 0x4000;
    irq_restore(ime);
}
