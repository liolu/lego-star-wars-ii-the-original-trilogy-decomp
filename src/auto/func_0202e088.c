#include "m2c.h"

typedef struct {
    s32 x, y, z;
} Vec3;
typedef struct {
    s32 v;
} Fixed;
typedef struct {
    u16 v;
} Half;
typedef struct {
    s32 unk0;
    s32 distance;
} ProbeResult;

extern u8 data_0210930c[];
extern const Vec3 data_020ff9b4;

void *func_0202cb44(void *arg0);
void func_0202db74(Fixed *arg0, void *arg1);
void *func_02031f24(void *self, void *a1, void *a2, Fixed a3, s32 a4, s32 a5, Half a6);
void func_02032b44(void *a0, void *a1, Vec3 *a2, Fixed a3, u8 a4, s32 a5, s32 a6, s32 a7, Fixed a8);
void func_02044b58(void *arg0, void *arg1, void *arg2, s8 arg3, s32 arg4);
void func_0204f9bc(ProbeResult *out, void *a1, Vec3 *a2, Vec3 *a3, s32 a4);
void *func_0205e80c(s32, s32, s32);
s32 func_02062a7c(s32 arg0, s32 arg1);

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

static inline void vec_normalize(const Vec3 *src, Vec3 *dst) {
    s64 t = (s64) src->x * src->x;
    s32 sqrt;
    s64 div;

    t += (s64) src->y * src->y;
    t += (s64) src->z * src->z;

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

static inline s32 fx_mul(s32 a, s32 b) {
    return (s32) (((s64) a * b + 0x800) >> 12);
}

static inline void vec_scale3(Vec3 *v, Fixed sx, Fixed sy, Fixed sz) {
    v->x = fx_mul(v->x, sx.v);
    v->y = fx_mul(v->y, sy.v);
    v->z = fx_mul(v->z, sz.v);
}

static inline void vec_scale(Vec3 *v, const Fixed *s) {
    vec_scale3(v, *s, *s, *s);
}

enum {
    ACTOR_POSITION = 0xB0,   /* Vec3 position (Actor layout in caller func_0202e8e4) */
    STEP_DISTANCE  = 0x5000, /* 5.0 in fx12 */
    PROBE_RAISE    = 0x3000, /* 3.0 */
    PROBE_DROP     = 0xF000, /* 15.0 */
    MIN_DISTANCE   = 0x2000, /* 2.0 */
};

/* Step 5.0 units from self towards the actor returned by func_0202cb44(self + 0x5E0)
 * (fx12 positions at +0xB0) and spawn an object depending on what is there:
 * 1. If the word at data_0210930c + 0x3C is set, allocate a 0x28-byte object and
 *    initialise it with func_02044b58(obj, self, &other's position, 1, 0); done.
 * 2. Otherwise take the direction to the other actor (data_020ff9b4 when both
 *    positions coincide, else normalised with the hardware divider / square root),
 *    scale it by 5.0 and add self's position.
 * 3. Probe with func_0204f9bc (world at data_0210930c->+0x68->+0x384, flag 1) from
 *    12.0 below to 3.0 above that point (y axis).
 * 4. Probe distance < 2.0: allocate 0x24 bytes and run func_02031f24 with
 *    func_0202db74's value, func_02062a7c(20, 60), arg1 and 0x38E.
 *    Otherwise allocate 0x40 bytes and run func_02032b44 at self's position
 *    with 2.0 and -1.0. */
void func_0202e088(u8 *self, s32 arg1) {
    Vec3 otherPos;
    Vec3 dir;
    Vec3 to;
    Vec3 from;
    ProbeResult result;
    Fixed reach;
    Fixed speed;
    u8 *other;
    void *p;
    u16 pick;
    s32 dx, dy, dz;

    other      = func_0202cb44(self + 0x5E0);
    otherPos.x = ((Vec3 *) (other + ACTOR_POSITION))->x;
    otherPos.y = ((Vec3 *) (other + ACTOR_POSITION))->y;
    otherPos.z = ((Vec3 *) (other + ACTOR_POSITION))->z;
    if (*(s32 *) (data_0210930c + 0x3C) != 0) {
        p = func_0205e80c(0x28, 1, 4);
        if (p == 0) {
            return;
        }
        func_02044b58(p, self, &otherPos, 1, 0);
        return;
    }

    dz    = ((Vec3 *) (other + ACTOR_POSITION))->z - ((Vec3 *) (self + ACTOR_POSITION))->z;
    dy    = ((Vec3 *) (other + ACTOR_POSITION))->y - ((Vec3 *) (self + ACTOR_POSITION))->y;
    dx    = ((Vec3 *) (other + ACTOR_POSITION))->x - ((Vec3 *) (self + ACTOR_POSITION))->x;
    dir.x = dx;
    dir.y = dy;
    dir.z = dz;
    if (dx == 0 && dy == 0 && dz == 0) {
        vec_set(&dir, &data_020ff9b4);
    } else {
        vec_normalize(&dir, &dir);
    }
    speed.v = STEP_DISTANCE;
    vec_scale(&dir, &speed);
    dir.x += ((Vec3 *) (self + ACTOR_POSITION))->x;
    dir.y += ((Vec3 *) (self + ACTOR_POSITION))->y;
    dir.z += ((Vec3 *) (self + ACTOR_POSITION))->z;

    to.x   = dir.x;
    to.y   = dir.y + PROBE_RAISE;
    to.z   = dir.z;
    from.x = to.x;
    from.y = to.y - PROBE_DROP;
    from.z = to.z;
    func_0204f9bc(&result, *(void **) (*(u8 **) (data_0210930c + 0x68) + 0x384), &from, &to, 1);

    if (result.distance < MIN_DISTANCE) {
        Half h;
        pick = func_02062a7c(0x14, 0x3C);
        func_0202db74(&reach, self);
        p = func_0205e80c(0x24, 1, 4);
        if (p == 0) {
            return;
        }
        h.v = 0x38E;
        func_02031f24(p, self, other, reach, pick, arg1, h);
    } else {
        Fixed b, a;
        p = func_0205e80c(0x40, 1, 4);
        if (p == 0) {
            return;
        }
        b.v = -0x1000;
        a.v = 0x2000;
        func_02032b44(p, self, (Vec3 *) (self + ACTOR_POSITION), a, 1, 0, 1, 0, b);
    }
}
