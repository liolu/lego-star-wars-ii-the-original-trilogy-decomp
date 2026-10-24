#include "m2c.h"

typedef struct Obj {
    u8 pad0[0x4C];
    void *ptrs[4];
    u8 pad1[0x92E - 0x5C];
    s8 slot[4];
    u8 timer[4];
} Obj;

void func_020917c8(Obj *, s32, s32);

void func_02091604(Obj *arg0) {
    s32 i;
    s32 v;
    s8 *q;

    for (i = 0; i < 4; i++) {
        if (arg0->timer[i] != 0) {
            arg0->timer[i]--;
        }
        if (arg0->ptrs[i] != NULL) {
            v = arg0->slot[i];
            if (v >= 0 && arg0->timer[i] == 0) {
                q = (s8 *) arg0 + (u32) i;
                func_020917c8(arg0, i, v);
                q[0x92E] = -1;
                q[0x932] = 10;
            }
        } else {
            arg0->slot[i] = -1;
        }
    }
}
