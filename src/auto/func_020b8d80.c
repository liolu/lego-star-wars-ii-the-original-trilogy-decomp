#include "m2c.h"

M2C_UNK func_0205e624(void *, M2C_UNK, M2C_UNK);    /* extern */
M2C_UNK func_020b8e18();                            /* extern */
M2C_UNK func_020b8f04(void (*)(s32, s32, s32));     /* extern */
M2C_UNK func_020b8f20(u8);                          /* extern */
M2C_UNK func_020b8f48(u8);                          /* extern */
void func_020b9434(s32, s32, s32);
M2C_UNK func_020bb028();                            /* extern */
extern M2C_UNK data_02104364;
extern u8 data_0210db14;

typedef struct {
    s32 heap;          /* +0x00 */
    u8 unk4[0x20];     /* +0x04 */
    u8 state[0xC];     /* +0x24, 0x0210db14 */
} SndWork;

extern SndWork data_0210daf0;

void func_020b8d80(void) {
    func_0205e624(data_0210daf0.state, 0, 0xC);
    func_020bb028();
    func_020b8f20(M2C_FIELD(&data_02104364, u8 *, 9));
    func_020b8f48(M2C_FIELD(&data_02104364, u8 *, 0xA));
    M2C_FIELD(&data_0210db14, u8 *, 9)   = 0xFF;
    M2C_FIELD(&data_0210db14, u8 *, 0xA) = 0;
    func_020b8f04(func_020b9434);
    data_0210db14 = 1;
    func_020b8e18();
}
