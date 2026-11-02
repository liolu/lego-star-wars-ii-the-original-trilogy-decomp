#include "m2c.h"

typedef struct {
    u8 pad0[0x64];
    s32 val64;
    u8 pad68[0x11a];
    u16 state;
    u8 pad184[0x44a];
    u16 flags5ce;
    u8 pad5d0[0xa];
    u8 slot;
} Inner;

typedef struct {
    s32 unk0;
    Inner *in;
    u8 pad8[0xc];
    u16 f14;
    u8 pad16[2];
    u8 f18;
    u8 pad19[2];
    u8 f1b;
    u8 pad1c[4];
    void *f20;
} Obj;

typedef struct {
    u8 pad0[0x10];
    s32 f10;
} Mgr;

extern u8 data_02106d90[];

void func_02007f24(s32 *, double);
void *func_020b95bc(s32, Inner *);
void func_0209d0fc(s32 p, u8 q);

void func_02046e80(Obj *obj, s32 flag) {
    s32 b, a, c;
    s32 *pb, *pc;
    if (flag != 0) {
        obj->f18 = 1;
        obj->f1b = 0;
        func_020b95bc(0xb0, obj->in);
        func_02007f24(&a, 1.4);
        func_02007f24(&b, 1.55);
        pb = &b;
        if (pb) { // FAKE: always-true null test keeps &b in a register
            obj->in->val64 = (s32) (((s64) *pb * a + 0x800) >> 12);
        }
        obj->f14 = 0;
        {
            s32 p = ((Mgr *) *(void **) (data_02106d90 + 0xad0))->f10;
            if (p != 0) {
                u32 i = obj->in->slot;
                if (i < 4) {
                    func_0209d0fc(p, i);
                }
            }
        }
    }
    if (obj->in->state != 0x69) {
        return;
    }
    if (obj->f20 == NULL) {
        obj->f20 = func_020b95bc(0x4d, obj->in);
    }
    if (obj->f18 != 0) {
        obj->f14 = obj->f14 + 1;
        func_02007f24(&c, 0.115);
        pc = &c;
        if (pc) { // FAKE: always-true null test keeps &c in a register
            s32 n          = *pc;
            n              = -n;
            obj->in->val64 = (s32) (((s64) n * 0x2000 + 0x800) >> 12);
        }
    }
    if ((obj->in->flags5ce & 2) == 0 && obj->f14 < 0x28) {
        return;
    }
    obj->f18 = 0;
}
