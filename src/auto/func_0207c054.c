#include "m2c.h"

s32 func_02086534(); /* extern */
M2C_UNK func_0205ecc4(); /* extern */
M2C_UNK func_020601ac(); /* extern */
extern M2C_UNK data_02102900;
extern M2C_UNK data_02104d90;

void func_0207c054(void *arg0) {
    if (M2C_FIELD(arg0, u8 *, 0xDD) == 0) {
        func_02086534(&data_02104d90, M2C_FIELD(arg0, u8 *, 0xDC) + 0x49);
        func_0205ecc4(&data_02102900, 9, 0, M2C_FIELD(arg0, u8 *, 0xDC), 0, 0);
        func_0205ecc4(&data_02102900, 0xA, 1, 0, 0, 0);
        return;
    }
    if (func_02086534(&data_02104d90, M2C_FIELD(arg0, u8 *, 0xDC) + 0x49) != 0) {
        func_020601ac(&data_02102900, 1, 0x35, -1, 2, 0);
        M2C_FIELD(arg0, s8 *, 0xDE) = 0;
    } else {
        M2C_FIELD(arg0, s8 *, 0xDF) = 0;
    }
}
