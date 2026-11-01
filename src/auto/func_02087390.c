#include "m2c.h"

typedef struct {
    u16 v;
} HalfWord;

typedef struct {
    HalfWord a;
    HalfWord b;
} HalfPair;

typedef struct {
    s16 f00;
    s16 f02;
} ShortPair;

typedef struct {
    u8 f00;
    u8 f01;
    u8 f02;
    u8 f03;
} Byte4;

typedef struct {
    u8 f00[8];
} Byte8;

typedef struct {
    s32 v;
} UnkVal;

typedef struct {
    u8 pad00[8];
    u32 flags;
    u8 pad0c[0xc];
    u8 b18;
    u8 b19;
} Sprite;

typedef struct {
    u8 pad00[0x28];
    u8 f28[8];
    u8 f30[8];
    u8 f38[0x30];
    u8 f68[0x30];
    u8 f98[0x2c];
    void *fc4;
    u8 fc8;
    u8 fc9;
    u8 fca;
    u8 pad_cb[5];
    Sprite *fd0;
    Sprite *fd4;
    s32 fd8;
    s32 fdc;
    s32 fe0;
    u8 pad_e4[4];
    u8 fe8[4];
} Obj;

extern const HalfPair data_020f0248;
extern char data_020f024c[];
extern M2C_UNK data_02101b98;
extern M2C_UNK data_02104d94;
extern M2C_UNK data_02104dc4;
extern M2C_UNK data_0210930c;

extern void func_0201ad50(void *p0, s32 p1, void *p2, UnkVal p3, UnkVal p4, u8 p5, s32 p6);
extern void func_020534f4(void *self, s32 a, s32 b, HalfWord h);
extern void func_0205365c(void *a0, s32 a1);
extern void func_020536a8(void *arg0, s32 arg1);
extern s32 func_02053b14(void *arg0);
extern s32 func_02053c98(void *arg0, s32 arg1, s32 arg2);
extern void func_02053ee8(void *arg0, s32 arg1, s32 arg2);
extern void func_02053f44(void *arg0, s32 arg1);
extern void func_02054c80(void *, void *, u32, u32, u32, u32, u32, u32, ShortPair *, Byte4);
extern void func_02055a18(s32 a0, s32 a1, s32 a2, s32 a3, s32 a4, s32 a5);
extern void *func_0205e80c(s32 size, s32 flag, s32 align);
extern void func_02065600(void *arg0, s32 arg1);
extern void func_02065658(void *arg0, void *arg1);
extern void func_0206574c(void *arg0, s32 arg1, s32 arg2);
extern void *func_020682c4(void *arg0, void *arg1, void *arg2, void *arg3, s32 arg4, s32 arg5, u8 arg6);
extern void func_0206d6cc(void *self);
extern void func_0206d7fc(void *arg0, s8 arg1);
extern void func_02072f30(s32 blockArg, s32 first, s32 second, s32 entry);
extern void func_02072f94(void);
extern s32 func_0208559c(void *arg0);
extern void func_02087ce4(void *arg0, s32 arg1, s32 arg2);
extern void func_02087e14(void *self);
extern void func_02088198(void *self);
extern void func_0208830c(void *s, void *p, s32 v);
extern void func_0208837c(void *s, s32 a, s32 b);
extern s32 func_0208c77c(void *arg0, u32 arg1);
extern u8 func_020917b4(void *arg0);
extern u8 func_02093fa8(s32 arg0);
extern s32 func_02094038(s32 arg0);

static inline void sprite_set_flag(Sprite *s, s32 on) {
    if (on) {
        s->flags |= 1;
    } else {
        s->flags &= ~1;
    }
}

