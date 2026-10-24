#include "m2c.h"

/* func_02060800: subtracting counterpart of func_02060710. */

extern M2C_UNK data_020f88c4;

static inline s32 fxmul(s32 a, s32 b) {
    return (s32) (((s64) a * b + (1 << 11)) >> 12);
}

static inline s32 tbl0(s32 v) {
    return *((s16 *) ((s8 *) &data_020f88c4 + (((v >> 4) * 2) * 2)));
}

static inline s32 tbl1(s32 v) {
    return *((s16 *) ((s8 *) &data_020f88c4 + ((((v >> 4) * 2) + 1) * 2)));
}

void func_02060800(void *p) {
    u16 s0;
    u16 s1;
    u16 s2;
    s32 m;

    m = fxmul(M2C_FIELD(p, s32 *, 0x1c), tbl0(M2C_FIELD(&s0, u16 *, 0) = M2C_FIELD(p, u16 *, 0xc26)));
    M2C_FIELD(p, s32 *, 0x8) =
        M2C_FIELD(p, s32 *, 0x10) - fxmul(m, tbl0(M2C_FIELD(&s1, u16 *, 0) = M2C_FIELD(p, u16 *, 0xc28)));
    M2C_FIELD(p, s32 *, 0xc) =
        M2C_FIELD(p, s32 *, 0x14) - fxmul(m, tbl1(M2C_FIELD(&s2, u16 *, 0) = M2C_FIELD(p, u16 *, 0xc28)));
}
