#include "m2c.h"

void func_02067ca0(void *, s32, u8, u8, u8);

s32 func_0206e098(void *arg0) {
    s32 value;

    switch (M2C_FIELD(arg0, s32 *, 0x38)) {
        case 0:
            func_02067ca0((u8 *) arg0 + 0x2C, 0, 0, (u8) M2C_FIELD(arg0, s32 *, 0x34), 0xC0);
            if (M2C_FIELD(arg0, s32 *, 0x34) >= 0xFF) {
                return 1;
            }
            break;
        case 1:
            func_02067ca0((u8 *) arg0 + 0x2C, 0xFF - M2C_FIELD(arg0, s32 *, 0x34), 0, 0xFF, 0xC0);
            if (M2C_FIELD(arg0, s32 *, 0x34) >= 0xFF) {
                return 1;
            }
            break;
        case 2:
            func_02067ca0((u8 *) arg0 + 0x2C, 0, 0, 0xFF, (u8) M2C_FIELD(arg0, s32 *, 0x34));
            if (M2C_FIELD(arg0, s32 *, 0x34) >= 0xC0) {
                return 1;
            }
            break;
        case 3:
            value = M2C_FIELD(arg0, s32 *, 0x34);
            func_02067ca0((u8 *) arg0 + 0x2C, 0, (u8) (0xC0 - value), 0xFF, 0xC0);
            if (M2C_FIELD(arg0, s32 *, 0x34) >= 0xC0) {
                return 1;
            }
            break;
    }
    return 0;
}
