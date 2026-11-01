/* func_02076008: constructor (allocated 0x88 bytes by func_0205f574, case 0x32). Stores the base
 * vtable, the four caller arguments (+0x04..+0x10) and zeroes the base fields, then stores the derived
 * vtable and zeroes its own fields, initialises the sub-object at +0x54 and sets +0x86 to 10. It then
 * takes the target object (+0x6ac) of the entry data_02109358[data_021027f8[0x2a]], keeps it at +0x28
 * and copies its position (+0xb0, three s32) and angles (+0xc8, three u16) to +0x30 / +0x3c (the same
 * layout func_0207641c later writes back). Finally queues command 0x12 with no arguments on
 * data_02102900, calls func_02063a90(0, 0) and clears bit 15 of the hardware register at 0x04000304
 * (func_0207641c sets the same bit).
 */
#include "m2c.h"

enum {
    INIT_F86     = 10,
    CMD_12       = 0x12,
    POWCNT1_ADDR = 0x04000304,
    POWCNT1_SWAP = 0x8000,
};

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct {
    u16 v;
} Half;

typedef struct {
    Half x;
    Half y;
    Half z;
} Ang3;

typedef struct Obj {
    u8 pad0[0xB0];
    Vec3 pos;
    u8 pad1[0xC8 - 0xBC];
    Ang3 ang;
    u8 pad2[0x6AC - 0xCE];
    struct Obj *target;
} Obj;

typedef struct {
    void *vtbl;
    s32 a;
    s32 b;
    s32 c;
    s32 d;
    s32 f14;
    s32 f18;
    s32 f1c;
    s32 f20;
    s32 f24;
    Obj *obj;
    s32 f2c;
    Vec3 pos;
    Ang3 ang;
    s32 f44;
    s32 f48;
    s32 f4c;
    s32 f50;
    s32 sub;
    u8 pad[0x86 - 0x58];
    u16 f86;
} Ctx;

extern u8 data_020fe6d0[];
extern u8 data_020fed34[];
extern u8 data_021027f8[];
extern Obj *data_02109358[];
extern u8 data_02102900[];

void func_0206549c(s32 *);
void func_02060158(void *, s32, s32, s32);
void func_02063a90(s32, s32);

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

static inline void half_set(Half *d, Half v) {
    d->v = v.v;
}

Ctx *func_02076008(Ctx *self, s32 a, s32 b, s32 c, s32 d) {
    Obj *target;

    self->vtbl    = data_020fe6d0;
    self->a       = a;
    self->b       = b;
    self->c       = c;
    self->d       = d;
    self->f14     = 0;
    self->f18     = 0;
    self->f1c     = 0;
    self->f20     = 0;
    self->f24     = 0;
    self->vtbl    = data_020fed34;
    self->obj     = 0;
    self->f2c     = 0;
    self->pos.x   = 0;
    self->pos.y   = 0;
    self->pos.z   = 0;
    self->ang.x.v = 0;
    self->ang.y.v = 0;
    self->ang.z.v = 0;
    self->f44     = 0;
    self->f48     = 0;
    self->f4c     = 0;
    self->f50     = 0;
    func_0206549c(&self->sub);
    self->f86 = INIT_F86;
    self->obj = data_02109358[data_021027f8[0x2a]]->target;
    vec_set(&self->pos, &self->obj->pos);
    target = self->obj;
    half_set(&self->ang.x, target->ang.x);
    half_set(&self->ang.y, target->ang.y);
    half_set(&self->ang.z, target->ang.z);
    func_02060158(data_02102900, 0, CMD_12, 0);
    func_02063a90(0, 0);
    *(u16 *) POWCNT1_ADDR &= ~POWCNT1_SWAP;
    return self;
}
