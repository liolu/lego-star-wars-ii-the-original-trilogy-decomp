#include "m2c.h"

void func_020922f4(void *, s8, s8);
extern M2C_UNK data_0210930c;

void func_0205665c(s32 arg0, u8 *arg1) {
    s8 i;
    s8 found;
    s8 last;
    u8 *slot;

    found = -1;
    i     = 0;
    do {
        slot = (u8 *) ((s32 *) &data_0210930c)[i + 0x13];
        if (slot == NULL) {
            break;
        }
        if (*arg1 == slot[0x6BB]) {
            found = i;
        }
        i += 1;
    } while (i < 4);
    last = i - 1;
    if ((last != found) && (found != -1)) {
        func_020922f4(&data_0210930c, found, last);
    }
}
