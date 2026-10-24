#include "m2c.h"

typedef struct Ctrl02010a24 {
    u8 unk_00[0x28];
    u16 unk_28;
} Ctrl02010a24;

typedef struct Unk02010a24 {
    u8 unk_00[0x8c];
    s32 unk_8c;
    u8 unk_90[0x148 - 0x90];
    s32 *unk_148;
    u8 unk_14c[0x190 - 0x14c];
    Ctrl02010a24 *unk_190;
    u8 unk_194[0x1a8 - 0x194];
    u16 unk_1a8;
} Unk02010a24;

extern void func_0205c8b0(void *, Unk02010a24 *);
extern void func_0205c37c(void *, Unk02010a24 *, s32, s32 *, s32);

void func_02010a24(Unk02010a24 *self, u16 arg) {
    s32 *p;
    s32 v;
    s32 *vp;

    if (arg == self->unk_190->unk_28) {
        return;
    }
    func_0205c8b0(self->unk_190, self);
    vp = &v;
    p  = self->unk_148;
    if (p) {
        v = *p;
    } else {
        v = 0;
    }
    func_0205c37c(self->unk_190, self, arg, &self->unk_8c, *vp >> 12);
    self->unk_1a8 |= 0x200;
}
