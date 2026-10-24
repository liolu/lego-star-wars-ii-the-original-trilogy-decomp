#include "m2c.h"

typedef struct {
    u16 v;
} A16;

typedef struct {
    A16 a;
    A16 b;
} Pair;

s32 func_01ffa41c(M2C_UNK);                         /* extern */
M2C_UNK func_01ffa430(void *, M2C_UNK);             /* extern */

void func_0205c03c(void *arg0, M2C_UNK arg1) {
    Pair l;
    s32 t;
    t                            = func_01ffa41c(arg1);
    l.b.v                        = (s16) (((s64) t * 0xB60B60B60BLL + 0x80000000000LL) >> 44);
    l.a                          = l.b;
    M2C_FIELD(arg0, u16 *, 0x1c) = l.a.v;
    M2C_FIELD(arg0, s32 *, 0x14) = func_01ffa41c(arg1);
    M2C_FIELD(arg0, s32 *, 0x18) = func_01ffa41c(arg1);
    M2C_FIELD(arg0, s32 *, 0x10) = func_01ffa41c(arg1);
    func_01ffa430(arg0, arg1);
}
