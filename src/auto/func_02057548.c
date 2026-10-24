#include "m2c.h"

typedef struct {
    u16 v;
} UnkAngle;

typedef struct {
    s32 v;
} UnkFlag;

typedef struct {
    UnkAngle angle;
    UnkAngle newAngle;
    UnkFlag flag;
    UnkFlag newFlag;
} UnkRot;

typedef struct {
    u16 deg;
    u8 flag;
} UnkRotInput;

extern void *func_020aaeb4(void);

void func_02057548(s32 arg0, UnkRotInput *arg1) {
    UnkRot rot;
    void *global;

    rot.newAngle.v                       = (arg1->deg << 16) / 360;
    rot.newFlag.v                        = arg1->flag;
    rot.flag                             = rot.newFlag;
    rot.angle                            = rot.newAngle;
    global                               = func_020aaeb4();
    M2C_FIELD(global, UnkAngle *, 0x27a) = rot.angle;
    M2C_FIELD(global, UnkFlag *, 0x27c)  = rot.flag;
}
