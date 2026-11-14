/* Reconstructed from USA ARM9 at 0x020e41e8 (476 bytes).
 *
 * Network/message parser: builds a merged descriptor from the caller's two
 * structures, then walks the message with the SDK varint helpers until the
 * message type is finished.
 *
 * arg0 (a):  7 words of state, followed by the 0x54 byte tail
 *            (5 x 16-byte blocks plus one word).
 * arg1 (b):  6 words of state that overwrite the first 6 words of the local.
 * arg2:      passed through to func_020e48dc.
 *
 * The local descriptor is 0x88 bytes; the local work structure holds the
 * values of both switch cases (case 12 uses 0x10/0x14/0x18, case 15 uses
 * 0x00..0x0c).
 */
#include "m2c.h"

extern u8 func_020e335c(void *);
extern u8 func_020e3378(void *);
extern u8 *_Z22__DecodeUnsignedNumberPcPm(u8 *p, s32 *out);
extern u8 *_Z20__DecodeSignedNumberPcPl(u8 *p, s32 *out);
extern s32 func_020e48dc(void *, s32, void *);
extern s32 func_020e4104(void *, void *);
extern void func_020e4188(void *, void *, void *, void *);
extern void func_020e48a4(void);

typedef struct {
    u32 q0;
    u32 q1;
    u32 q2;
    u32 q3;
} E16;

typedef struct {
    E16 arr[5];
    u32 v;
} Tail;

typedef struct {
    s32 f00;
    s32 f04;
    u8 *f08;
    u8 *f0c;
    s32 f10;
    s32 f14;
    s32 f18;
    Tail t;
} A;

typedef struct {
    s32 f00;
    s32 f04;
    u8 *f08;
    u8 *f0c;
    s32 f10;
    s32 f14;
    s32 f18;
    s32 f1c;
    u8 *f20;
    u8 *f24;
    s32 f28;
    s32 f2c;
    u8 *f30;
    Tail t;
} S;

typedef struct {
    u32 f00;
    u32 f04;
    u32 f08;
    u32 f0c;
    u32 f10;
    u32 f14;
} B;

typedef struct {
    u32 f00;
    u32 f04;
    u32 f08;
    u8 *f0c;
    u32 f10;
    s32 f14;
    s32 f18;
} T;

u8 *func_020e41e8(A *a, B *b, void *arg2) {
    S s;
    T tmp;
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
            case 12: {
                u8 *p = s.f08;
                u8 *q;
                tmp.f10 = (u32) p[1] | ((u32) p[2] << 8) | ((u32) p[3] << 16) | ((u32) p[4] << 24);
                q       = _Z22__DecodeUnsignedNumberPcPm(p + 5, &tmp.f14);
                q       = _Z20__DecodeSignedNumberPcPl(q, &tmp.f18);
                if (func_020e48dc(a->f00, tmp.f10, arg2) != 0) {
                    goto out;
                }
                break;
            }
            case 15: {
                u8 *p = s.f08;
                u8 *q;
                q       = _Z22__DecodeUnsignedNumberPcPm(p + 1, &tmp.f00);
                q       = _Z22__DecodeUnsignedNumberPcPm(q, &tmp.f04);
                q       = _Z20__DecodeSignedNumberPcPl(q, &tmp.f08);
                tmp.f0c = q;
                if (func_020e4104(a->f00, &tmp) == 0) {
                    func_020e4188(a, b, &tmp, s.f08);
                }
                break;
            }
            case 1:
            case 14:
            default:
                func_020e48a4();
                goto out;
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
            case 13:
            case 16:
            case 17:
            case 18:
            case 19:
                break;
        }
        st = func_020e3378(&s);
    }
out:
    return s.f08;
}
