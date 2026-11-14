#include "m2c.h"

typedef struct {
    u32 q0, q1, q2, q3;
} E16;

typedef struct {
    E16 arr[5];
    u32 v;
} Tail;

typedef struct {
    s32 f00, f04;
    u8 *f08;
    s32 f0c, f10, f14;
} B;

typedef struct {
    s32 f00, f04;
    u8 *f08;
    u8 *f0c;
    s32 f10, f14;
    u8 *f18;
    Tail t;
} A;

typedef struct {
    s32 f00, f04;
    u8 *f08;
    s32 f0c, f10, f14;
    s32 f18, f1c;
    u8 *f20;
    u8 *f24;
    s32 f28, f2c;
    u8 *f30;
    Tail t;
} S;

extern u8 func_020e335c(void *);
extern u8 func_020e3378(void *);
extern u8 *_Z20__DecodeSignedNumberPcPl(u8 *, s32 *);

u8 *func_020e3fb0(A *a, B *b) {
    S s;
    s32 v;
    u8 st;

    s.f00 = b->f00;
    s.f04 = b->f04;
    s.f08 = b->f08;
    s.f0c = b->f0c;
    s.f10 = b->f10;
    s.f14 = b->f14;
    s.f18 = a->f00;
    s.f1c = a->f04;
    s.f20 = a->f08;
    s.f24 = a->f0c;
    s.f28 = a->f10;
    s.f2c = a->f14;
    s.f30 = a->f18;
    s.t   = a->t;

    st = func_020e335c(&s);
    for (;;) {
        switch (st) {
            default:
                return 0;
            case 0:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
            case 9:
            case 10:
            case 11:
            case 12:
            case 15:
            case 16:
            case 17:
            case 18:
                st = func_020e3378(&s);
                continue;
            case 13: {
                u8 *p;
                _Z20__DecodeSignedNumberPcPl(s.f08 + 1, &v);
                p      = s.f30 + v;
                a->f00 = *(s32 *) (p + 4);
                a->f04 = *(s32 *) p;
                a->f08 = 0;
                a->f0c = p;
                return p;
            }
        }
    }
}
