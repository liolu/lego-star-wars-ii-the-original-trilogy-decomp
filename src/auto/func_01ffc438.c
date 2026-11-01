#include "m2c.h"

/* Busy flag of the asynchronous geometry display-list send: NNS_G3dGeSendDL
 * sets it to 1 and hands its address to MI_SendGXCommandAsyncFast together
 * with the completion callback func_01ffc450, which stores 0 into it. */
extern s32 data_027e0e78;

/* Spin until the pending asynchronous display-list send has completed
 * (called by NNS_G3dGeBufferOP_N and NNS_G3dGeFlushBuffer). */
void func_01ffc438(void) {
    while (*(volatile s32 *) &data_027e0e78 != 0) { // FAKE: flag cleared asynchronously by the DMA-done callback func_01ffc450
    }
}
