#include "m2c.h"

typedef struct {
    u16 v;
} H;

typedef struct {
    s16 v;
} S;

typedef struct {
    u16 v;
    u8 f2;
    u8 f3;
    u8 f4;
    u8 f5;
} Key;

typedef struct {
    u8 pad[0xca];
    u16 fca;
} Target;

typedef struct {
    H a;
    H b;
    H c;
} Loc;

extern void func_0202732c(Key *);

void func_02027470(Key *out, Target *t, H h) {
    Loc l;
    s32 d;

    func_0202732c(out);
    l.b.v  = t->fca;
    l.c.v  = h.v - l.b.v;
    l.a    = l.c;
    out->v = l.a.v;
    d      = *(s16 *) &l.c;
    if (d < 0) {
        d = -d;
    }
    if (d > 0x5555) {
        out->f2 = 0;
    } else {
        out->f2 = 1;
    }
    out->f4 = 100;
    out->f5 = 100;
}
