#include "m2c.h"

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct {
    void *vptr;
    s32 p0;
    s32 p1;
    s32 p2;
    s32 f10;
    s32 a0;
    s32 a1;
    s32 a2;
    s32 sum;
    u8 f24[4];
    s32 b0;
    s32 b1;
    s32 b2;
} Obj;

extern M2C_UNK data_020fcd18;
extern M2C_UNK data_020fe438;

void func_0200af88(s8 *arg0, s8 arg1);
void func_0204c594(Obj *o, Vec3 *src);
void func_01ff9c88(Obj *o, Vec3 *v);

/* Build a local shape object from two objects and pass it to func_01ff9c88.
 *
 * - shape: given vptr data_020fcd18, words +4..+0x20 zeroed, func_0200af88 on
 *   the bytes at +0x24 (arg -1), then vptr data_020fe438, +0x26 = 0, words
 *   +0x28..+0x30 zeroed and +0x25 = 5 (same sequence as func_0200a158).
 * - The vector at +4 is copied from arg1 (skipped if it is the same address).
 * - func_0204c594(&shape, &sum), where sum = arg0 +0x28..+0x30 plus arg1
 *   +0x28..+0x30 per component.
 * - func_01ff9c88(&shape, &arg0->p0).
 */
void func_01ff9d4c(Obj *arg0, Obj *arg1) {
    Obj shape;
    Vec3 sum;
    s32 sx;
    s32 sy;
    s32 sz;

    shape.vptr = &data_020fcd18;
    shape.p0   = 0;
    shape.p1   = 0;
    shape.p2   = 0;
    shape.f10  = 0;
    shape.a0   = 0;
    shape.a1   = 0;
    shape.a2   = 0;
    shape.sum  = 0;
    func_0200af88((s8 *) &shape.f24[0], -1);
    shape.f24[2] = 0;
    shape.vptr   = &data_020fe438;
    shape.b0     = 0;
    shape.b1     = 0;
    shape.b2     = 0;
    shape.f24[1] = 5;
    if (&shape.p0 != &arg1->p0) {
        shape.p0 = arg1->p0;
        shape.p1 = arg1->p1;
        shape.p2 = arg1->p2;
    }

    sx    = arg0->b0 + arg1->b0;
    sz    = arg0->b2 + arg1->b2;
    sy    = arg0->b1 + arg1->b1;
    sum.x = sx;
    sum.y = sy;
    sum.z = sz;
    func_0204c594(&shape, &sum);
    func_01ff9c88(&shape, (Vec3 *) &arg0->p0);
}
