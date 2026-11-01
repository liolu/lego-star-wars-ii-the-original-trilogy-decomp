#include "m2c.h"

struct Vec3 {
    s32 x;
    s32 y;
    s32 z;

    Vec3() {}
    Vec3(s32 vx, s32 vy, s32 vz) {
        x = vx;
        y = vy;
        z = vz;
    }
};

typedef struct Part {
    s32 pad0[2];
    u32 flags;
    u8 pad1[0xC];
    u8 mode_a;
    u8 mode_b;
} Part;

typedef struct Pair {
    u8 pad[0x2c];
    Part *part;
} Pair;

typedef struct Cfg {
    u8 pad[0x24];
    Vec3 pos;
    Vec3 scale;
    s16 r0;
    s16 r1;
    s16 r2;
    u8 gap[2];
    u8 f44;
    u8 f45;
} Cfg;

static inline s32 vec_ne(const Vec3 *a, const Vec3 *b) {
    if (a->x != b->x || a->y != b->y || a->z != b->z) {
        return 1;
    }
    return 0;
}

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

class Node {
public:
    virtual void unknown00();
    virtual void unknown04();
    virtual void unknown08();
    virtual void unknown0c();
    virtual void unknown10();
    virtual void unknown14();
    virtual void unknown18();
    virtual void unknown1c();
    virtual void unknown20();
    virtual void unknown24(s32, s32, s32, s32); // Slot 0x24.

    u8 pad_04[0xac];
    Vec3 pos;
    Vec3 scale;
    u8 pad_c8[9];
    u8 dirty;

    void SetPos(const Vec3 &v) {
        if (vec_ne(&pos, &v)) {
            vec_set(&pos, &v);
            dirty = 1;
        }
    }
    void SetScale(s32 s) {
        SetScale(Vec3(s, s, s));
    }
    void SetScale(const Vec3 &v) {
        if (vec_ne(&scale, &v)) {
            vec_set(&scale, &v);
            dirty = 1;
        }
    }
};

/* Per-index table entry split by the symbol map: id word at data_020ff1dc,
   following entries' bytes at data_020ff1e0. */
typedef struct Entry {
    s32 id;
    u8 pad4[4];
} Entry;

typedef struct EntryTail {
    u8 b0;
    u8 scale;
    u8 pad2[2];
    s32 next_id;
} EntryTail;

typedef struct Menu {
    s32 index;
    s32 pad4;
    void *obj8;
    void *objC;
    s32 *obj10;
    Pair *pair;
    u8 pad18[0xF];
    u8 state;
    u8 pad28[0x14];
    Node *node;
} Menu;

extern "C" void func_0205365c(s32 *, s32);
extern "C" void func_0205ea38(s32, s32);
extern "C" void func_0206574c(void *, s32, s32);
extern "C" M2C_UNK *func_02066708(s32);
extern "C" void func_02065be8(void *, s32, s32, s32, M2C_UNK *, s32);
extern "C" void func_0207f068(void *, s32, s32);
extern "C" s32 func_0208c72c(s32 *, u32);
extern "C" void *func_0205e80c(s32, s32, s32);
extern "C" void *func_0200ff50(void *, s32, s32, s32, void *);
extern "C" s32 data_02104dc4;
extern "C" s32 data_027e02a8;
extern "C" u8 data_02106d90[];
extern "C" Entry data_020ff1dc[];
extern "C" EntryTail data_020ff1e0[];

/* Node transform values are fx32 (0x1000 = 1.0). */
enum {
    FX32_SHIFT = 12,
    FX32_ONE   = 0x1000
};

/* If the lookup for menu->index (or, when bit 0x40 of data_027e02a8 is set,
   byte 0x8d1 of data_02106d90) is nonzero: set up the menu sub-objects,
   allocate and construct a 0x1bc-byte Node with unit scale, give it a
   uniform scale from the per-index table byte (integer -> fx32) and the
   position (0, -80.0, -250.0), and return 1. Otherwise return 0. */
extern "C" s32 func_0207e590(Menu *menu) {
    s32 ok;
    void *mem;
    Cfg cfg;
    Part *p;

    ok = func_0208c72c(&data_02104dc4, menu->index);
    if (data_027e02a8 & 0x40) {
        ok = data_02106d90[0x8d1];
    }
    if (ok != 0) {
        func_0205365c(menu->obj10, 0);
        func_0205ea38((s32) &menu->pair, 0);
        menu->pair->part->flags |= 1;
        p         = menu->pair->part;
        p->mode_b = p->mode_a = 3;
        menu->pair->part->flags |= 8;
        func_0206574c(menu->objC, 9, -2);
        func_02065be8(menu->objC, 0x8c, 0x80, 0x22, func_02066708(0x158), 1);
        func_0206574c(menu->obj8, 0xc, -2);
        func_0207f068(menu, 0, ok);
        menu->state = 0x11;
        mem         = func_0205e80c(0x1bc, 3, 4);
        if (mem != NULL) {
            cfg.pos.x   = 0;
            cfg.pos.y   = 0;
            cfg.pos.z   = 0;
            cfg.scale.x = FX32_ONE;
            cfg.scale.y = FX32_ONE;
            cfg.scale.z = FX32_ONE;
            cfg.r0      = 0;
            cfg.r1      = 0;
            cfg.r2      = 0;
            cfg.f44     = 1;
            cfg.f45     = 1;
            mem         = func_0200ff50(mem, 4, data_020ff1dc[menu->index].id, 1, &cfg);
        }
        menu->node = (Node *) mem;
        menu->node->unknown24(0xe4, 0, 1, 0);
        menu->node->unknown24(0x79, 0, 1, 0);
        menu->node->SetScale((s32) (data_020ff1e0[menu->index].scale << FX32_SHIFT));
        menu->node->SetPos(Vec3(0, -0x50000, -0xfa000));
        return 1;
    }
    return 0;
}
