#include "m2c.h"

typedef struct {
    u16 v;
} HalfWord;

typedef struct {
    s16 a;
    s16 b;
} ShortPair;

typedef struct {
    u8 a;
    u8 b;
    u8 c;
    u8 d;
} Quad;

typedef struct {
    s32 a;
    s32 b;
} Out;

typedef struct {
    void *obj;
    s16 x;
    s16 y;
    s16 f8;
    s16 fa;
    s16 fc;
    s16 fe;
    Quad q;
    u8 f14;
} Elem;

typedef struct {
    u8 pad0[2];
    u8 enabled;
    u8 pad3[0x21];
} Entry24;

typedef struct {
    u8 pad00[0x14];
    u32 flags;
} Sub;

typedef struct {
    u8 pad00[0x28];
    s32 f28[8];
    u8 spr48[8];
    u8 spr50[8];
    u8 spr58[8];
    u8 f60[8];
    u8 f68[8];
    u8 f70[0x30];
    Sub *fa0;
    u8 fa4[6];
    u8 pad_aa[2];
    Elem *elems[5];
    u8 fc0[0x20];
    s32 fe0;
    s32 fe4;
    s32 fe8;
    s32 fec[5];
    s32 f100;
    s32 f104;
    s32 f108;
    s32 f10c[6];
    s32 f124;
    s32 f128;
    s32 f12c;
    u8 f130;
    u8 pad_131[3];
    u8 *lists[5];
    s32 counts[5];
    u8 *list2;
    s32 count2;
} Obj;

extern void func_02053ee8(void *, s32, s32);
extern void func_02053f44(void *, s32);
extern void func_020534f4(void *, s32, s32, HalfWord);
extern void func_02054c80(void *, void *, s32, s32, s32, s32, s32, s32, ShortPair *, Quad);
extern void func_0205365c(void *, s32);
extern void func_02065600(void *, s32);
extern void func_02065658(void *, void *);
extern void func_0206574c(void *, s32, s32);
extern void func_0206653c(s32, s32, s32, s32, s32, s32, s32);
extern void func_02067be4(void *, s32, s32, s32, u8, u8, u8);
extern void func_02067c68(void *);
extern void func_020540b4(void *, s16, s16);
extern void *func_0205e80c(s32, s32, s32);
extern void *func_0205e818(s32, s32, s32);
extern void *func_0201abbc(void *, s32);
extern void clearDataAndInstructionCache(void *, const void *);
extern s32 func_0207fcc0(void);
extern void *func_0205e624(void *, u32, u32);
extern u8 *func_0208dad8(u16);
extern s32 func_020865dc(void *, s32);
extern s32 func_02086630(void *, s32);
extern s32 func_020832d0(void *, s32, s32);
extern void func_02082b88(void *, s32, s32);
extern s32 func_02083358(void *, s32);
extern void func_02082ef0(void *, s32);
extern void func_02081de4(void *, s32, s32);
extern void func_02081d00(void *, s32);
extern void func_020831b0(void *);

extern M2C_UNK data_02101b98;
extern const HalfWord data_020f0174[];
extern const u8 data_020f01fc[];
extern const u8 data_020f0204[];
extern u8 data_02104de2[];
extern M2C_UNK data_02104d90;
extern const Entry24 data_020f6ba8[];
extern ShortPair data_020ff2c0[];

/* Fill one 0x18-byte element (sprite source, cell x/y, four extra halfwords, a
 * byte rectangle) and register it with func_02054c80 under id 0x59e. */
