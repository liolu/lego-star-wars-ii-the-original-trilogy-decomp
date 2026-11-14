#include "m2c.h"

void *_Z22__DecodeUnsignedNumberPcPm(void *, M2C_UNK *);             /* extern */

void func_020e4ba8(u8 *arg0) {
    M2C_UNK sp0;
    u8 temp_r4;
    void *temp_r0;

    temp_r4 = *arg0;
    temp_r0 = _Z22__DecodeUnsignedNumberPcPm(arg0 + 2, &sp0);
    if (!(temp_r4 & 0x40)) {
        return;
    }
    _Z22__DecodeUnsignedNumberPcPm(temp_r0, &sp0);
}
