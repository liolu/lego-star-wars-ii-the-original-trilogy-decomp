#include "m2c.h"

extern void func_02091e30(void *, u32, void *);
extern void func_020b95bc(s32, void *);

void func_020927f0(void *self) {
    void *target = M2C_FIELD(self, void **, 0x928);
    void *slot;
    u32 flags;
    u32 idx;
    s32 ready;
    u32 id = M2C_FIELD(target, u8 *, 0x5da);

    if (M2C_FIELD(target, u8 *, 0x40) == 0 || ((flags = M2C_FIELD(target, u32 *, 8)) & 2) == 2 || (flags & 0x800) == 0x800) {
        return;
    }
    if ((flags & 1) != 1) {
        return;
    }
    idx = M2C_FIELD(self, u8 *, 0x92d);
    if (id == idx) {
        return;
    }
    slot  = ((u32 *) self + idx)[0x13];
    ready = (((M2C_FIELD(slot, u32 *, 8) & 0x803) == 1) & M2C_FIELD(slot, u8 *, 0x40)) != 0;
    if (ready == 0) {
        return;
    }
    func_02091e30(self, idx, target);
    slot = ((u32 *) self + M2C_FIELD(self, u8 *, 0x92d))[0x13];
    if (slot == 0) {
        return;
    }
    func_020b95bc(0xa8, slot);
}
