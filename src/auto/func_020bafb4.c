/* Reconstructed from USA ARM9 at 0x020bafb4 (116 bytes).
 * Names and types are inferred from the binary.
 */
#include "m2c.h"

void SND_WaitForFreeCommand(s32);
void SND_SetChannelTimer(s32, s32);

/* Sets the timer of a sound channel from a hardware division (DIVCNT 32/64 mode). */
void func_020bafb4(void *arg0) {
    s32 denom;

    if (M2C_FIELD(arg0, u8 *, 1) != 3) {
        return;
    }
    SND_WaitForFreeCommand(1);
    denom                                                = M2C_FIELD(arg0, s32 *, 8);
    M2C_FIELD((void *) 0x04000280, volatile u16 *, 0)    = 0U;
    M2C_FIELD((void *) 0x04000280, volatile s32 *, 0x10) = 0xFFB0FF;
    *(volatile u64 *) 0x04000298                         = (u64) (u32) denom;
    do {

    } while (M2C_FIELD((void *) 0x04000280, volatile u16 *, 0) & 0x8000);
    SND_SetChannelTimer(1 << M2C_FIELD(arg0, u8 *, 2), *(s32 *) 0x040002A0);
}
