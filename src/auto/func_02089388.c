/* func_02089388: for each of the four entity slots of data_0210930c (+0x4c) fills the matching
 * 64-byte record of arg0 (array at +0x21c0, same layout as in func_020897f8). Non-NULL slot: copies
 * the entity position (+0xb0), its three u16 angles (+0xc8), the byte at +0x6bb as the record mode
 * (+0x24), the s16 at +0x6b6 (+0x3e), sets the flag (+0x3f), stores vtable slot 2's result (+0x18) and
 * copies the vector at +0x6a0 (+0x0c). The pair arg1 (+0x30) takes the two bytes at +0x20/+0x21 of the
 * entity's +0x808 pointer (else 0xff, 0). The five u16 slots (+0x26) are reset to ID_NONE, then each
 * data_020eebb8[mode] id that differs from the entity's u16 table (+0x6e8) indexed by func_0208da74(id)
 * puts that table value at slots[func_0208da74(id)]. NULL slot: mode = MODE_SENTINEL, flag cleared.
 */
#include "m2c.h"

enum {
    SLOT_COUNT    = 4,
    MODE_SENTINEL = 0x64,
    ID_NONE       = 0xfa,
};

typedef struct {
    s32 x, y, z;
} Vec3;
typedef struct {
    u16 x, y, z;
} Vec3s;
typedef struct {
    u16 v;
} H16;
typedef struct {
    H16 x, y, z;
} HVec;
typedef struct Item {
    u8 pad[0x20];
    u8 a;
    u8 b;
} Item;
typedef struct Obj Obj;
typedef struct ObjVt {
    void *f0;
    void *f1;
    s32 (*f2)(Obj *);
} ObjVt;
struct Obj {
    ObjVt *vt;
    u8 pad4[0xac];
    Vec3 pos;
    u8 padbc[0xc];
    HVec ang;
    u8 padce[0x6a0 - 0xce];
    Vec3 vec;
    u8 pad6ac[0x6b6 - 0x6ac];
    s16 s6b6;
    u8 pad6b8[3];
    u8 b6bb;
    u8 pad6bc[0x6e8 - 0x6bc];
    u16 tab[0x80];
    u8 pad7e8[0x808 - 0x7e8];
    Item *item;
};
typedef struct Entry {
    Vec3 pos;
    Vec3 vec;
    s32 r;
    Vec3s ang;
    u16 pad22;
    s16 k;
    u16 slots[5];
    struct {
        u8 a, b;
    } pair[7];
    s8 s;
    u8 on;
} Entry;
typedef struct Dst {
    u8 pad[0x21c0];
    Entry e[4];
} Dst;
typedef struct Glob {
    u8 pad[0x4c];
    Obj *obj[4];
} Glob;

extern Glob data_0210930c;
extern u8 data_020eebb8[][5];
s32 func_0208da74(u32 arg0);

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        *(s32 *) d = s->x; // FAKE: cast store keeps the x write on d itself (d->x gets re-derived from arg0 + i*64)
        d->y       = s->y;
        d->z       = s->z;
    }
}

static inline u16 h16v(H16 h) {
    return h.v;
}

static inline void fill_slots(Dst *dst, u8 i) {
    s16 *kp = (s16 *) ((u8 *) dst + i * 64 + 0x21e4); // FAKE: raw spelling of &dst->e[i].k builds it as (dst + i*64) + 0x21e4,
                                                      // sharing dst + i*64 with the slot store
    u16 v;
    u8 j;
    for (j = 0; j < 5; j++) {
        if (data_020eebb8[*kp][j] != ID_NONE) {
            u8 idx = func_0208da74(data_020eebb8[*kp][j]);
            if (data_020eebb8[*kp][j] != data_0210930c.obj[i]->tab[idx]) {
                idx                                                   = func_0208da74(data_020eebb8[*kp][j]);
                v                                                     = data_0210930c.obj[i]->tab[idx];
                dst->e[i].slots[func_0208da74(data_020eebb8[*kp][j])] = v;
            }
        }
    }
}

void func_02089388(Dst *arg0, s32 arg1) {
    u8 i;
    u8 j;
    for (i = 0; i < SLOT_COUNT; i++) {
        if (data_0210930c.obj[i] != 0) {
            Item *item;
            Obj *o;
            vec_set(&arg0->e[i].pos, &data_0210930c.obj[i]->pos);
            o                = data_0210930c.obj[i];
            arg0->e[i].ang.x = h16v(o->ang.x);
            arg0->e[i].ang.y = h16v(o->ang.y);
            arg0->e[i].ang.z = h16v(o->ang.z);
            arg0->e[i].k     = data_0210930c.obj[i]->b6bb;
            arg0->e[i].s     = data_0210930c.obj[i]->s6b6;
            arg0->e[i].on    = 1;
            arg0->e[i].r     = data_0210930c.obj[i]->vt->f2(data_0210930c.obj[i]);
            vec_set(&arg0->e[i].vec, &data_0210930c.obj[i]->vec);
            item = data_0210930c.obj[i]->item;
            if (item != 0) {
                arg0->e[i].pair[arg1].a = item->a;
                arg0->e[i].pair[arg1].b = item->b;
            } else {
                arg0->e[i].pair[arg1].b = 0;
                arg0->e[i].pair[arg1].a = 0xff;
            }
            for (j = 0; j < 5; j++) {
                arg0->e[i].slots[j] = ID_NONE;
            }
            fill_slots(arg0, i);
        } else {
            arg0->e[i].k  = MODE_SENTINEL;
            arg0->e[i].on = 0;
        }
    }
}