/* Set-up method of the type 0x30 object (its vtable is data_020ff414; the
 * constructor is func_020872b4).  Steps:
 *  - resets data_02101b98 (func_02053ee8 / func_02053f44 with 0x1000);
 *  - reads a byte from data_0210930c via func_020917b4: fc8 is set when
 *    func_02093fa8 maps it to 1 or 2, fca when it lies in 9..14;
 *  - initialises the records at +0x30 and +0x28 with the two halfwords of
 *    data_020f0248 (second halfword for +0x30, first for +0x28), the
 *    component at +0x98, and the blocks at +0x38 / +0x68 bound to +0x28;
 *  - creates the two objects stored at +0xD0 and +0xD4 through func_0201ad50
 *    (strings at data_020f024c and data_020f024c + 6), sets their bytes
 *    0x18/0x19 to 3, bit 3 of their word at +8, and bit 0 from fca (and, for
 *    +0xD4, from func_0208c77c);
 *  - allocates a 0x38-byte object (func_0205e80c) built by func_020682c4 and
 *    stores it (or NULL) at +0xC4, then calls func_02072f30 / func_02072f94. */
void func_02087390(Obj *self) {
    Byte8 out;
    ShortPair pair;
    Byte4 bytes;
    UnkVal x1;
    UnkVal y1;
    UnkVal x2;
    UnkVal y2;
    HalfPair ids;
    u8 kind;
    void *p;
    Sprite *s;

    ids.b = data_020f0248.b;
    ids.a = data_020f0248.a;
    func_02053ee8(&data_02101b98, 0, 0);
    func_02053f44(&data_02101b98, 0x1000);
    kind = func_020917b4(&data_0210930c);
    if (func_02093fa8(kind) == 1 || func_02093fa8(kind) == 2) {
        self->fc8 = 1;
    }
    if (kind >= 9 && kind <= 14) {
        self->fca = 1;
    }
    func_020534f4(self->f30, 1, 0, ids.b);
    func_02053c98(self->f30, 0, 0);
    func_0208830c(self->fe8, self->f30, 0x5a);
    bytes.f00 = 0;
    bytes.f01 = 0;
    bytes.f02 = 0;
    bytes.f03 = 0;
    pair.f00  = 0;
    pair.f02  = 0;
    func_02054c80(&out, self->f30, 0xb05, 0, 1, 0, 0, 0x30, &pair, bytes);
    self->fd8 = func_02053b14(self->f30);
    func_0206d6cc(self->f98);
    func_0206d7fc(self->f98, 1);
    func_020534f4(self->f28, 1, 3, ids.a);
    func_0205365c(self->f28, 0);
    func_020536a8(self->f28, 0);
    func_02065600(self->f38, 2);
    func_02065658(self->f38, self->f28);
    func_0206574c(self->f38, 0, 0);
    func_02065600(self->f68, 4);
    func_02065658(self->f68, self->f28);
    func_0206574c(self->f68, 0, 0);
    func_02088198(self);
    func_02055a18(0x8f9, 4, 3, 3, 6, 0xf);
    self->fe0 = func_02094038(M2C_FIELD(M2C_FIELD(&data_0210930c, u8 **, 0x68), u8 *, 0x47a));
    self->fdc = func_0208559c(&data_02104d94);

    y1.v = 0;
    x1.v = 0x8000;
    func_0201ad50(&self->fd0, 0, data_020f024c, x1, y1, 0, 1);
    s      = self->fd0;
    s->b18 = 3;
    s->b19 = 3;
    self->fd0->flags |= 8;
    s = self->fd0;
    if (self->fca == 0) {
        s->flags |= 1;
    } else {
        s->flags &= ~1;
    }

    y2.v = 0x8000;
    x2.v = 0xa0000;
    func_0201ad50(&self->fd4, 1, data_020f024c + 6, x2, y2, 0, 1);
    self->fd4->flags |= 8;
    s      = self->fd4;
    s->b18 = 3;
    s->b19 = 3;
    if (self->fca == 0) {
        s32 on = func_0208c77c(&data_02104dc4, M2C_FIELD(M2C_FIELD(&data_0210930c, u8 **, 0x68), u8 *, 0x47a));
        sprite_set_flag(self->fd4, on);
        func_02087ce4(self, 5, 1);
        func_02087e14(self);
    } else {
        sprite_set_flag(self->fd4, 0);
    }
    func_0208837c(self->fe8, 1, self->fc8);
    p = func_0205e80c(0x38, 3, 4);
    if (p != 0) {
        p = func_020682c4(p, self, self->f28, self->f68, -1, -1, self->fc8);
    }
    self->fc4 = p;
    func_02072f30(1, 0x80, 6, 2);
    func_02072f94();
}
