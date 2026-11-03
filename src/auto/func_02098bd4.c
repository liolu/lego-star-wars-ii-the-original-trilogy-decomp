/* func_02098bd4: resets the object's state bytes (+0x28, +0x2afc..+0x2b01), calls func_0208896c on
 * data_02104df4, sets byte 0 of the flag block at data_02106d90+0x8d0 to 9 and, for each of the
 * data_021027f8[0x29] 64-byte slots of data_02104d90, sets the halfword at slot+0x24 to 0x14 and the
 * byte at slot+0x3f to 1. It then copies the default configuration (360 entries, 4 slots, 180 items and the
 * trailing blocks) from data_02104df4 / data_02106d90 and the following data symbols into self+0x2c.
 */
#include "m2c.h"

enum {
    FLAG0_INIT  = 9,
    SLOT_D_INIT = 0x14,
    SLOT_Y_INIT = 1,
    ENTRY_COUNT = 360,
    PAIR_COUNT  = 7,
    ITEM_COUNT  = 180,
};

typedef struct Vec3 {
    s32 x, y, z;
} Vec3;

typedef struct Half {
    u16 v;
} Half;

typedef struct HVec {
    Half x, y, z;
} HVec;

typedef struct Bytes5 {
    u8 b[5];
} Bytes5;

typedef struct Pair {
    u8 a, b;
} Pair;

typedef struct Entry {
    Vec3 pos;
    s32 val;
    Bytes5 bytes;
} Entry;

typedef struct Blk10 {
    u16 h[5];
} Blk10;

typedef struct Slot {
    Vec3 a;
    Vec3 b;
    s32 c;
    HVec h;
    u16 pad22;
    s16 d;
    Blk10 e;
    Pair p[7];
    u8 x;
    u8 y;
} Slot;

typedef struct Item {
    u8 a, b;
    s32 c;
} Item;

typedef struct Blk4 {
    u8 b[4];
} Blk4;
typedef struct Blk57 {
    u8 b[57];
} Blk57;
typedef struct Blk18 {
    u8 b[18];
} Blk18;
typedef struct Blk70 {
    u16 h[35];
} Blk70;
typedef struct Blk284 {
    s32 w[71];
} Blk284;
typedef struct Blk64 {
    s32 w[16];
} Blk64;

typedef struct Ints2 {
    s32 a, b;
} Ints2;

typedef struct Cfg {
    Entry ent[360];
    Slot slot[4];
    Item item[180];
    Vec3 v;
    Blk4 flags;
    Ints2 ii;
    Blk57 k;
    Blk18 l;
    Blk70 m;
    s8 n;
    Blk284 o;
    Blk64 q;
    u8 r;
} Cfg;

typedef struct CfgTail {
    u8 pad0[0x224];
    Slot slot[4];
    Item item[180];
    Vec3 v;
    Blk4 flags;
    Ints2 ii;
    Blk57 k;
    Blk18 l;
    Blk70 m;
    s8 n;
    Blk284 o;
    Blk64 q;
    u8 r;
} CfgTail;

typedef struct Obj {
    u8 pad0[0x28];
    s32 f28;
    Cfg cfg;
    u8 pad2a98[0x2AFC - 0x2A98];
    u8 afc;
    u8 pad2afd;
    u16 afe;
    u8 b00;
    u8 b01;
} Obj;

typedef struct Global {
    u8 pad0[0x64];
    Cfg cfg;
} Global;

typedef struct Info {
    u8 pad0[0x29];
    u8 slotCount;
} Info;

extern void func_0208896c(Cfg *p);
extern Global data_02104d90;
extern Info data_021027f8;
extern CfgTail data_02106d90;
extern Entry data_02104df4[];
extern Slot data_02106fb4;
extern Vec3 data_02106fc0;
extern u8 data_02106f90[];
extern Pair data_02106fe4[];
extern Slot data_02106ff4;
extern Vec3 data_02107000;
extern Pair data_02107024[];
extern Slot data_02107034;
extern Vec3 data_02107040;
extern Pair data_02107064[];
extern Slot data_02107074;
extern Vec3 data_02107080;
extern u8 data_02107090[];
extern Pair data_021070a4[];
extern Item data_021070b4[];
extern Blk57 data_0210766c;
extern Blk70 data_021076b8;
extern Blk284 data_02107700;
extern Blk64 data_0210781c;

static inline void entry_set(Entry *d, const Entry *s) {
    if (&d->pos != &s->pos) {
        d->pos.x = s->pos.x;
        d->pos.y = s->pos.y;
        d->pos.z = s->pos.z;
    }
    d->val        = s->val;
    d->bytes.b[0] = s->bytes.b[0];
    d->bytes.b[1] = s->bytes.b[1];
    d->bytes.b[2] = s->bytes.b[2];
    d->bytes.b[3] = s->bytes.b[3];
    d->bytes.b[4] = s->bytes.b[4];
}

