#include "m2c.h"

typedef struct {
    s32 v;
} Fx;

typedef struct {
    Fx a;
    Fx b;
    Fx c;
    Fx d;
    u8 flag;
} Input;

typedef struct {
    Fx d;
    Fx c;
    Fx b;
    Fx a;
    Fx newD;
    Fx newC;
    Fx newB;
    Fx newA;
} Loc;

extern void *func_020aaeb4(void);

void func_020578f4(s32 arg0, Input *arg1) {
    Loc l;
    u8 flag;
    void *g;

    l.newD                     = arg1->d;
    l.d                        = l.newD;
    l.newC                     = arg1->c;
    l.c                        = l.newC;
    l.newB                     = arg1->b;
    l.b                        = l.newB;
    l.newA                     = arg1->a;
    l.a                        = l.newA;
    flag                       = arg1->flag;
    g                          = func_020aaeb4();
    M2C_FIELD(g, u8 *, 0x378)  = flag;
    M2C_FIELD(g, s32 *, 0x380) = l.a.v;
    M2C_FIELD(g, s32 *, 0x384) = l.b.v;
    M2C_FIELD(g, s32 *, 0x388) = l.c.v;
    M2C_FIELD(g, s32 *, 0x38c) = l.d.v;
}