static inline void elem_init(Elem *e, void *obj, s16 x, s16 y, s16 f8, s16 fa, s16 fc, s16 fe, Quad q) {
    Quad sq;
    ShortPair pair;
    Out out;

    e->obj = obj;
    e->x   = x;
    e->y   = y;
    e->f8  = f8;
    e->fa  = fa;
    e->fc  = fc;
    e->fe  = fe;
    e->q.a = q.a;
    e->q.b = q.b;
    e->q.c = q.c;
    e->q.d = q.d;
    e->f14 = 0;
    sq.a   = e->f8;
    sq.b   = e->fa;
    sq.c   = e->fc;
    sq.d   = e->fe;
    pair.a = e->x;
    pair.b = e->y;
    func_02054c80(&out, e->obj, 0x59e, 0x10d, 1, 0x10, 0, 0, &pair, sq);
}

enum {
    REG_BG0CNT_ADDR = 0x04000008, /* NDS engine A BG0 control register */
    FA4_NONE        = 0xfa,              /* entry value that skips the setup call */
};

/* Third entry of the function table data_020ff320 (after func_0208341c and
 * func_020833ec). Initialises the object in steps:
 *   1. graphics: func_02053ee8/func_02053f44 on data_02101b98, BG0 priority 1,
 *      sprite sources at 0x48/0x50/0x58 registered with func_02054c80, the 0x70
 *      block bound to the 0x58 source, and the 0x60/0x68 blocks set up;
 *   2. allocates the 0x80-byte buffer at 0xa0 and sets its flag bits 10-11 to 1;
 *   3. copies a 6-byte row of data_02104de2 (row picked by func_0207fcc0);
 *   4. builds five id lists (ranges from data_020f0204/data_020f01fc) and one
 *      0x28-entry list (data_020f6ba8), keeping ids that pass the callee checks;
 *   5. applies the copied row entries, allocates five elements, then clears
 *      counters and the 8-byte array at 0x28. */
