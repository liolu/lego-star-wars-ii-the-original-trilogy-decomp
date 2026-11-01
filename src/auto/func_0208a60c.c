#include "m2c.h"

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct {
    Vec3 pos;
    s32 id;
    u8 key0;
    u8 key1;
    u8 mode;
    u8 arg;
    u32 pad;
} Entry;

typedef struct Obj Obj;
typedef struct {
    u8 pad_00[0x58];
    void (*method58)(Obj *, u8);
} VTable;

struct Obj {
    VTable *vtbl;
    u8 pad_04[8];
    u16 type;
    u8 pad_0e[0xb0 - 0xe];
    Vec3 pos;
    u8 pad_bc[0xd1 - 0xbc];
    u8 dirty;
    u8 pad_d2[0x1b4 - 0xd2];
    s32 id;
    u8 pad_1b8[0x5e0 - 0x1b8];
    u8 sub[1];
};

void func_02067624(void *, s32, u8);
Obj *func_02067080(M2C_UNK, u8, u8);
void func_0202c498(void *, u8);
extern M2C_UNK data_0210930c;

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

enum {
    KEY0_NONE = 0xff,
};

/*
 * Applies saved entry `index` of `entries` to the object it refers to.
 * Entries with a negative id, or with no key pair (key0 == KEY0_NONE or
 * key1 == 0), are skipped. Otherwise the key pair is passed to
 * func_02067624 (which forwards key1 to func_02067838 on slot key0 of an
 * 8-byte-element table) and used to look the object up via func_02067080.
 * If found, the object's position is copied from the entry (setting the
 * byte at 0xd1 when it changed) and the entry id is stored in the object.
 * Then, for object type 4 the entry's arg byte goes to vtable slot 0x58;
 * for type 2 it goes to func_0202c498 on the object's 0x5e0 member unless
 * arg is 0xb.
 */
void func_0208a60c(Entry *entries, s32 index) {
    Entry *e;
    Obj *obj;

    if (entries[index].id < 0) {
        return;
    }
    e = &entries[index];
    if (e->key0 == KEY0_NONE || e->key1 == 0) {
        return;
    }
    func_02067624((void *) (M2C_FIELD(M2C_FIELD(M2C_FIELD(&data_0210930c, s32 *, 0x68), s32 *, 0x38c), s32 *, 4) + 0x28),
                  e->key0, e->key1);
    obj = func_02067080(M2C_FIELD(M2C_FIELD(&data_0210930c, s32 *, 0x68), s32 *, 0x38c), entries[index].key0,
                        entries[index].key1);
    if (obj == 0) {
        return;
    }
    if (vec_ne(&obj->pos, &entries[index].pos)) {
        vec_set(&obj->pos, &entries[index].pos);
        obj->dirty = 1;
    }
    obj->id = entries[index].id;
    if (obj->type == 4) {
        obj->vtbl->method58(obj, entries[index].arg);
    } else if (obj->type == 2) {
        if (entries[index].arg != 0xb) {
            func_0202c498(obj->sub, entries[index].arg);
        }
    }
}
