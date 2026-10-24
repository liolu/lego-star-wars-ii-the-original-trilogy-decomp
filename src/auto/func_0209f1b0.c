#include "m2c.h"

typedef struct {
    u16 w0, w2, w;
} Hdr;

typedef struct {
    s32 *v;
    u16 a;
    u16 on : 1;
    u16 f1 : 1;
    u16 f2 : 1;
    u16 f3 : 1;
} Slot;

extern void func_0209f2cc(Slot *);
extern s32 func_0205ab1c(u16);
extern s32 func_020c65c8(s32, s32);
extern s32 *func_0205e5e8(void);
extern s32 *NNS_G3dAllocAnmObj(s32 *, s32, s32);
extern void func_020c38f0(s32 *, s32, s32, s32);

s32 func_0209f1b0(Slot *slot, u16 id, s32 mdl, u8 arg3, u8 arg4, u8 mode) {
    s32 res;
    s32 *obj;
    s32 *p;
    s32 *dst;
    s32 t;
    s32 u;

    if (mode != 1) {
        func_0209f2cc(slot);
    }
    slot->a = id;
    res     = func_0205ab1c(id);
    if (res != 0) {
        res     = func_020c65c8(res, 0);
        obj     = NNS_G3dAllocAnmObj(func_0205e5e8(), res, mdl);
        slot->v = obj;
        func_020c38f0(obj, res, mdl, 0);
        slot->on = 1;
        slot->f1 = 0;
        slot->f2 = arg3;
        slot->f3 = arg4;
        if (slot->f3) {
            s32 v[3];
            s32 *q = &v[1];
            p      = slot->v;
            if (p == 0) {
                v[1] = 0;
            } else {
                v[1] = ((Hdr *) p[2])->w << 12;
            }
            u    = *q;
            t    = u - 0x1000;
            dst  = slot->v;
            v[2] = t;
            v[0] = t;
            *dst = t;
        }
        return 1;
    }
    return 0;
}
