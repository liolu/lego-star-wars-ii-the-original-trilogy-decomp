#include "m2c.h"

extern u8 *_Z22__DecodeUnsignedNumberPcPm(u8 *p, s32 *out);

typedef struct {
    /* 0x00 */ s32 f00;
    /* 0x04 */ u8 *f04;
} Arg1;

typedef struct {
    /* 0x00 */ u8 f00[0x14];
    /* 0x14 */ s32 f14;
    /* 0x18 */ s32 f18;
    /* 0x1c */ u8 f1c[0x1c];
    /* 0x38 */ s32 f38;
    /* 0x3c */ u8 f3c[0xc];
    /* 0x48 */ s32 f48;
    /* 0x4c */ u8 f4c[0x14];
    /* 0x60 */ s32 f60;
    /* 0x64 */ s32 f64;
    /* 0x68 */ u16 f68;
    /* 0x6a */ u8 f6a;
    /* 0x6b */ u8 f6b;
    /* 0x6c */ u8 f6c;
} Self;

void func_020e4ae8(Self *self, Arg1 *arg1) {
    u8 *p    = arg1->f04;
    u8 flags = *p;
    u8 *q;

    self->f6b = (flags & 0x40) != 0;
    self->f6a = (flags & 0x20) != 0;
    self->f6c = (flags & 0x80) != 0;
    self->f68 = p[1] << 4;
    self->f68 |= 0x4000;
    q = _Z22__DecodeUnsignedNumberPcPm(p + 2, &self->f60);
    if (flags & 0x40) {
        _Z22__DecodeUnsignedNumberPcPm(q, &self->f64);
    }
    if (flags & 0x40) {
        if (flags & 0x80) {
            self->f18 = self->f38;
        } else {
            self->f18 = self->f48;
        }
    } else {
        self->f18 = self->f14;
    }
}
