/* Reconstructed from USA ARM9 at 0x020e4420 (236 bytes).
 *
 * Message parser: looks the message type up in the exception table, decodes
 * the header of the message and hands the result to the dispatcher.
 *  - func_020e3254() fills the local descriptor from the table at self+0x10,
 *  - func_020e4ae8() decodes the flags of the message,
 *  - func_020e3fb0() / func_020e41e8() walk the message,
 *  - func_020e4bd8() finally jumps to the handler address.
 * The four word local at sp+0 keeps its address taken (it is passed to
 * func_020e41e8), so the store of the 32 bit header value is kept.
 */
#include "m2c.h"

extern void func_020e3254(s32 r4, void *sb);
extern void func_020e48a4(void);
extern void func_020e4ae8(void *self, void *arg1);
extern u8 *func_020e3fb0(void *a, void *b);
extern u8 *func_020e41e8(void *a, void *b, void *arg2);
extern u8 *_Z22__DecodeUnsignedNumberPcPm(u8 *p, s32 *out);
extern u8 *_Z20__DecodeSignedNumberPcPl(u8 *p, s32 *out);
extern void func_020e3700(void *self, void *sb, u8 *p);
extern void func_020e43c4(u32 *p, s32 a, s32 b);
extern void func_020e4bd8(void *self, void *sb, void *target);

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
    u8 *f18;
    Tail t;
} A;

typedef struct {
    s32 f00;
    u8 *f04;
    s32 f08;
    void *f0c;
    void *f10;
    s32 f14;
} B;

typedef struct {
    s32 v00;
    s32 v04;
    s32 v08;
    s32 v0c;
} Loc;

void func_020e4420(A *self) {
    B sb;
    Loc loc;
    u8 *p;

    func_020e3254(self->f10, &sb);
    if (sb.f04 == NULL) {
        func_020e48a4();
    }
    func_020e4ae8(self, &sb);
    if (self->f00 != 0) {
        self->f0c = NULL;
    } else {
        self->f0c = func_020e3fb0(self, &sb);
        if (self->f0c == NULL) {
            func_020e48a4();
        }
    }
    p       = func_020e41e8(self, &sb, &loc.v00);
    loc.v04 = (u32) p[1] | ((u32) p[2] << 8) | ((u32) p[3] << 16) | ((u32) p[4] << 24);
    _Z20__DecodeSignedNumberPcPl(_Z22__DecodeUnsignedNumberPcPm(p + 5, &loc.v08), &loc.v0c);
    func_020e3700(self, &sb, p);
    func_020e43c4((u32 *) self, loc.v0c, loc.v00);
    func_020e4bd8(self, &sb, (void *) (sb.f00 + loc.v08));
}
