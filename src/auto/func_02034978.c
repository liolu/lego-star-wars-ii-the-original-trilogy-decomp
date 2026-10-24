#include "m2c.h"

typedef struct {
    u8 pad0[0x4];
    u8 *f4;
    u8 pad8[0x16];
    u8 f1e;
    u8 pad1f[0x17];
    volatile u16 f36;
    s32 f38;
    s32 f3c;
    s32 f40;
} Obj;

s32 func_0200aef8(void *, void *, s32);
void func_0202ff38(Obj *);
extern s32 data_02104d90;

s32 func_02034978(Obj *this) {
    s32 var_r6;
    s32 var_r7;
    volatile s32 sp0;
    s32 *src;

    var_r6 = 1;
    var_r7 = var_r6;
    sp0    = 0x14000;
    if ((func_0200aef8(this->f4 + 0xB0, &this->f38, 0x14000) != 0) && (this->f4[0x5D6] == 0)) {
        var_r7 = 0;
    }
    if ((var_r7 == 0) && ((data_02104d90 & 1) == 0)) {
        var_r6 = 0;
    }
    if (var_r6 != 0) {
        src = (s32 *) (this->f4 + 0xB0);
        if (&this->f38 != src) {
            this->f38 = src[0];
            this->f3c = src[1];
            this->f40 = src[2];
        }
        this->f36 = 0;
    } else {
        this->f36 = this->f36 + 1;
        if (this->f36 >= 60) {
            src = (s32 *) (this->f4 + 0xB0);
            if (&this->f38 != src) {
                this->f38 = src[0];
                this->f3c = src[1];
                this->f40 = src[2];
            }
            this->f36 = 0;
            if (!(this->f1e & 2)) {
                func_0202ff38(this);
            }
            return 1;
        }
    }
    return 0;
}
