#include "m2c.h"

typedef struct Vec3 {
    s32 x, y, z;
} Vec3;

typedef struct H16 {
    u16 v;
} H16;

typedef struct Rot3 {
    H16 x, y, z, w;
} Rot3;

typedef struct Part {
    u8 pad0[0xB0];
    Vec3 pos;
    u8 pad1[0xC8 - 0xBC];
    Rot3 rot;
    u8 pad2[0xD0 - 0xCE - 2];
    u8 rotDirty;
    u8 posDirty;
} Part;

typedef struct Src {
    s16 x, y, z, ang;
} Src;

typedef struct Mgr {
    u8 pad0[0x4C];
    Part *ptrs[4];
    u8 pad1[0x68 - 0x5C];
    void *anim;
} Mgr;

typedef struct Self {
    void *vtbl;
    Part *part;
    u8 pad8[4];
    u8 flags;
    u8 padD;
    s8 anim_id;
} Self;

typedef struct StateObj {
    u8 pad0[4];
    s32 id;
} StateObj;

typedef struct StateMgr {
    u8 pad0[4];
    StateObj *cur;
} StateMgr;

extern s32 data_027e02a8;
extern u8 data_021027f8[];
extern Mgr data_0210930c;
extern StateMgr data_02102900;

void *func_020aaeb4(void);
void func_020a3458(void *arg0, s8 arg1);
Src **func_02090668(void *, s32, s32);
void func_0206dd60(void);
void func_0205ecc4(void *arg0, s32 arg1, s32 arg2, void *arg3, s32 arg4, s32 arg5);

static inline int vec_ne(const Vec3 *a, const Vec3 *b) {
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

static inline void vec_from16(Vec3 *d, Src **p) {
    s32 z = (*p)->z << 12;
    s32 y = (*p)->y << 12;
    d->x  = (*p)->x << 12;
    d->y  = y;
    d->z  = z;
}

static inline s32 state_id(void) {
    if (data_02102900.cur != NULL) {
        return data_02102900.cur->id;
    }
    return 0;
}

static inline int half_eq(const H16 *a, H16 b) {
    return a->v == b.v;
}

static inline void half_set(H16 *d, H16 v) {
    d->v = v.v;
}

static inline void sync_pos(Part *o, const Vec3 *p) {
    if (vec_ne(&o->pos, p)) {
        vec_set(&o->pos, p);
        o->posDirty = 1;
    }
}

static inline void sync_ang(Part *o, const Rot3 *a) {
    int e;
    int f;
    f = 0;
    e = 0;
    if (half_eq(&a->x, o->rot.x)) {
        if (half_eq(&a->y, o->rot.y)) {
            e = 1;
        }
    }
    if (e) {
        if (half_eq(&a->z, o->rot.z)) {
            f = 1;
        }
    }
    if (!f) {
        half_set(&o->rot.x, a->x);
        half_set(&o->rot.y, a->y);
        half_set(&o->rot.z, a->z);
        o->rotDirty = 1;
    }
}

/*
 * Per-frame sync of self->part with the record that func_02090668 returns for
 * self->anim_id. With bit 0x40 of data_027e02a8 set: if the part is one of the
 * four slot pointers in data_0210930c and data_021027f8[0x2a] is non-zero,
 * calls func_020a3458(func_020aaeb4(), 1); then copies the record's whole-unit
 * x/y/z (shifted to 20.12 fixed point) into part->pos and its Y rotation into
 * part->rot.y, setting the matching dirty byte only when a value changed. The
 * Y rotation is 20.12 fixed-point radians; multiplying by 0x28BE60DB9391 / 2^44
 * (= 65536 / (2 * pi * 4096)) with rounding gives a 16-bit angle (same
 * conversion as func_0206dad0). Without the bit: unless data_02102900's current
 * entry already has id 0x1E, calls func_0206dd60 and passes 0x1E and the record
 * to func_0205ecc4. Both paths set bit 0 of self->flags.
 */
void func_02036424(Self *self) {
    if (data_027e02a8 & 0x40) {
        u8 i;
        Part *me   = self->part;
        u8 enabled = data_021027f8[0x2A];
        Src **sp;
        s32 ang;
        Vec3 v;
        Rot3 r;

        for (i = 0; i < 4; i++) {
            if (me == data_0210930c.ptrs[i] && enabled) {
                func_020a3458(func_020aaeb4(), 1);
                break;
            }
        }
        sp = func_02090668(data_0210930c.anim, self->anim_id, 0);
        vec_from16(&v, sp);
        sync_pos(self->part, &v);
        ang   = (*sp)->ang;
        r.x.v = 0;
        r.y.v = (s32) (((s64) ang * 0x28BE60DB9391LL + 0x080000000000LL) >> 44);
        r.z.v = 0;
        sync_ang(self->part, &r);
        self->flags |= 1;
    } else {
        if (state_id() == 0x1E) {
            return;
        }
        func_0206dd60();
        func_0205ecc4(&data_02102900, 0x1E, 1, func_02090668(data_0210930c.anim, self->anim_id, 0), 0, 0);
        self->flags |= 1;
    }
}
