#include "m2c.h"

typedef struct {
    u8 pad0[0x34];
    s32 f34;
    s32 pad38;
    s32 f3c;
    u8 f40;
} Mgr;

typedef struct {
    u16 v;
} HalfWord;

typedef struct {
    s32 v;
} UnkVal;

typedef struct {
    s32 x;
    UnkVal y;
} Pos;

typedef struct {
    u8 pad0[0x28];
    u8 f28[0x8];
    u8 f30[0x8];
    u8 f38[0x30];
    u8 f68[0x2c];
    void *f94[3];
    void *fa0[3];
    u8 fac;
    u8 padad[3];
    s32 fb0;
    u8 fb4;
    u8 fb5;
} Obj;

typedef struct {
    s16 a;
    s16 b;
} Pair;

typedef struct {
    u8 a;
    u8 b;
    u8 c;
    u8 d;
} Desc;

typedef struct {
    u32 a;
    u32 b;
} Out;

/* Mode values of the manager byte at 0x40 and the resource ids started for them on the
 * sub-object at 0x30 (same switch as func_0209dd64), plus the 20.12 fixed point shift used
 * for the by-value word positions handed to func_0201ad50 (see func_02072f30). */
enum {
    MGR_MODE_4    = 4,
    MGR_MODE_5    = 5,
    RES_ID_MODE_4 = 0xbb2,
    RES_ID_MODE_5 = 0xebe,
    RES_FLAGS     = 0x100,
    FX32_SHIFT    = 12,
};

extern Mgr *data_02107860;
extern s32 data_02101b98;
extern s32 data_02101b78;
extern s32 data_027e0874;
extern s32 data_027e02a4;
extern const HalfWord data_020f7794[];
extern char data_020f7798[];

extern void func_02053ee8(void *, s32, s32);
extern void func_02053f44(void *, s32);
extern void func_02001688(s32);
extern void func_020ac278(void *, s32, s32);
extern void func_020016c0(void);
extern void func_020534f4(void *, s32, s32, HalfWord);
extern void func_0205365c(void *, s32);
extern void func_02065600(void *, s32);
extern void func_02065658(void *, void *);
extern void func_0206574c(void *, s32, s32);
extern void func_0206653c(s32, s32, s32, s32, s32, s32, s32);
extern void func_02054c80(void *, void *, s32, s32, s32, s32, s32, s32, void *, Desc);
extern void func_02055a18(s32, s32, s32, s32, s32, s32);
extern void func_0206d6cc(void *);
extern void func_0206d7fc(void *, s8);
extern void func_02072f30(s32, s32, s32, void *);
extern void func_0201ad50(void *, s32, void *, UnkVal, UnkVal, u8, s32);
extern void func_0209e1ac(Obj *);

/* Sets up the widget: resets the two objects at data_02101b98 / data_02101b78 (scale 0x1000),
 * reinitialises the screen block at data_027e0874, prepares the sub-objects at 0x28, 0x30, 0x38
 * and 0x68, starts the mode's resource on 0x30 when the manager mode (byte 0x40) is 4 or 5,
 * creates two objects per row for three rows (stored in 0x94[i] and 0xa0[i]; the first
 * position word is 32 or 199, the second 31 + 40 * i, both in 20.12 fixed point), then caches
 * the manager state (0x3c -> 0xb0, 0x34 -> 0xb4, 0x40 -> 0xb5), clears 0xac and redraws the
 * text block through func_0209e1ac. */
void func_0209da24(Obj *self) {
    Out out5;
    Pair pair5;
    Desc desc5;
    Out out4;
    Pair pair4;
    Desc desc4;
    UnkVal firstA;
    UnkVal secondA;
    UnkVal firstB;
    UnkVal secondB;
    u8 i;
    s32 secondFx;
    Mgr **pp;

    func_02053ee8(&data_02101b98, 0, 0);
    func_02053f44(&data_02101b98, 0x1000);
    func_02053ee8(&data_02101b78, 0, 0);
    func_02053f44(&data_02101b78, 0x1000);
    func_02001688(0);
    func_020ac278(&data_027e0874, 4, 0);
    func_020016c0();
    func_020534f4(self->f28, 1, 0, data_020f7794[1]);
    func_0205365c(self->f28, 0);
    func_02065600(self->f38, 4);
    func_02065658(self->f38, self->f28);
    func_0206574c(self->f38, 3, -1);
    func_0206653c(1, 0, 1, 3, 7, 7, 0);
    func_020534f4(self->f30, 0, 1, data_020f7794[0]);
    func_0205365c(self->f30, 0);
    pp = &data_02107860; // FAKE: manager pointer address held across the loop; spilled, so its literal is reloaded at each use
                         // (3x in the epilogue)
    switch ((*pp)->f40) {
        case MGR_MODE_5:
            pair5.a = 0;
            pair5.b = 0;
            desc5.a = 0;
            desc5.b = 0;
            desc5.c = 0;
            desc5.d = 0;
            func_02054c80(&out5, self->f30, RES_ID_MODE_5, 0, 0, 0, 0, RES_FLAGS, &pair5, desc5);
            break;
        case MGR_MODE_4:
            pair4.a = 0;
            pair4.b = 0;
            desc4.a = 0;
            desc4.b = 0;
            desc4.c = 0;
            desc4.d = 0;
            func_02054c80(&out4, self->f30, RES_ID_MODE_4, 0, 0, 0, 0, RES_FLAGS, &pair4, desc4);
            break;
    }
    func_02055a18(0x8f9, 4, 3, 3, 6, 0xf);
    func_0206d6cc(self->f68);
    func_0206d7fc(self->f68, 1);
    func_02072f30(data_027e02a4, 0xd9, 0x9f, 0);
    for (i = 0; i < 3; i++) {
        secondFx  = (i * 0x28 + 0x1f) << FX32_SHIFT;
        firstA.v  = 0x20000;
        secondA.v = secondFx;
        func_0201ad50(&self->f94[i], 1, data_020f7798, firstA, secondA, 0, data_027e02a4);
        firstB.v  = 0xc7000;
        secondB.v = secondFx;
        func_0201ad50(&self->fa0[i], 1, data_020f7798 + 6, firstB, secondB, 0, data_027e02a4);
    }
    self->fb0 = (*pp)->f3c;
    self->fb4 = (*pp)->f34;
    self->fb5 = (*pp)->f40;
    self->fac = 0;
    func_0209e1ac(self);
}
