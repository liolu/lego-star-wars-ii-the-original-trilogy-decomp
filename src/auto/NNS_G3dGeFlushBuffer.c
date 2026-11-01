#include "m2c.h"

/* Software geometry-command buffer: word count, then the queued command words. */
typedef struct {
    u32 idx;
    u32 buffer[1];
} GeBuffer;

enum {
    GX_COMMAND_FIFO = 0x4000400, /* hardware geometry-engine command FIFO register */
};

extern void MIi_CpuSend32(const void *srcp, volatile void *destp, u32 size);
extern void func_01ffc438(void);
extern s32 data_027e0e78;
extern GeBuffer *NNS_G3dGeBuffer;

/*
 * Flush the buffered geometry commands to the GX command FIFO.
 * data_027e0e78 is the flag NNS_G3dGeSendDL sets to 1 before its async
 * display-list send; while it is set, func_01ffc438 is called first.
 * Then, if a buffer is installed and holds words, they are sent to the
 * FIFO (count * 4 bytes) and the word count is reset to 0.
 */
void NNS_G3dGeFlushBuffer(void) {
    if (data_027e0e78) {
        func_01ffc438();
    }
    if (NNS_G3dGeBuffer && NNS_G3dGeBuffer->idx != 0) {
        MIi_CpuSend32(&NNS_G3dGeBuffer->buffer[0], (volatile void *) GX_COMMAND_FIFO, NNS_G3dGeBuffer->idx * 4);
        NNS_G3dGeBuffer->idx = 0;
    }
}
