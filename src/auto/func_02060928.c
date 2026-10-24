#include "m2c.h"

typedef struct {
    u8 pad[0x700];
    u8 used[0x20];
    u8 count;
    u8 pad2[3];
} Slot;

extern Slot data_02103374[];

u16 func_02060928(s32 idx) {
    u16 i;
    u16 n;

    i = 0;
    n = 0x20;
    do {
        if (data_02103374[idx].used[i] == 1) {
            i++;
        } else {
            data_02103374[idx].used[i] = 1;
            if (i >= data_02103374[idx].count) {
                data_02103374[idx].count++;
            }
            return i;
        }
        n--;
    } while (n != 0);
    return i;
}
