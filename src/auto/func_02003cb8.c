#include "m2c.h"

enum {
    KIND_FIRST       = 0x54, /* kinds 0x54..0x63 get the large default */
    KIND_LAST        = 0x63,
    KIND_DOUBLED_MAX = 2,    /* kinds 0x54..0x56 are doubled again */
    MODE_A           = 1,    /* func_02093fa8 results that select the small default */
    MODE_B           = 2,
    FLAG_QUARTER     = 13,   /* func_020867ec flag that quarters the vector */
    FX_TWO           = 0x2000, /* 2.0 in 20.12 fixed point */
};

typedef struct Vec3 {
    s32 x, y, z;
} Vec3;

typedef struct KindInfo {
    u8 flag;
    u8 pad[11];
} KindInfo;

typedef struct Obj {
    u8 pad0[0xc];
    u16 unkC;
    u8 pad1[0x42 - 0xe];
    u8 unk42;
    u8 pad2[0xbc - 0x43];
    Vec3 scale;
    u8 pad3[0xd1 - 0xc8];
    u8 dirty;
    u8 pad4[0x6bb - 0xd2];
    u8 kind;
} Obj;

extern KindInfo data_020fec74[];
extern M2C_UNK data_0210930c;
extern M2C_UNK data_02104d90;

extern void func_02007f24(s32 *, f64);
extern void func_02010b7c(void *, s32);
extern s32 func_020867ec(s32, s32);
extern u8 func_020917b4(void *);
extern u8 func_02093fa8(s32);

static inline s32 fx_mul(s32 a, s32 b) {
    return (s32) (((s64) a * b + 0x800) >> 12);
}

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

/* Stores v into obj->scale and raises the dirty byte only when it differs. */
static inline void obj_set_scale(Obj *obj, const Vec3 *v) {
    if (vec_ne(&obj->scale, v)) {
        vec_set(&obj->scale, v);
        obj->dirty = 1;
    }
}

static inline void obj_set_uniform(Obj *obj, s32 s) {
    Vec3 v;
    Vec3 *p = &v;
    p->x    = s;
    p->y    = s;
    p->z    = s;
    obj_set_scale(obj, p);
}

static inline void vec_scale(Vec3 *d, const Vec3 *s, s32 f) {
    s32 z = fx_mul(s->z, f);
    s32 y = fx_mul(s->y, f);
    s32 x = fx_mul(s->x, f);
    d->x  = x;
    d->y  = y;
    d->z  = z;
}

/* Recomputes the Vec3 at 0xbc (named scale: it is set uniformly and multiplied by factors).
 * Kinds 0x54..0x63 flag 0x42 when their 12-byte data_020fec74 entry has byte 0 clear and,
 * unless func_02093fa8 reports mode 1 or 2, get 6.25; other kinds get 0.16 only in those
 * modes. Kinds 0x54..0x56 are then doubled, and flag 13 of data_02104d90 (with unkC != 1)
 * quarters it. Each step sets the dirty byte 0xd1 only on change; if any step ran
 * (changed or not), func_02010b7c(obj, 1) is called. */
void func_02003cb8(Obj *obj) {
    s32 changed = 0;

    if (obj->kind >= KIND_FIRST && obj->kind <= KIND_LAST) {
        if (data_020fec74[obj->kind].flag == 0) {
            obj->unk42 = 1;
        }
        if (func_02093fa8(func_020917b4(&data_0210930c)) != MODE_A && func_02093fa8(func_020917b4(&data_0210930c)) != MODE_B) {
            s32 s;
            func_02007f24(&s, 6.25);
            obj_set_uniform(obj, s);
            changed = 1;
        }
    } else if (func_02093fa8(func_020917b4(&data_0210930c)) == MODE_A ||
               func_02093fa8(func_020917b4(&data_0210930c)) == MODE_B) {
        s32 s;
        func_02007f24(&s, 0.16);
        obj_set_uniform(obj, s);
        changed = 1;
    }

    if ((u32) (obj->kind - KIND_FIRST) <= KIND_DOUBLED_MAX) {
        Vec3 v;
        vec_scale(&v, &obj->scale, FX_TWO);
        obj_set_scale(obj, &v);
        changed = 1;
    }

    if (func_020867ec((s32) &data_02104d90, FLAG_QUARTER) != 0 && obj->unkC != 1) {
        s32 s;
        Vec3 v;
        func_02007f24(&s, 0.25);
        vec_scale(&v, &obj->scale, s);
        obj_set_scale(obj, &v);
        changed = 1;
    }

    if (changed) {
        func_02010b7c(obj, 1);
    }
}
