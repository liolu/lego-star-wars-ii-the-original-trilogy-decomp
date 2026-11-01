/* func_02072f30 - if the word at data_02104d20 + 4 is still zero, sets up
 * the object at data_02104d24 through func_0201ad50 with resource
 * data_020edfe8.  The two integer arguments are converted to 20.12 fixed
 * point and passed as func_0201ad50's two by-value word aggregates;
 * blockArg is forwarded to func_0201abbc when func_0201ad50 allocates its
 * 0x80 byte block.  If the slot at data_02104d20 + 4 is set, nothing is done.
 */
#include "m2c.h"

typedef struct {
    s32 v;
} UnkVal;

enum {
    FX32_SHIFT = 12,
};

extern void func_0201ad50(void *p0, void *p1, void *p2, UnkVal p3, UnkVal p4, u8 p5, void *p6);
extern M2C_UNK data_02104d20;
extern M2C_UNK data_02104d24;
extern M2C_UNK data_020edfe8;

void func_02072f30(void *blockArg, s32 first, s32 second, void *entry) {
    UnkVal firstFx;
    UnkVal secondFx;
    if (M2C_FIELD(&data_02104d20, void **, 4) != NULL) {
        return;
    }
    secondFx.v = second << FX32_SHIFT;
    firstFx.v  = first << FX32_SHIFT;
    func_0201ad50(&data_02104d24, entry, &data_020edfe8, firstFx, secondFx, 0, blockArg);
}
