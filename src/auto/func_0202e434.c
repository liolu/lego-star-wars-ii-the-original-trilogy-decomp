#include "m2c.h"

typedef struct {
    s32 x, y, z;
} Vec3;
typedef struct {
    s32 value;
} Fixed;
typedef struct {
    u16 value;
} Angle;
typedef struct {
    u8 pad_00[0x28];
    s32 f28;
    u8 pad_2c[0x84];
    Vec3 pos;
} Actor;

enum {
    QUARTER_TURN = 0x4000, /* 16-bit angle: 0x10000 = full turn (4096-entry sin/cos table) */
    FX32_SHIFT   = 12,     /* 20.12 fixed-point fraction bits */
};

/* sin/cos pairs indexed by (angle >> 4) * 2: [2i] = sin, [2i + 1] = cos */
extern const s16 data_020f88c4[];
extern s32 data_021019d8;

extern s32 func_0202cb44(void *);
extern void func_0201807c(Angle *, Actor *, Vec3 *, s32);
extern s32 func_02062a7c(s32, s32);
extern void func_020c8244(void *, s16, s16);
extern void func_01ffa958(Vec3 *, void *, Vec3 *);
extern s32 func_0202db94(Vec3 *, Vec3 *);
extern s32 func_0200a6c4(Vec3 *, Vec3 *, void *, s32, s32, s32);
extern void *func_0205e80c(s32, s32, s32);
extern void *func_02032b44(void *, Actor *, Vec3 *, Fixed, u8, s32, s32, s32, Fixed);
extern void func_0202dc70(Actor *, s32, s32);

static inline u16 angle_store(Angle *k, u16 v) {
    Angle b;
    *(u16 *) k  = v;
    *(u16 *) &b = v;
    return v;
}
static inline Angle angle_add(u16 a, u16 v) {
    {
        Angle r;
        r.value = a + v;
        return r;
    }
}
static inline Angle angle_sub(u16 a, u16 v) {
    {
        Angle r;
        r.value = a - v;
        return r;
    }
}
static inline void set_angle(Angle *d, Angle s) {
    d->value = s.value;
}

static inline void vec_add(Vec3 *out, const Vec3 *a, const Vec3 *b) {
    s32 z  = a->z + b->z;
    s32 y  = a->y + b->y;
    s32 x  = a->x + b->x;
    out->x = x;
    out->y = y;
    out->z = z;
}
static inline void make_matrix(void *m, Angle a) {
    s32 i2 = (a.value >> 4) * 2;
    func_020c8244(m, *(s16 *) ((s8 *) data_020f88c4 + i2 * 2), *(s16 *) ((s8 *) data_020f88c4 + (i2 + 1) * 2));
}

/*
 * Places a point a random 10..25 units (fx32) from the target object that
 * func_0202cb44 returns for arg0 + 0x5e0. The direction is the yaw that
 * func_0201807c computes between the two objects, plus or minus QUARTER_TURN
 * on a coin flip (func_02062a7c(lo, hi) returns a random integer in
 * [lo, hi]). The offset (0, 0, dist) is rotated by the 3x3 matrix built from
 * that yaw (func_01ffa958 multiplies a vector by it) and added to the target
 * position. If func_0202db94(from, pos) returns nonzero, arg0->f28 is set to
 * data_021019d8 + 1 and, when func_0200a6c4 also returns 0, a 0x40-byte
 * object from func_0205e80c is set up at the point by func_02032b44 (owner
 * arg0); a failed allocation just returns. Otherwise func_0202dc70(arg0, 2, 1)
 * runs.
 */
void func_0202e434(Actor *arg0) {
    Actor *target;
    Angle yaw;
    s32 m[9];
    Vec3 off;
    Vec3 pos;
    Vec3 from;
    s32 world[3];
    Vec3 *w;
    void *mem;

    target = (Actor *) func_0202cb44((u8 *) arg0 + 0x5e0);
    func_0201807c(&yaw, target, &arg0->pos, 0);
    if (func_02062a7c(0, 1) == 1) {
        Angle k;
        set_angle(&yaw, angle_add(yaw.value, angle_store(&k, QUARTER_TURN)));
    } else {
        Angle k;
        set_angle(&yaw, angle_sub(yaw.value, angle_store(&k, QUARTER_TURN)));
    }
    make_matrix(m, yaw);
    {
        s32 d = func_02062a7c(10, 25);
        off.x = 0;
        off.y = 0;
        off.z = d << FX32_SHIFT;
    }
    func_01ffa958((Vec3 *) world, m, &off);
    w = (Vec3 *) world; // FAKE: pointer alias (s32[3] viewed as Vec3) keeps &world in a register for the ldmib in vec_add
    vec_add(&pos, w, &target->pos);
    from.x = arg0->pos.x;
    from.y = arg0->pos.y;
    from.z = arg0->pos.z;
    if (func_0202db94(&from, &pos) != 0) {
        s32 p[2]  = {0, 0};
        arg0->f28 = data_021019d8 + 1;
        if (func_0200a6c4(&from, &pos, p, 2, 7, 1) == 0) {
            mem = func_0205e80c(0x40, 1, 4);
            if (mem == NULL) {
                return;
            }
            {
                Fixed limit, speed;
                limit.value = -0x1000;
                speed.value = 0x2000;
                func_02032b44(mem, arg0, &pos, speed, 1, 0, 6, 0, limit);
            }
            return;
        }
    }
    func_0202dc70(arg0, 2, 1);
}
