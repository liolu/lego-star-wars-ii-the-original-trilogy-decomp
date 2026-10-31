#include "m2c.h"

typedef volatile u32 vu32;

extern u32 OS_DisableInterrupts(void);
extern u32 OS_RestoreInterrupts(u32 enabled);

enum {
    REG_DMA0SAD_ADDR       = 0x040000B0, /* DMA0 source address register */
    DMA_CHANNEL_REG_STRIDE = 12,   /* SAD, DAD, CNT words per channel */
};

/*
 * Programs DMA channel dmaNo: writes the source address, destination address
 * and control word (count | mode flags, as built by the MI_Dma* callers) to
 * that channel's SAD/DAD/CNT registers, with interrupts disabled around the
 * three writes.
 */
void MIi_DmaSetParams(u32 dmaNo, u32 src, u32 dest, u32 ctrl) {
    u32 enabled = OS_DisableInterrupts();
    vu32 *p     = (vu32 *) (REG_DMA0SAD_ADDR + dmaNo * DMA_CHANNEL_REG_STRIDE);
    *p          = src;
    *(p + 1)    = dest;
    *(p + 2)    = ctrl;
    (void) OS_RestoreInterrupts(enabled);
}
