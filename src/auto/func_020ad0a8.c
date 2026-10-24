#include "m2c.h"

/* Recomputes the +0x15e scale: the rect (+0x168..+0x174, inclusive) gives a
 * fixed-point aspect quotient via FX_Div, which scales the halfword at +0x15c.
 * The halfword is read through a volatile pointer so the product is a signed
 * 32x32->64 multiply (smull); the volatile members of L give the two dead word
 * stores of the quotient in the stack frame. */
extern s32 FX_Div(s32 a, s32 b);

typedef struct {
    u32 f0 : 1;
    u32 f1 : 1;
    u32 f2 : 1;
    u32 rest : 29;
} Flags;

static inline s32 fxmul(s32 a, s32 b) {
    return (s32) (((s64) a * b + 0x800) >> 12);
}

typedef struct {
    u16 a;
    u16 b;
    volatile s32 c;
    volatile s32 d;
} L;

void func_020ad0a8(void *p) {
    Flags *fl = (Flags *) ((char *) p + 0x160);
    s32 q;
    s32 t;
    L l;
    if (fl->f0 == 0) {
        return;
    }
    fl->f0                        = 0;
    fl->f2                        = 1;
    q                             = FX_Div((*(s32 *) ((char *) p + 0x170) - *(s32 *) ((char *) p + 0x168) + 1) << 12,
                                           (*(s32 *) ((char *) p + 0x174) - *(s32 *) ((char *) p + 0x16c) + 1) << 12);
    t                             = *(volatile u16 *) ((char *) p + 0x15c);
    l.d                           = q;
    l.c                           = q;
    l.b                           = fxmul(t, q);
    l.a                           = l.b;
    *(u16 *) ((char *) p + 0x15e) = l.a;
}