static inline void half_set(Half *d, Half v) {
    d->v = v.v;
}
static inline void hvec_set(HVec *d, const HVec *s) {
    half_set(&d->x, s->x);
    half_set(&d->y, s->y);
    half_set(&d->z, s->z);
}

/* s: slot address, v: same slot read through the +0x2000 view,
 * b: address of s->b, h: halfword view, p: pair array */
static inline void slot_set(Slot *d, const Slot *s, const Slot *v, const Vec3 *b, const Slot *h, const Pair *p) {
    const Pair *sp;
    Pair *dp;
    if (&d->a != &s->a) {
        d->a.x = s->a.x;
        d->a.y = v->a.y;
        d->a.z = v->a.z;
    }
    if (&d->b != b) {
        d->b.x = v->b.x;
        d->b.y = v->b.y;
        d->b.z = v->b.z;
    }
    d->c = v->c;
    hvec_set(&d->h, &h->h);
    d->d = h->d;
    d->e = h->e;
    sp   = p;
    dp   = d->p;
    do {
        dp->a = sp->a;
        dp->b = sp->b;
        dp++;
        sp++;
    } while (dp < d->p + PAIR_COUNT);
    d->x = v->x;
    d->y = v->y;
}

/* slots_set1/2/2w/3 only set the inline depth of each slot copy. mwcc lays out the dead by-value
 * Half copies of half_set by inline depth (shallower first, higher address), so slot 3 is copied at
 * depth 1, slot 2 at depth 2, slot 1 at depth 3 and slot 0 at depth 4. One helper holding three or
 * more slot copies is not inlined. */
static inline void slots_set1(Cfg *d) {
    slot_set(&d->slot[0], &data_02106fb4, &data_02106d90.slot[0], &data_02106fc0, (const Slot *) (data_02106f90 + 0x24),
             data_02106fe4);
}

static inline void slots_set2(Cfg *d) {
    slots_set1(d);
    slot_set(&d->slot[1], &data_02106ff4, &data_02106d90.slot[1], &data_02107000, (const Slot *) (data_02106f90 + 0x64),
             data_02107024);
}

// FAKE: slots_set1/2/2w/3 nest only sets inline depth; slots_set2w is a pure pass-through
static inline void slots_set2w(Cfg *d) {
    slots_set2(d);
}

static inline void slots_set3(Cfg *d) {
    slot_set(&d->slot[2], &data_02107034, &data_02106d90.slot[2], &data_02107040, (const Slot *) (data_02106f90 + 0xa4),
             data_02107064);
}

void func_02098bd4(Obj *self) {
    const Entry *se;
    Entry *de;
    Item *di;
    const Item *si;
    u8 i;

    self->f28 = 0;
    self->b00 = 0;
    self->b01 = 0;
    self->afc = 0;
    self->afe = 0;
    func_0208896c((Cfg *) data_02104df4);
    data_02106d90.flags.b[0] = FLAG0_INIT;
    for (i = 0; i < data_021027f8.slotCount; i++) {
        data_02104d90.cfg.slot[i].d = SLOT_D_INIT;
        data_02104d90.cfg.slot[i].y = SLOT_Y_INIT;
    }

    se = data_02104df4;
    de = self->cfg.ent;
    do {
        entry_set(de, se);
        de++;
        se++;
    } while (de < self->cfg.ent + ENTRY_COUNT);

    slots_set2w(&self->cfg);
    slots_set3(&self->cfg);
    slot_set(&self->cfg.slot[3], &data_02107074, &data_02106d90.slot[3], &data_02107080, (const Slot *) (data_02107090 - 0x1c),
             data_021070a4);

    di = self->cfg.item;
    si = data_021070b4;
    do {
        di->a = si->a;
        di->b = si->b;
        di->c = si->c;
        di++;
        si++;
    } while (di < self->cfg.item + ITEM_COUNT);
    self->cfg.v.x        = data_02106d90.v.x;
    self->cfg.v.y        = data_02106d90.v.y;
    self->cfg.v.z        = data_02106d90.v.z;
    self->cfg.flags.b[0] = data_02106d90.flags.b[0];
    self->cfg.flags.b[1] = data_02106d90.flags.b[1];
    self->cfg.flags.b[2] = data_02106d90.flags.b[2];
    self->cfg.flags.b[3] = data_02106d90.flags.b[3];
    self->cfg.ii         = data_02106d90.ii;
    self->cfg.k          = data_0210766c;
    self->cfg.l          = *(const Blk18 *) (data_0210766c.b + 0x39);
    self->cfg.m          = data_021076b8;
    self->cfg.n          = data_02106d90.n;
    self->cfg.o          = data_02107700;
    self->cfg.q          = data_0210781c;
    self->cfg.r          = data_02106d90.r;
}
