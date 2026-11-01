#include "m2c.h"

typedef struct {
    s32 v;
} Fx;

typedef struct {
    void *vtable;
    u8 pad04[4];
    u32 flags;
    u8 pad0c[0x40];
    s32 f4c;
    s32 f50;
    u8 pad54[0x44];
    void *f98;
    s32 f9c;
    s32 fa0;
    u8 fa4;
    u8 fa5;
    u8 fa6;
} Sprite;

typedef struct {
    void *vtable;
    void *f04;
    u16 f08;
    void *f0c;
    void *f10;
    s32 f14;
    u8 pad18[4];
    s32 f1c;
    s32 f20;
    Sprite *f24;
    Sprite *f28;
    Sprite *f2c;
    u8 f30[0x60];
} Obj;

extern void *func_0205e80c(s32 size, s32 heap, s32 flags);
extern void *func_0201c1f8(void *self, s32 arg1);
extern void func_02060cd0(void *self, u32 arg1, u32 arg2);
extern void clearDataAndInstructionCache(void *self, void *data);
extern void func_0201c478(void *self, Fx a, Fx b, Fx c, Fx d, ...);
extern void func_020e46bc(void *arr, s32 count, s32 size, void *ctor, void *dtor);
extern void func_02065600(void *self, s32 arg1);
extern void func_0206574c(void *self, s32 arg1, s32 arg2);
extern s32 func_02065be8(void *self, s32 arg1, s32 arg2, s32 arg3, void *arg4, s32 arg5);
extern void *func_02066708(s32 id);
extern void func_020852c8(Obj *self);

extern u8 data_020fe77c[];
extern u8 data_020ff3e0[];
extern u8 data_020fd198[];
extern const u16 data_020ee2a0[];
extern u8 data_020ee2c8[];
extern u8 data_020f0240[];
extern s32 data_027e02a8;
extern u8 func_0206afa4[];
extern u8 func_02064a34[];

enum {
    SPRITE_SIZE       = 0xa8,
    OBJ_SLOT_COUNT    = 8,
    OBJ_SLOT_SIZE     = 0xc,
    SYS_FLAG_0200     = 0x200,
    SPRITE_FLAG_BIT31 = 0x80000000,
};

/* Wraps one fixed-point (20.12) word; func_0201c478 takes four of them by value. */
static inline Fx fx(s32 v) {
    Fx r;
    r.v = v;
    return r;
}

/*
 * Allocates and initialises one 0xa8-byte child object: base init
 * (func_0201c1f8), vtable data_020fd198, data pointer at +0x98, the fields
 * +0x9c..+0xa6, then func_02060cd0(p, mode, flag) where flag is 1 only when
 * bit 0x200 of data_027e02a8 and bit 31 of p->flags are both clear, and
 * finally clearDataAndInstructionCache(p, data).  NULL if allocation fails.
 */
static inline Sprite *sprite_new(void *data, s32 f9c, u32 mode) {
    Sprite *p = func_0205e80c(SPRITE_SIZE, 0, 4);
    s32 flag;
    if (p != 0) {
        func_0201c1f8(p, 1);
        p->vtable = data_020fd198;
        p->f98    = data;
        p->f9c    = f9c;
        p->fa0    = 0;
        p->fa4    = 1;
        p->fa5    = 0;
        p->fa6    = 0;
        flag      = 0;
        if (!(data_027e02a8 & SYS_FLAG_0200) && !(p->flags & SPRITE_FLAG_BIT31)) {
            flag = 1;
        }
        func_02060cd0(p, mode, flag);
        clearDataAndInstructionCache(p, data);
    }
    return p;
}

/*
 * Initialiser for a 0x90-byte object (the caller func_0208ea58 allocates it and
 * passes file, file->unk_3c, file->unk_34).  Stores the base vtable
 * data_020fe77c, then the final vtable data_020ff3e0, the three arguments,
 * builds an 8 x 0xc array at +0x30 (func_020e46bc), creates three child
 * objects at +0x24/+0x28/+0x2c, each with a fixed-point position at
 * +0x4c/+0x50 and four fixed-point values passed to func_0201c478, sets up
 * the object at +0x0c (func_02065600 / func_0206574c / func_02065be8 with
 * string 7; result kept at +0x14) and refreshes via func_020852c8.
 */
Obj *func_02084c04(Obj *self, void *arg1, void *arg2, void *arg3) {
    Sprite *p;

    self->vtable = data_020fe77c;
    self->f04    = arg1;
    self->f08    = 0;
    self->vtable = data_020ff3e0;
    self->f0c    = arg2;
    self->f10    = arg3;
    self->f1c    = 0;
    self->f20    = 0;
    func_020e46bc(self->f30, OBJ_SLOT_COUNT, OBJ_SLOT_SIZE, func_02064a34, func_0206afa4);

    p              = sprite_new((void *) &data_020ee2a0[0x11], 0, 0);
    self->f24      = p;
    self->f24->fa6 = 1;
    self->f24->f4c = 0xa8000;
    self->f24->f50 = 0xb0000;
    func_0201c478(self->f24, fx(-0xf000), fx(-0xf000), fx(0xf000), fx(0));

    p              = sprite_new(data_020ee2c8, 0, 0);
    self->f28      = p;
    self->f28->fa6 = 1;
    self->f28->f4c = 0x58000;
    self->f28->f50 = 0xa8000;
    func_0201c478(self->f28, fx(-0xf000), fx(0), fx(0xf000), fx(0xf000));

    p              = sprite_new(data_020f0240, 1, 1);
    self->f2c      = p;
    self->f2c->fa6 = 1;
    self->f2c->f4c = 0xdc000;
    self->f2c->f50 = 0x9c000;
    func_0201c478(self->f2c, fx(0), fx(0), fx(0x20000), fx(0x20000));

    func_02065600(self->f0c, 4);
    func_0206574c(self->f0c, 6, 0);
    self->f14 = func_02065be8(self->f0c, 1, 0x80, 8, func_02066708(7), 1);
    func_02065600(self->f0c, 3);
    func_0206574c(self->f0c, 0, 0);
    func_020852c8(self);
    return self;
}
