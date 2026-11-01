#include "m2c.h"

/* Calls an emitter on data_021093b0 with a position next to the actor and a unit direction.
 * 1. Actors of kinds 0xd6, 0x5d, 0x68, 0xd5, 0x5c, 0x67 and 0x184 do nothing.
 * 2. When func_02093fd0(func_020917b4(data_0210930c)) is 0x11, only kind 0x5b goes on.
 * 3. With an active mount (+0x20, active byte +0x26) the position is the mount's position (+0x04),
 *    raised by the mount word at +0x18 when func_02093fa8 of the same value is 0; the direction
 *    stays zero. Otherwise the position is the actor-local offset (0, 4.0, 2.0) (12-bit fixed point)
 *    rotated, scaled and moved to world space, and the direction is it minus the actor's position.
 * 4. The direction is normalised with the hardware divider and square root; a zero direction is
 *    replaced by the vector data_020ff9b4.
 * 5. Kind 0x5b calls func_02094bf0 (extra argument 3), every other kind func_02094b30. */
enum {
    FIXED_ONE        = 0x1000,
    OFFSET_Y         = 0x4000,
    OFFSET_Z         = 0x2000,
    RESTRICTED_MODE  = 0x11,
    KIND_ALT_EMITTER = 0x5b,
};

typedef struct {
    s32 x, y, z;
} Vec3;
typedef struct {
    s32 value;
} Fixed;
typedef struct {
    Fixed x, y, z;
} Scale;
typedef struct {
    u8 pad00[4];
    Vec3 position;
    u8 pad10[8];
    s32 height;
    u8 pad1c[0xa];
    u8 active;
} Mount;
typedef struct {
    u8 pad00[0x20];
    Mount *mount;
    u8 pad24[0x68];
    s32 matrix[9];
    Vec3 position;
    Scale scale;
    u16 rotation[4];
    u8 rotationDirty;
    u8 matrixDirty;
    u8 pad_d2[0x140];
    u16 kind;
} Actor;

extern u8 data_0210930c[];
extern const Vec3 data_020ff9b4;
extern u8 data_021093b0[];

extern void func_01ffa6f8(void *, const void *);
extern void func_01ffa958(Vec3 *, const void *, const Vec3 *);
extern u8 func_020917b4(void *arg0);
extern u8 func_02093fa8(s32 arg0);
extern u8 func_02093fd0(s32 arg0);
extern void func_02094b30(u8 *self, Vec3 *pos, Vec3 *src);
extern void func_02094bf0(u8 *self, Vec3 *pos, Vec3 *src, s32 flag);

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

static inline void vec_sub(const Vec3 *a, const Vec3 *b, Vec3 *d) {
    s32 z = a->z - b->z;
    s32 y = a->y - b->y;
    s32 x = a->x - b->x;
    d->x  = x;
    d->y  = y;
    d->z  = z;
}

static inline void vec_normalize(const Vec3 *src, Vec3 *dst) {
    s32 sqrt;
    s64 div;
    s64 t = (s64) ((const s32 *) src)[0] * ((const s32 *) src)[0];

    t += (s64) ((const s32 *) src)[1] * ((const s32 *) src)[1];
    t += (s64) ((const s32 *) src)[2] * ((const s32 *) src)[2];

    *(volatile u16 *) 0x04000280 = 2;
    *(volatile u64 *) 0x04000290 = 1LL << 56;
    *(volatile u64 *) 0x04000298 = t;

    *(volatile u16 *) 0x040002B0 = 1;
    *(u64 *) 0x040002B8          = t << 2;

    while (*(volatile u16 *) 0x040002B0 & 0x8000) {
    }
    sqrt = (s32) * (volatile u32 *) 0x040002B4;

    while (*(volatile u16 *) 0x04000280 & 0x8000) {
    }
    div = (s64) * (u64 *) 0x040002A0;

    div *= sqrt;

    dst->x = (s32) ((div * src->x + (1LL << 44)) >> 45);
    dst->y = (s32) ((div * src->y + (1LL << 44)) >> 45);
    dst->z = (s32) ((div * src->z + (1LL << 44)) >> 45);
}

static inline s32 multiplyFixed(s32 value, const Fixed *scale) {
    return (s32) (((s64) value * scale->value + 0x800) >> 12);
}

static inline Vec3 *worldOffset(Vec3 *result, Actor *actor, const Vec3 *offset) {
    Fixed x, y, z;
    if (actor->rotationDirty) {
        actor->matrixDirty   = 1;
        actor->rotationDirty = 0;
        func_01ffa6f8(actor->matrix, actor->rotation);
    }
    func_01ffa958(result, actor->matrix, offset);
    if (actor->scale.x.value != FIXED_ONE || actor->scale.y.value != FIXED_ONE || actor->scale.z.value != FIXED_ONE) {
        x         = actor->scale.x;
        result->x = multiplyFixed(result->x, &x);
        y         = actor->scale.y;
        result->y = multiplyFixed(result->y, &y);
        z         = actor->scale.z;
        result->z = multiplyFixed(result->z, &z);
    }
    result->x += actor->position.x;
    result->y += actor->position.y;
    result->z += actor->position.z;
    return result;
}

void func_02016248(Actor *self) {
    Vec3 offset;
    Vec3 pos;
    Vec3 dir;
    Mount *mount;
    u32 useMount;
    u16 kind = self->kind;

    if (kind == 0xd6 || kind == 0x5d || kind == 0x68 || kind == 0xd5 || kind == 0x5c || kind == 0x67 || kind == 0x184) {
        return;
    }
    if (func_02093fd0(func_020917b4(data_0210930c)) == RESTRICTED_MODE && self->kind != KIND_ALT_EMITTER) {
        return;
    }
    useMount = 0;
    offset.x = 0;
    offset.y = OFFSET_Y;
    offset.z = OFFSET_Z;
    pos.x    = 0;
    pos.y    = 0;
    pos.z    = 0;
    dir.x    = 0;
    dir.y    = 0;
    dir.z    = 0;
    mount    = self->mount;
    if (mount != NULL && mount->active != 0) {
        useMount = 1;
    }
    if (useMount) {
        vec_set(&pos, &mount->position);
        if (func_02093fa8(func_020917b4(data_0210930c)) == 0) {
            pos.y += mount->height;
        }
    } else {
        Vec3 world;
        Vec3 delta;
        vec_set(&pos, worldOffset(&world, self, &offset));
        vec_sub(&pos, &self->position, &delta);
        vec_set(&dir, &delta);
    }
    if (dir.x == 0 && dir.y == 0 && dir.z == 0) {
        vec_set(&dir, &data_020ff9b4);
    } else {
        vec_normalize(&dir, &dir);
    }
    if (self->kind == KIND_ALT_EMITTER) {
        func_02094bf0(data_021093b0, &pos, &dir, 3);
    } else {
        func_02094b30(data_021093b0, &pos, &dir);
    }
}
