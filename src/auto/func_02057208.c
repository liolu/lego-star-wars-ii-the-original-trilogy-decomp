#include "m2c.h"

typedef struct {
    s32 v;
} UnkVal;

typedef struct {
    UnkVal a;
    UnkVal b;
} UnkPair;

extern void *func_020aaeb4(void);
extern void func_020a1434(void *obj, UnkVal a, UnkVal b);

/*
 * Handler entry in pointer table data_020ed928 (arg0 unused). Copies the two
 * words of *arg1 and hands them by value to func_020a1434, which stores them
 * at +0x2CC/+0x2D0 of the global object returned by func_020aaeb4
 * (&data_02109c88).
 */
void func_02057208(s32 arg0, UnkPair *arg1) {
    UnkVal a;
    UnkVal b;
    void *obj;

    b   = arg1->b;
    a   = arg1->a;
    obj = func_020aaeb4();
    func_020a1434(obj, a, b);
}
