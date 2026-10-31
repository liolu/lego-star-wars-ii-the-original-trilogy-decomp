#include "m2c.h"

enum {
    DMA0_SRC_ADDR      = 0x040000B0, /* first DMA channel register block (src, dest, control) */
    DMA_CHANNEL_STRIDE = 12,    /* bytes per channel block: 3 words */
    DMA_DUMMY_CONTROL  = 0x81400001, /* enabled, fixed src/dest, 1 unit */
};

/*
 * Programs DMA channel dmaNo with source, destination and control words.
 * The three writes are followed by two reads of the channel-0 source
 * register as a wait. If dmaNo is 0, the channel is then rewritten with a
 * 1-unit dummy transfer (src 0, dest 0) and waited on the same way.
 * Callers: MI_DmaFill16/32.
 */
void MIi_DmaSetParams_wait_noInt(u32 dmaNo, void *src, void *dest, u32 cnt) {
    volatile u32 *regs = (volatile u32 *) (DMA0_SRC_ADDR + dmaNo * DMA_CHANNEL_STRIDE);
    u32 dummy;

    regs[0] = (u32) src;
    regs[1] = (u32) dest;
    regs[2] = cnt;

    dummy = *(volatile u32 *) DMA0_SRC_ADDR;
    dummy = *(volatile u32 *) DMA0_SRC_ADDR;

    if (dmaNo == 0) {
        regs[0] = 0;
        regs[1] = 0;
        regs[2] = DMA_DUMMY_CONTROL;
    }

    dummy = *(volatile u32 *) DMA0_SRC_ADDR;
    dummy = *(volatile u32 *) DMA0_SRC_ADDR;
}
