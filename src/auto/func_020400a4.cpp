#include "m2c.h"

typedef struct Vec3 {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct Vec3s {
    s16 x;
    s16 y;
    s16 z;
} Vec3s;

// Partial interface: unknown virtuals keep their observed vtable slots.
// Declarations only: the vtable stays in the ROM data.
class Entity {
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
    virtual void action(int, int, int, int); // Slot 0x24.
    u8 pad4[4];
    u32 flags;
    u8 pad0c[0x690 - 0xc];
    void *ctrl;
};
typedef Entity Owner;

typedef struct Inner Inner;

typedef struct Holder {
    u32 vptr;
    Owner *owner;
    u8 pad8[4];
    u8 flag_c;
    u8 flag_d;
    u8 padE[2];
    Inner *inner;
} Holder;

struct Inner {
    s32 state;
    u8 pad4[4];
    Holder *parent;
    Owner *owner;
    u8 pad10[4];
    s32 f14, f18, f1c, f20, f24, f28, f2c, f30, f34, f38, f3c, f40;
    u8 pad44[4];
    s32 f48;
    u8 pad4c[4];
    s32 arr50[2];
    s32 f58, f5c, f60;
    u8 pad64[8];
    s32 f6c, f70, f74;
    Vec3 center;
    Vec3 pts[2];
};

typedef struct Src {
    u8 pad0[0x34];
    u8 *list;
} Src;

typedef struct Mid {
    u8 pad0[4];
    u8 *p;
} Mid;

typedef struct Root {
    u8 pad[0x38c];
    Mid *mid;
} Root;

typedef struct Glob {
    u8 pad[0x68];
    Root *root;
} Glob;

extern "C" {

// Size of Inner: two Vec3 points end at 0x84 + 2 * 12.
enum {
    INNER_SIZE = 0x9c
};
enum {
    OWNER_FLAG_BIT25 = 0x2000000
};

extern u32 data_020fdbc4;
extern u32 data_020fe1ec;
extern Glob data_0210930c;

extern void *func_0205e824(s32 size);
extern void func_02027ff0(s32 *p);
extern void func_0202934c(void *dst, void *src);
extern void func_020e46bc(void *, s32, s32, void *, void *);
extern Vec3s *func_02067b0c(void *, s32);
extern u8 func_02067b18(void *);
extern void func_02040078(void);
extern void func_020402d0(void);

static inline void vec_init(Vec3 *o, s32 x, s32 y, s32 z) {
    o->x = x;
    o->y = y;
    o->z = z;
}

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

Holder *func_020400a4(Holder *self, Owner *owner, s32 arg2) {
    u8 *list;
    Vec3 v;
    u32 i;
    Vec3 *pv;
    Vec3 tmp;
    Inner *obj;
    // Base vtable first, then the derived one (inlined constructor chain).
    self->vptr   = (u32) &data_020fdbc4;
    self->owner  = owner;
    self->flag_c = 0;
    self->flag_d = 0;
    self->vptr   = (u32) &data_020fe1ec;
    // Allocate the inner object, constructed inline after the NULL test.
    obj = (Inner *) func_0205e824(INNER_SIZE);
    if (obj != 0) {
        obj->parent = self;
        obj->f14    = 0;
        obj->f18    = 0;
        obj->f1c    = 0;
        obj->f20    = 0;
        obj->f24    = 0;
        obj->f28    = 0;
        obj->f2c    = 0;
        obj->f30    = 0;
        obj->f34    = 0;
        obj->f38    = 0;
        obj->f3c    = 0;
        obj->f40    = 0;
        func_02027ff0(&obj->f48);
        func_020e46bc(obj->arr50, 2, 4, func_020402d0, func_02040078);
        obj->f58      = 0;
        obj->f5c      = 0;
        obj->f60      = 0;
        obj->f6c      = 0;
        obj->f70      = 0;
        obj->f74      = arg2;
        obj->center.x = 0;
        obj->center.y = 0;
        obj->center.z = 0;
        {
            Vec3 *p = obj->pts;
            do {
                p->x = 0;
                p->y = 0;
                p->z = 0;
                p++;
            } while (p < obj->pts + 2);
        }
        obj->owner = obj->parent->owner;
        obj->owner->flags |= OWNER_FLAG_BIT25;
        func_0202934c(&tmp, obj->owner);
        vec_set(&obj->center, &tmp);
        {
            Src *src = (Src *) data_0210930c.root->mid->p;
            if (*(u16 *) ((u8 *) src + 0x44) >= 1) {
                list = src->list;
                if (func_02067b18(list + 0x50) >= 2) {
                    for (i = 0; i < 2; i++) {
                        Vec3s *s = func_02067b0c(list + 0x50, i);
                        pv       = &v;
                        vec_init(pv, s->x << 12, s->y << 12, s->z << 12);
                        // FAKE: pts[i] self-check and copy as byte offsets (obj + i * 12) so mwcc keeps one offset IV
                        if ((Vec3 *) ((u8 *) obj->pts + (int) (i * 12)) != pv) {
                            Vec3 *d = ((Inner *) ((u8 *) obj + (int) (i * 12)))->pts; // FAKE: pun, see above
                            d->x    = pv->x;
                            d->y    = pv->y;
                            d->z    = pv->z;
                        }
                    }
                }
            }
        }
        obj->state = 0;
    }
    self->inner       = obj;
    self->owner->ctrl = self;
    // Virtual call through vtable slot 0x24.
    owner->action(0, 1, 1, 0);
    return self;
}
}