void func_02080274(Obj *self) {
    Out out1;
    ShortPair pair1;
    Quad q1;
    Out out2;
    ShortPair pair2;
    Quad q2;
    void *p;
    s32 i;
    s32 j;
    u8 *row;
    s32 k;

    func_02053ee8(&data_02101b98, 0, 1);
    func_02053f44(&data_02101b98, 0x1100);
    *(volatile u16 *) REG_BG0CNT_ADDR = (*(volatile u16 *) REG_BG0CNT_ADDR & ~3) | 1;
    func_020534f4(self->spr48, 1, 3, data_020f0174[1]);
    pair1.a = 0;
    pair1.b = 0;
    q1.a    = 0;
    q1.b    = 0;
    q1.c    = 0x20;
    q1.d    = 0x18;
    func_02054c80(&out1, self->spr48, 0x59d, 0x100, 1, 0x10, 0, 0xf0, &pair1, q1);

    func_020534f4(self->spr50, 1, 2, data_020f0174[2]);
    pair2.a = 0;
    pair2.b = 0;
    q2.a    = 0;
    q2.b    = 0;
    q2.c    = 0x20;
    q2.d    = 0x18;
    func_02054c80(&out2, self->spr50, 0x59e, 0x101, 1, 0x10, 0, 0, &pair2, q2);

    func_020534f4(self->spr58, 1, 1, data_020f0174[0]);
    func_0205365c(self->spr58, 0);
    func_02065600(self->f70, 4);
    func_02065658(self->f70, self->spr58);
    func_0206574c(self->f70, 0, 0);
    func_0206653c(0, 3, 7, 7, 7, 7, 0);
    func_02067be4(self->f60, 1, 0, 0x20, 0, 0x80, 0x38);
    func_02067be4(self->f68, 1, 1, 0x20, 0xa8, 0x80, 0xc0);
    func_02067c68(self->f60);
    func_02067c68(self->f68);
    func_020540b4(&data_02101b98, 0x3e3e, 0x3f);

    p = func_0205e80c(0x80, 0, 4);
    if (p != NULL) {
        p = func_0201abbc(p, 1);
    }
    self->fa0 = p;
    clearDataAndInstructionCache(p, &data_020f0174[3]);
    self->fa0->flags = (self->fa0->flags & ~0xc00) | 0x400;

    row          = &data_02104de2[func_0207fcc0() * 6];
    self->fa4[0] = row[0];
    self->fa4[1] = row[1];
    self->fa4[2] = row[2];
    self->fa4[3] = row[3];
    self->fa4[4] = row[4];
    self->fa4[5] = row[5];

    j = 0;
    for (i = 0; i < 5; i++) {
        self->lists[i] = func_0205e818(data_020f01fc[i] - data_020f0204[i] + 1, 3, 4);
        func_0205e624(self->lists[i], 0, data_020f01fc[i] - data_020f0204[i] + 1);
        self->counts[i] = 0;
        for (; j <= data_020f01fc[i]; j++) {
            if (func_0208dad8(j)[0x18] != 0 && func_020865dc(&data_02104d90, j) != 0) {
                self->lists[i][self->counts[i]++] = j;
            }
        }
    }

    k           = 0;
    self->list2 = func_0205e818(0x28, 3, 4);
    func_0205e624(self->list2, 0, 0x28);
    self->count2 = 0;
    for (; k <= 0x28; k++) {
        if (data_020f6ba8[k].enabled != 0 && func_02086630(&data_02104d90, k) != 0) {
            self->list2[self->count2++] = k;
        }
    }

    self->fe4  = 0;
    self->fe0  = 0;
    self->f130 = 1;
    for (i = 0; i < 5; i++) {
        self->fec[i] = -1;
    }
    if (self->fa4[0] != FA4_NONE) {
        func_02082b88(self, 0, func_020832d0(self, 0, self->fa4[0]));
    }
    if (self->fa4[1] != FA4_NONE) {
        func_02082b88(self, 2, func_020832d0(self, 2, self->fa4[1]));
    }
    if (self->fa4[2] != FA4_NONE) {
        func_02082b88(self, 1, func_020832d0(self, 1, self->fa4[2]));
    }
    if (self->fa4[3] != FA4_NONE) {
        func_02082b88(self, 3, func_020832d0(self, 3, self->fa4[3]));
    }
    if (self->fa4[4] != FA4_NONE) {
        func_02082b88(self, 4, func_020832d0(self, 4, self->fa4[4]));
    }
    self->f100 = -1;
    self->fe8  = 0;
    func_02082ef0(self, func_02083358(self, self->fa4[5]));
    for (i = 0; i < 6; i++) {
        func_02081de4(self, i, 0);
    }

    for (i = 0; i < 5; i++) {
        Elem *e;

        if (i < 2) {
            e = func_0205e80c(sizeof(Elem), 3, 4);
            if (e != NULL) {
                Quad q;
                q.a = data_020ff2c0[i].a * 8;
                q.b = data_020ff2c0[i].b * 8 - 4;
                q.c = 0x20;
                q.d = 0x18;
                elem_init(e, self->spr58, data_020ff2c0[i].a, data_020ff2c0[i].b, i * 8, 0x1e, 4, 2, q);
            }
            self->elems[i] = e;
        } else {
            e = func_0205e80c(sizeof(Elem), 3, 4);
            if (e != NULL) {
                Quad q;
                q.a = data_020ff2c0[i].a * 8 - 4;
                q.b = data_020ff2c0[i].b * 8 - 4;
                q.c = 0x28;
                q.d = 0x28;
                elem_init(e, self->spr50, data_020ff2c0[i].a, data_020ff2c0[i].b, (i - 2) * 8, 0x20, 4, 4, q);
            }
            self->elems[i] = e;
        }
    }

    for (i = 0; i < 6; i++) {
        self->f10c[i] = 0;
    }
    func_0205e624(self->fc0, 0, 0x20);
    self->f124 = 0;
    self->f128 = -1;
    self->f12c = 0;
    self->f108 = 6;
    func_02081d00(self, 0);
    func_020831b0(self);
    {
        u8 k;
        for (k = 0; k < 8; k++) {
            self->f28[k] = 0;
        }
    }
}
