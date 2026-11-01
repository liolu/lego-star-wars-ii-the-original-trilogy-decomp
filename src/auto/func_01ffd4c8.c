#include "m2c.h"

typedef volatile u32 vu32;

extern u32 OS_DisableInterrupts(void);
extern M2C_UNK OS_RestoreInterrupts(s32);

enum {
    REG_DMA0SAD        = 0x040000B0, /* channel 0 source register */
    DMA_CHANNEL_STRIDE = 12,  /* SAD, DAD, CNT words per channel */
    /* enable, fixed source and destination, one unit */
    DMA_DUMMY_CNT = 0x81400001,
};

/*
 * Programs one DMA channel with interrupts disabled: writes the source,
 * destination and control words (callers MI_DmaCopy16/32 pass the unit
 * count ORed with the enable/width bits), then reads channel 0's source
 * register twice. When the channel is 0, a one-unit fixed-address
 * transfer of zero to zero is started right after.
 */
void func_01ffd4c8(u32 ch, u32 src, u32 dst, u32 ctrl) {
    u32 saved  = OS_DisableInterrupts();
    vu32 *regs = (vu32 *) (REG_DMA0SAD + ch * DMA_CHANNEL_STRIDE);
    u32 dummy;

    regs[0] = src;
    regs[1] = dst;
    regs[2] = ctrl;
    dummy   = *(vu32 *) REG_DMA0SAD;
    dummy   = *(vu32 *) REG_DMA0SAD;
    if (ch == 0) {
        regs[0] = 0;
        regs[1] = 0;
        regs[2] = DMA_DUMMY_CNT;
    }
    OS_RestoreInterrupts(saved);
}
