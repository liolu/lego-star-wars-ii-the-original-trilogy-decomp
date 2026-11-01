#include "m2c.h"

typedef struct {
    u16 v;
} HalfWord;

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

/* One-word aggregate passed by value as func_0201ad50's 4th and 5th arguments
 * (20.12 fixed-point values here; their exact role is not established). */
typedef struct {
    s32 v;
} UnkVal;

typedef struct {
    u8 x;
    u8 y;
} BytePos;

typedef struct {
    u16 h[3];
} Text3;

typedef struct {
    u8 pad00[0x08];
    s32 f08;
} Flag;

typedef struct {
    u8 pad00[0x08];
    HalfWord f08;
    u8 pad0a[0x0e];
} Entry18;

typedef struct {
    u8 pad00[0x28];
    u8 f28[8];
    u8 f30[8];
    u8 f38[0x2c];
    Flag *f64;
    Flag *f68[4];
    Flag *f78;
    u8 f7c[4];
    u8 f80;
    u8 pad81[3];
    Flag *f84[4];
    Flag *f94[4];
    s32 fa4;
    u8 pada8[4];
    Flag *fac[4];
    u16 fbc;
    u16 fbe;
    u8 padc0[9];
    u8 fc9[4];
    u8 fcd[4];
    u8 padd1[0x13];
    s32 fe4;
    s32 fe8;
    s32 fec;
} Obj;

extern const HalfWord data_020f76e4[];
extern const HalfWord data_020f76e8[];
extern const HalfWord data_020f76f4[];
extern const BytePos data_020f7700[];
extern const Text3 data_020f7708[];
extern Entry18 data_020ef57c[];
extern u16 data_020ffa28[];
extern M2C_UNK data_02101b98;
extern u8 data_021027f8[];
extern u8 data_02106d90[];
typedef struct {
    u8 pad00[0x4c];
    u8 *f4c[4];
} PlayerTab;

extern PlayerTab data_0210930c;
extern s32 data_027e02a4;
extern s32 data_027e02a8;

extern void func_02001688(s32);
extern void func_020016c0(void);
extern void func_0201ad50(void *, s32, void *, UnkVal, UnkVal, u8, void *);
extern void func_0201b318(void *, u8);
extern s32 func_02053b14(void *);
extern void func_020534f4(void *, s32, s32, HalfWord);
extern void func_02053ee8(void *, s32, s32);
extern void func_02053f44(void *, s32);
extern void func_02054c80(void *, void *, s32, s32, s32, s32, s32, s32, void *, Desc);
extern void func_020559f0(void);
extern void func_02055b24(s32, s32, s32, s32, s32);
extern void func_02055ba4(s32, s32, s32, s32, s32);
extern void func_0206d6cc(void *);
extern void func_0206d7fc(void *, s8);
extern void func_02072f30(s32, s32, s32, s32);
extern void func_0209bb34(void *);

enum {
    SLOT_COUNT = 4,
    FX_SHIFT   = 12,  /* 20.12 fixed point */
};

/* Screen setup with four per-slot rows: resets the buffer in data_02101b98,
 * clears the fields at 0xbc/0xbe/0xe8 and sets 0xec to 2, then sets up the
 * sprite at 0x28 (resource 0x47 or 0x46 chosen by the mode word at +0x34 of
 * the record pointed to by data_02106d90+0xad0; 0xe4 records which) and the
 * sprite at 0x30 (resource 0x45) plus the sub-object at 0x38. It then creates
 * the func_0201ad50 objects: one at 0x64, one at 0x78, and per slot one at
 * 0x68 (skipped for the slot in data_021027f8+0x2a), one at 0x94 (only when
 * data_0210930c has an entry for the slot; its table entry in data_020ef57c is
 * picked by that entry's byte at 0x6bb) and one at 0x84; finally four objects
 * at 0xac positioned from data_020f7700, each with bit 0 of +0x08 cleared.
 * Roles of the flag fields and of func_02055ba4/func_02055b24 are unknown. */
