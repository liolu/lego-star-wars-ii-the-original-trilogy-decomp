#include "m2c.h"

typedef struct {
    u16 v;
} A16;

typedef struct {
    A16 a;
    A16 b;
    A16 c;
} Trip;

s32 func_01ffa41c(M2C_UNK);                         /* extern */
M2C_UNK func_01ffa430(void *, M2C_UNK);             /* extern */

void func_0205bdb4(void *arg0, M2C_UNK arg1) {
    Trip m;
    Trip l;
    s32 t;
    if (M2C_FIELD(arg0, u8 *, 0x24) == 0) {
        M2C_FIELD(arg0, s32 *, 0x10) = func_01ffa41c(arg1);
    } else {
        t                            = func_01ffa41c(arg1);
        m.c.v                        = (s16) (((s64) t * 0xB60B60B60BLL + 0x80000000000LL) >> 44);
        l.c                          = m.c;
        M2C_FIELD(arg0, u16 *, 0x1c) = l.c.v;
        t                            = func_01ffa41c(arg1);
        m.b.v                        = (s16) (((s64) t * 0xB60B60B60BLL + 0x80000000000LL) >> 44);
        l.b                          = m.b;
        M2C_FIELD(arg0, u16 *, 0x1e) = l.b.v;
        t                            = func_01ffa41c(arg1);
        m.a.v                        = (s16) (((s64) t * 0xB60B60B60BLL + 0x80000000000LL) >> 44);
        l.a                          = m.a;
        M2C_FIELD(arg0, u16 *, 0x20) = l.a.v;
        M2C_FIELD(arg0, s32 *, 0x14) = func_01ffa41c(arg1);
        M2C_FIELD(arg0, s32 *, 0x18) = func_01ffa41c(arg1);
        M2C_FIELD(arg0, s32 *, 0x10) = func_01ffa41c(arg1);
    }
    func_01ffa430(arg0, arg1);
}
