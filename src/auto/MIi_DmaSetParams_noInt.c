#include "m2c.h"

typedef volatile u32 vu32;

enum {
    REG_DMA0SAD_ADDR   = 0x040000b0, /* DMA0 source address register */
    DMA_CHANNEL_STRIDE = 12,       /* SAD, DAD, CNT words per channel */
};

/*
 * Programs DMA channel dmaNo: writes the source address, destination address
 * and control word (count | mode | enable bits, built by the callers
 * MI_DmaFill16Async/MI_DmaFill32Async) into its three consecutive registers.
 */
void MIi_DmaSetParams_noInt(u32 dmaNo, u32 src, u32 dest, u32 ctrl) {
    vu32 *p  = (vu32 *) (REG_DMA0SAD_ADDR + dmaNo * DMA_CHANNEL_STRIDE);
    *p       = src;
    *(p + 1) = dest;
    *(p + 2) = ctrl;
}