void func_02099e98(Obj *self) {
    Out out;
    Pair pair;
    Desc desc;
    Out out2;
    Out out3;
    UnkVal p3_1;
    UnkVal p4_1;
    UnkVal p3_2;
    UnkVal p4_2;
    UnkVal p3_3;
    UnkVal p4_3;
    UnkVal p3_4;
    UnkVal p4_4;
    UnkVal p3_5;
    UnkVal p4_5;
    UnkVal p3_6;
    UnkVal p4_6;
    u8 *state;
    u8 kind;
    u8 *player;
    u8 slot;
    s32 j;

    func_02053ee8(&data_02101b98, 0, 0);
    func_02053f44(&data_02101b98, 0x1000);
    self->fbc = 0;
    self->fbe = 0;
    if (data_027e02a8 & 0x40) {
        func_02072f30(1, 0xa4, 0x9c, 0);
        state = *(u8 **) (data_02106d90 + 0xad0);
        if (state[0x50] == 0) {
            func_0209bb34(state);
        }
    }
    self->fe8 = 0;
    self->fec = 2;
    func_020534f4(self->f28, 1, 0, data_020f76e4[1]);
    switch (*(s32 *) (*(u8 **) (data_02106d90 + 0xad0) + 0x34)) {
        case 0:
        case 2: {
            pair.a = 0;
            pair.b = 0;
            desc.a = 0;
            desc.b = 0;
            desc.c = 0;
            desc.d = 0;
            func_02054c80(&out, self->f28, 0x47, 0, 0, 0, 0, 0x100, &pair, desc);
            self->fe4 = 1;
            break;
        }
        case 1:
        case 3:
        case 4: {
            pair.a = 0;
            pair.b = 0;
            desc.a = 0;
            desc.b = 0;
            desc.c = 0;
            desc.d = 0;
            func_02054c80(&out2, self->f28, 0x46, 0, 0, 0, 0, 0x100, &pair, desc);
            self->fe4 = 0;
            break;
        }
    }
    func_020534f4(self->f30, 1, 1, data_020f76e4[0]);
    pair.a = 0;
    pair.b = 0;
    desc.a = 0;
    desc.b = 0;
    desc.c = 0;
    desc.d = 0;
    func_02054c80(&out3, self->f30, 0x45, 0, 0, 0, 0, 0x100, &pair, desc);
    func_0206d6cc(self->f38);
    func_0206d7fc(self->f38, 1);
    func_020559f0();
    func_02055b24(data_020ffa28[data_021027f8[0x2a]], 0x80, 0x40, 0xb0, 1);
    self->fa4 = func_02053b14(self->f30);

    p4_1.v = 0x4c000;
    p3_1.v = 0xac000;
    func_0201ad50(&self->f64, 1, &data_020f76f4[3], p3_1, p4_1, 0, (void *) data_027e02a4);
    self->f80 = 0;
    p4_2.v    = 0;
    p3_2.v    = 0;
    func_0201ad50(&self->f78, 3, &data_020f76e8[0], p3_2, p4_2, 1, (void *) data_027e02a4);
    func_0201b318(self->f78, 4);

    p3_5.v = 0x8000;
    for (slot = 0; slot < SLOT_COUNT; slot++) {
        self->f7c[slot] = 0;
        self->fc9[slot] = slot;
        self->fcd[slot] = 0;
        if (slot == data_021027f8[0x2a]) {
            self->f68[slot] = 0;
        } else {
            p4_3.v = 0;
            p3_3.v = 0;
            func_0201ad50(&self->f68[slot], 2, &data_020f76f4[0], p3_3, p4_3, 1, (void *) data_027e02a4);
            func_0201b318(self->f68[slot], 4);
        }
        func_02001688(0);
        kind   = 0;
        player = data_0210930c.f4c[slot];
        if (player != 0) {
            kind = player[0x6bb];
        }
        func_020016c0();
        if (player != 0) {
            p3_4.v = 0x8000;
            p4_4.v = (slot * 0x30 + 8) << FX_SHIFT;
            func_0201ad50(&self->f94[slot], slot + 8, &data_020ef57c[kind].f08, p3_4, p4_4, 0, (void *) data_027e02a4);
        } else {
            self->f94[slot] = 0;
        }
        if (slot < 2) {
            func_02055ba4(0xf52, slot << 5, (slot + 4) << 4, 0x10, 1);
        } else {
            func_02055ba4(0xf52, (slot + 1) << 5, (slot + 4) << 4, 0x10, 1);
        }
        p4_5.v = (slot * 0x30 + 8) << FX_SHIFT;
        func_0201ad50(&self->f84[slot], slot + 4, &data_020f76e8[3], p3_5, p4_5, 1, (void *) 1);
    }

    for (j = 0; j < SLOT_COUNT; j++) {
        p4_6.v = data_020f7700[j].y << FX_SHIFT;
        p3_6.v = data_020f7700[j].x << FX_SHIFT;
        func_0201ad50(&self->fac[j], j + 12, &data_020f7708[j], p3_6, p4_6, 0, (void *) 1);
        self->fac[j]->f08 &= ~1;
    }
}
