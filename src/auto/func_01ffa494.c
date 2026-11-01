#include "m2c.h"

/*
 * H-blank interrupt handler (installed by func_02060618 via
 * OS_SetIrqFunction(2, func_01ffa494)).
 *
 * data_027e0390 points at a per-scanline table of 4-word entries
 * (func_020606c8 fills 0xC00 bytes = 192 lines * 16 bytes) and
 * data_027e038c at the 4-word destination block.  While the display is
 * still drawing visible lines, the entry for the NEXT line is copied to
 * the destination.  In every case the H-blank bit is then set in the
 * DTCM IRQ check buffer, like OS_SetIrqCheckFlag.
 */

enum {
    REG_VCOUNT_ADDR    = 0x04000006, /* current scanline */
    LAST_VISIBLE_LINE  = 0xBF,       /* 191: last drawn line */
    DTCM_SYSRV_OFFSET  = 0x3FC0,     /* system area at the end of DTCM */
    INTR_CHECK_BUF_OFF = 0x38,       /* IRQ check flags within it */
    IRQ_HBLANK         = 2,          /* H-blank interrupt mask bit */
    WORDS_PER_LINE     = 4,
};

extern u32 SDK_AUTOLOAD_DTCM_START[];
extern s32 *data_027e038c;
extern s32 *data_027e0390;

static inline void set_irq_check(u32 mask) {
    *(volatile u32 *) (((u32) SDK_AUTOLOAD_DTCM_START + DTCM_SYSRV_OFFSET) + INTR_CHECK_BUF_OFF) |= mask;
}

void func_01ffa494(void) {
    s32 line = *(volatile u16 *) REG_VCOUNT_ADDR;

    if (line > LAST_VISIBLE_LINE) {
        set_irq_check(IRQ_HBLANK);
        return;
    }
    {
        s32 i    = (line + 1) * WORDS_PER_LINE;
        s32 *src = &data_027e0390[i];
        s32 *dst = data_027e038c;
        dst[0]   = src[0];
        dst[1]   = src[1];
        dst[2]   = src[2];
        dst[3]   = src[3];
    }
    set_irq_check(IRQ_HBLANK);
}
