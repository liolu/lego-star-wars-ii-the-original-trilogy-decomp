#include "m2c.h"

typedef struct {
    u8 f00;
    u8 f01;
    u8 f02;
    u8 f03;
} Byte4;

typedef struct {
    s16 f00;
    s16 f02;
} ShortPair;

typedef struct {
    u8 f00[8];
} Byte8;

enum {
    LIST_SLOTS        = 5,    /* row objects allocated and laid out by func_0207b540 */
    VISIBLE_ROWS      = 4,    /* rows filled by func_0207b420; first_visible is clamped to count - 4 */
    ENTRY_COUNT_FULL  = 0x1d, /* table length when flag bit 10 of data_02104d90 is set */
    ENTRY_COUNT_BASIC = 0xc,
};

/* 8-byte table entry; kind selects which flag set of data_02104d90 gates it, arg is the bit. */
typedef struct {
    u8 kind;
    u8 arg;
    u8 pad02[6];
} Entry;

typedef struct {
    u8 pad00[8];
    u32 flags;
} Sprite;

typedef struct {
    void *vtable;
    void *owner;
    u16 f08;
    u8 pad0A[2];
    u8 f0C[8];
    void *f14;
    void *f18;
    Sprite *slots[LIST_SLOTS]; /* drawn rows, see func_0207b420/func_0207b540 */
    s32 first_visible;         /* index of the first entry shown */
    s32 scroll;
    s32 f38;
    s32 f3C;
    s32 entry_count;
    Entry **entries;           /* entries that passed the filter */
    s32 f48[2];
} Obj;

typedef struct {
    u8 pad00[0x50];
    u8 f50;
} Global;

extern u8 data_020fe77c[];
extern u8 data_020ff050[];
extern Global data_02104d90;
extern const Entry data_020ee704[];
extern const Entry data_020ee6a4[];
extern u8 data_02101b98[];
extern const u16 data_020ee670[];

extern void *func_0201abbc(void *object, M2C_UNK initializerSelector);
extern void func_020536a8(void *arg0, s32 arg1);
extern void func_020540b4(void *arg0, s16 arg1, s16 arg2);
extern void func_02054c80(void *, void *, u32, u32, u32, u32, u32, u32, ShortPair *, Byte4);
extern void *func_0205e80c(s32, s32, s32);
extern void *func_0205e818(s32, s32, s32);
extern void func_02060cd0(void *arg0, u32 arg1, u32 arg2);
extern void func_0206574c(void *arg0, s32 arg1, s32 arg2);
extern void func_02065be8(M2C_UNK arg0, M2C_UNK arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5);
extern M2C_UNK *func_02066708(M2C_UNK arg0);
extern void func_02067bc0(void *arg0);
extern void func_02067be4(void *arg0, s32 arg1, s32 arg2, M2C_UNK arg3, u8 arg4, u8 arg5, u8 arg6);
extern void func_0207b420(Obj *s, s32 idx);
extern void func_0207b540(Obj *p);
extern void func_0207b5dc(Obj *p, s32 a, s32 b);
extern s32 func_02086204(void *arg0, s32 arg1);
extern s32 func_020865dc(void *arg0, s32 arg1);
extern s32 func_02086630(void *arg0, s32 arg1);
extern s32 func_0208671c(void *arg0, s32 arg1);

/* Constructor-style init of a scrolling list object (called from func_02087844 after a func_0205e80c
 * allocation). Stores the base then the final type word at +0, owner and the two handles passed by the
 * caller, picks the 29- or 12-entry table depending on flag bit 10 of data_02104d90, keeps only the
 * entries whose flag bit is set (kind 0 always; 1/2 test the entry's arg bit in two flag sets; 3/4 test
 * fixed bits 11/10), clamps first_visible (read from byte +0x50 of data_02104d90) so four rows stay filled,
 * allocates the five row objects, fills and lays out the visible rows, then sets up two
 * func_02054c80 items (ids from data_020ee670) and refreshes them via func_0207b5dc. */
Obj *func_0207a9c4(Obj *self, void *owner, void *arg2, void *arg3) {
    const Entry *tab;
    s32 n;
    s32 i;
    Sprite *p;

    self->vtable = data_020fe77c;
    self->owner  = owner;
    self->f08    = 0;
    self->vtable = data_020ff050;
    func_02067bc0(self->f0C);
    self->f14           = arg2;
    self->f18           = arg3;
    self->first_visible = 0;
    self->scroll        = 0;
    self->f38           = -1;
    self->f3C           = 0;
    self->entry_count   = 0;
    self->entries       = 0;

    if (func_02086204(&data_02104d90, 10) != 0) {
        n   = ENTRY_COUNT_FULL;
        tab = data_020ee704;
    } else {
        n   = ENTRY_COUNT_BASIC;
        tab = data_020ee6a4;
    }
    self->entries = func_0205e818(n * 4, 3, 4);
    for (i = 0; i < n; i++) {
        switch (tab[i].kind) {
            case 2:
                if (func_020865dc(&data_02104d90, tab[i].arg) != 0) {
                    self->entries[self->entry_count++] = (Entry *) &tab[i];
                }
                break;
            case 1:
                if (func_02086630(&data_02104d90, tab[i].arg) != 0) {
                    self->entries[self->entry_count++] = (Entry *) &tab[i];
                }
                break;
            case 3:
                if (func_0208671c(&data_02104d90, 11) != 0) {
                    self->entries[self->entry_count++] = (Entry *) &tab[i];
                }
                break;
            case 4:
                if (func_0208671c(&data_02104d90, 10) != 0) {
                    self->entries[self->entry_count++] = (Entry *) &tab[i];
                }
                break;
            default:
                self->entries[self->entry_count++] = (Entry *) &tab[i];
                break;
        }
    }

    self->first_visible = data_02104d90.f50;
    if (self->entry_count < VISIBLE_ROWS) {
        self->first_visible = 0;
    } else if (self->first_visible + VISIBLE_ROWS > self->entry_count) {
        self->first_visible = self->entry_count - VISIBLE_ROWS;
    }

    func_020536a8(self->f14, 1);
    func_0206574c(self->f18, 9, -2);
    func_02065be8(self->f18, 0x31, 0x80, 0x22, func_02066708(0x3b), 1);
    func_02067be4(self->f0C, 1, 0, 0xd8, 0x3a, 0x28, 0x75);
    func_020540b4(data_02101b98, 0x2f, 0x3f);

    for (i = 0; i < LIST_SLOTS; i++) {
        p = func_0205e80c(0x80, 0, 4);
        if (p != 0) {
            p = func_0201abbc(p, 1);
        }
        self->slots[i] = p;
        p->flags &= ~1;
        func_02060cd0(self->slots[i], i + 4, 1);
    }

    for (i = 0; i < VISIBLE_ROWS; i++) {
        func_0207b420(self, self->first_visible + i);
    }
    func_0207b540(self);

    {
        s32 j;
        Byte8 out;
        ShortPair pair;
        Byte4 bytes;

        bytes.f00 = 0;
        bytes.f01 = 0;
        bytes.f02 = 0;
        bytes.f03 = 0;
        for (j = 0; j < 2; j++) {
            self->f48[j] = 0;
            pair.f00     = 0;
            pair.f02     = 0;
            func_02054c80(&out, self->f14, data_020ee670[j], 2, 1 + j * 0x18, 0x40, 0, 0x10, &pair, bytes);
        }
    }

    func_0207b5dc(self, 0, 0);
    func_0207b5dc(self, 1, 0);
    return self;
}
