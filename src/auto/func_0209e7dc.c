#include "m2c.h"

enum {
    CHILD_COUNT   = 5,
    ITEM_4C_FIRST = 0x83,
    ITEM_50_FIRST = 0x58,
    ITEM_STEP     = 4,
};

typedef struct {
    s32 v;
} UnkVal;

typedef struct {
    u16 value;
} UnkShort;

typedef struct {
    u8 a;
    u8 b;
    u8 c;
    u8 d;
} Byte4;

typedef struct {
    s16 a;
    s16 b;
} ShortPair;

typedef struct {
    u8 b[8];
} Byte8;

typedef struct {
    s32 x;
    s32 y;
} Pos;

typedef struct {
    void *vptr;
    s32 owner;
    u16 field_08;
    void *graphics;
    UnkShort saved_value; /* graphics entry halfword; func_0209ea20 writes it back */
    void *child;
    void *children[CHILD_COUNT];
    u8 field_2c;
    u8 ids[CHILD_COUNT];
    u8 field_32;
} Self;

typedef struct {
    u8 pad00[8];
    u32 flags;
} Flags;

extern u32 data_020fe77c[];
extern u32 data_020ffb64[];
extern M2C_UNK data_02101b98;
typedef struct {
    u8 b[6];
} Six;

typedef struct {
    UnkShort first;        /* installed into the graphics entry */
    Six rows[CHILD_COUNT]; /* one entry for each of children[] */
    u8 pad20[0x36];
    Six last; /* entry for child */
} Table;

extern Table data_020f7804;
extern void *data_027e02a4;

extern UnkShort func_02053a9c(void *);
extern void func_02053f24(void *, s32);
extern void func_0205353c(void *, UnkShort);
extern void func_02054c80(void *, void *, u16, u32, u32, u32, u32, u32, void *, Byte4);
extern void func_02053808(void *, Pos *);
extern void func_020536ec(void *, Pos *);
extern void func_020536a8(void *, s32);
extern void func_0201ad50(void *, s32, void *, UnkVal, UnkVal, u8, void *);
extern void func_0201a934(void *, UnkVal, UnkVal, s32);
extern void func_02053ff8(void *, s32, s32);
extern void func_02054080(void *, s32, s32);

/* Constructor of the 0x34-byte object that func_02087844 creates for its
 * kind 3; func_0209ea20 is the matching destructor.  owner is the creating
 * object and gfx the block at owner+0x28, whose first two words select the
 * data_02101bb8 entry returned by func_020531d8 (the graphics entry below).
 * Returns self.
 *
 * It saves and replaces the graphics entry's first halfword, changes state
 * in data_02101b98 and data_02101b78 (func_0209ea20 calls the same helpers
 * again with other values) and creates six child objects with
 * func_0201ad50: child and children[0..4].
 *
 * Unknown: field_08, field_2c, field_32, bit 0 of a child's word +0x08, the
 * meaning of the 6-byte table entries and the effect of func_02054c80 (not
 * reconstructed yet).
 *
 * Matching: UnkVal, UnkShort and Byte4 are passed by value, which gives the
 * stack copies of the original.  The children[] slot address is built from
 * a signed byte offset; with &self->children[i] or an unsigned offset
 * mwccarm makes it a single pointer induction variable and the loop no
 * longer matches.
 */
Self *func_0209e7dc(Self *self, s32 owner, void *gfx) {
    Byte8 out;
    ShortPair pair;
    Byte4 bytes;
    Pos pair_1e;
    Pos pair_08;
    UnkVal child_4c;
    UnkVal child_50;
    UnkVal divisor_y;
    UnkVal divisor_x;
    UnkVal item_4c;
    UnkVal item_50;
    UnkShort saved;
    u32 i;

    /* Vtable data_020fe77c with owner and a zero halfword first, then
     * data_020ffb64, the vtable func_0209ea20 restores. */
    self->vptr     = data_020fe77c;
    self->owner    = owner;
    self->field_08 = 0;
    self->vptr     = data_020ffb64;
    self->graphics = gfx;
    self->field_2c = 0;
    /* Keep the entry's first halfword for the destructor and install the
     * first halfword of data_020f7804 in its place. */
    saved             = func_02053a9c(gfx);
    self->saved_value = saved;
    /* Low three bits of word +4 of data_02101b98 = 3; the destructor
     * passes 0. */
    func_02053f24(&data_02101b98, 3);
    func_0205353c(gfx, data_020f7804.first);

    pair.a  = 0;
    pair.b  = 0;
    bytes.a = 0;
    bytes.b = 0;
    bytes.c = 0;
    bytes.d = 0;
    func_02054c80(&out, gfx, 0x1437, 0, 0, 0x50, 0x50, 0x10, &pair, bytes);

    /* func_02053808 keeps both words >> 12 as halfwords at entry +0x1e
     * and +0x20. */
    pair_1e.x = 0x80000;
    pair_1e.y = 0x80000;
    func_02053808(gfx, &pair_1e);
    /* func_020536ec keeps both words at entry +0x08 and +0x0c:
     * (-0x7f000, -0x54000), the negated words given to child below. */
    pair_08.x = -0x7f000;
    pair_08.y = pair_08.x + 0x2b000;
    func_020536ec(gfx, &pair_08);
    /* Sets bit 0x100 << gfx[1] of record gfx[0] in data_02101b78; the
     * destructor passes 0, which clears it. */
    func_020536a8(gfx, 1);

    /* func_0201ad50 allocates a 0x80-byte object, stores it in the given
     * slot and hands it a 6-byte data_020f7804 entry and two words, which
     * func_0201adc4 keeps at +0x4c and +0x50. */
    child_50.v = 0x54000;
    child_4c.v = 0x7f000;
    func_0201ad50(&self->child, 3, &data_020f7804.last, child_4c, child_50, 1, data_027e02a4);
    /* func_0201a934 keeps both words at child +0x30 and the last argument
     * at +0x3c, and divides by the words. */
    divisor_x.v = 0x1000;
    divisor_y.v = 0x1000;
    func_0201a934(self->child, divisor_x, divisor_y, 0);

    self->field_32 = 0xe;
    /* children[i] gets rows[i] and two words that grow by 4 << 12 per
     * child. */
    for (i = 0; i < CHILD_COUNT; i++) {
        Flags *item;
        s32 slot_offset = i * sizeof(self->children[0]);

        item_50.v = (ITEM_50_FIRST + i * ITEM_STEP) << 12;
        item_4c.v = (ITEM_4C_FIRST + i * ITEM_STEP) << 12;
        func_0201ad50((void *) ((u8 *) self->children + slot_offset), 3, &data_020f7804.rows[i], item_4c, item_50, 1,
                      data_027e02a4);
        item = (Flags *) self->children[i];
        item->flags &= ~1;
        self->ids[i] = i;
    }
    /* The destructor passes (1, 1) and (0, 0) to these two. */
    func_02053ff8(&data_02101b98, 8, 0x17);
    func_02054080(&data_02101b98, 8, 8);
    return self;
}
