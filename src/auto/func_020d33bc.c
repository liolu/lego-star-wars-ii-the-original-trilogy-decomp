#include "m2c.h"

/*
 * Touch-panel calibration setter (data_02117450 is the TP driver state used by
 * TP_Init / TP_RequestSamplingAsync / TPi_TpCallback).
 * param = { x0, y0, x_dot_size, y_dot_size } (s16 each; caller func_020645bc
 * passes a 4-entry s16 array). NULL clears the calibrated flag. Otherwise, with
 * interrupts disabled, each axis stores its origin and dot size plus the
 * reciprocal 2^28 / dot_size from the hardware divider (32/32 mode); a zero dot
 * size clears that axis. The calibrated flag is then set.
 */

enum {
    REG_DIVCNT     = 0x4000280,
    REG_DIV_NUMER  = 0x4000290,
    REG_DIV_DENOM  = 0x4000298,
    REG_DIV_RESULT = 0x40002a0,
    DIVCNT_BUSY    = 0x8000,
    DIV_MODE_32_32 = 0,
    INV_ONE        = 0x10000000,
};

typedef struct {
    u8 pad0[0x1c];
    s32 x0;
    s32 x_dot_size;
    s32 x_dot_size_inv;
    s32 y0;
    s32 y_dot_size;
    s32 y_dot_size_inv;
    u16 calibrated;
} TpState;

extern TpState data_02117450;
extern u32 OS_DisableInterrupts(void);
extern void OS_RestoreInterrupts(u32);

void func_020d33bc(s16 *param) {
    u32 irq;
    s32 dot_size;

    if (param == (void *) 0) {
        data_02117450.calibrated = 0;
        return;
    }
    irq      = OS_DisableInterrupts();
    dot_size = param[2];
    if (dot_size != 0) {
        *(volatile u16 *) REG_DIVCNT    = DIV_MODE_32_32;
        *(volatile u32 *) REG_DIV_NUMER = INV_ONE;
        *(volatile u64 *) REG_DIV_DENOM = (u32) dot_size;
        data_02117450.x0                = param[0];
        data_02117450.x_dot_size        = dot_size;
        while (*(volatile u16 *) REG_DIVCNT & DIVCNT_BUSY) {
        }
        data_02117450.x_dot_size_inv = *(volatile s32 *) REG_DIV_RESULT;
    } else {
        data_02117450.x0             = 0;
        data_02117450.x_dot_size     = 0;
        data_02117450.x_dot_size_inv = 0;
    }
    dot_size = param[3];
    if (dot_size != 0) {
        *(volatile u16 *) REG_DIVCNT    = DIV_MODE_32_32;
        *(volatile u32 *) REG_DIV_NUMER = INV_ONE;
        *(volatile u64 *) REG_DIV_DENOM = (u32) dot_size;
        data_02117450.y0                = param[1];
        data_02117450.y_dot_size        = dot_size;
        while (*(volatile u16 *) REG_DIVCNT & DIVCNT_BUSY) {
        }
        data_02117450.y_dot_size_inv = *(volatile s32 *) REG_DIV_RESULT;
    } else {
        data_02117450.y0             = 0;
        data_02117450.y_dot_size     = 0;
        data_02117450.y_dot_size_inv = 0;
    }
    OS_RestoreInterrupts(irq);
    data_02117450.calibrated = 1;
}
