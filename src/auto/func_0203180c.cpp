#include "m2c.h"
struct Fixed {
    s32 value;
};
struct Vec3 {
    Fixed x, y, z;
};
struct Angle {
    u16 value;
    Angle() {}
    Angle(u16 v) :
        value(v) {}
};
class Actor {
public:
    virtual void v00();
    virtual void v04();
    virtual void v08();
    virtual void v0c();
    virtual void v10();
    virtual void v14();
    virtual void v18();
    virtual void v1c();
    virtual void v20();
    virtual void v24();
    virtual void v28();
    virtual void v2c();
    virtual void v30();
    virtual void v34();
    virtual void v38();
    virtual void v3c();
    virtual void v40();
    virtual s32 v44();
    u8 pad4[4];
    u32 flags;
    u16 kind;
    u8 pade[0x76];
    s32 state;
    u8 pad88[0x28];
    Vec3 pos;
    u8 padbc[0xe];
    Angle yaw;
};
struct Controller {
    u32 pad0;
    Actor *actor;
};
struct Node {
    Actor *actor;
    Node *next;
};
struct List {
    u32 pad0;
    Node *head;
    u8 pad8[0x30];
};
extern "C" s32 data_027e02a4;
extern "C" u8 data_021019f4[];
extern "C" s32 func_0202d7f8(void *, s32, void *, s32);
extern "C" s16 func_020985d8(s32, s32);

enum {
    REG_SQRTCNT          = 0x040002b0,
    REG_SQRT_RESULT      = 0x040002b4,
    REG_SQRT_PARAM       = 0x040002b8,
    SQRTCNT_64BIT        = 1,
    SQRTCNT_BUSY         = 0x8000,
    ACTOR_FLAGS_SKIP     = 0x802,
    ACTOR_KIND_STATEFUL  = 4,
    ACTOR_STATE_REQUIRED = 0x10,
    YAW_LIMIT_15_DEG     = 0xaaa,
};

/* Angle (0x10000 = full turn) of the horizontal offset (x, z). */
static inline Angle Atan2(const Fixed &z, const Fixed &x) {
    return Angle(func_020985d8(x.value, z.value));
}
/* Negated angle of y over the horizontal length h. */
static inline Angle Elevation(Fixed y, s32 h) {
    return Angle(-(u16) func_020985d8(y.value, h));
}
static inline void VecSub(Vec3 *d, const Vec3 *a, const Vec3 *b) {
    d->z.value = a->z.value - b->z.value;
    d->y.value = a->y.value - b->y.value;
    d->x.value = a->x.value - b->x.value;
}
/* |a - b| as a signed 16-bit angle difference, below limit. */
static inline bool Within(Angle a, Angle b, Angle limit) {
    s32 d = (s16) (u16) (a.value - b.value);
    if (d < 0) {
        d = -d;
    }
    return (u16) d < limit.value;
}
/* sqrt(x*x + z*z) for fx32 x, z via the SQRT unit: the 64-bit square sum is
   shifted to 26 fraction bits, the 32-bit root rounded back to 12. */
static inline s32 HorizontalLength(s32 x, s32 z) {
    s64 t = (s64) x * x;
    t += (s64) z * z;
    *(volatile u16 *) REG_SQRTCNT    = SQRTCNT_64BIT;
    *(volatile u64 *) REG_SQRT_PARAM = t << 2;
    while (*(volatile u16 *) REG_SQRTCNT & SQRTCNT_BUSY) {
    }
    return (s32) (*(volatile u32 *) REG_SQRT_RESULT + 1) >> 1;
}
/*
 * Returns the nearest actor in the current list (data_021019f4 entry
 * data_027e02a4, 0x38-byte records) whose kind bits intersect mask, that has
 * no ACTOR_FLAGS_SKIP flag and lies within radius (whole units, compared on
 * squared distance of the >>12 position offsets) of controller->actor.
 * Kinds with bits 0-1 must also pass func_0202d7f8 (given both actors' v44
 * results); kind 4 must be in state 0x10. With angles set, the actor must also
 * be within 15 degrees of the controller's yaw and its elevation within limit
 * of pitch. Returns NULL when none qualifies.
 */
extern "C" Actor *func_0203180c(Controller *controller, u32 mask, s32 radius, bool angles, Angle pitch, Angle limit) {
    Actor *best = 0;
    s32 nearest = radius * radius;
    s32 tag     = controller->actor->v44();
    Node *node;
    s32 distance;
    for (node = ((List *) (data_021019f4 + data_027e02a4 * 0x38))->head; node; node = node->next) {
        Actor *actor = node->actor;
        u32 kind     = actor->kind;
        if (!(mask & kind) || (actor->flags & ACTOR_FLAGS_SKIP)) {
            continue;
        }
        if (kind & 3) {
            if (func_0202d7f8(controller->actor, tag, actor, actor->v44())) {
                goto valid;
            }
            continue;
        }
        if (kind == ACTOR_KIND_STATEFUL && actor->state != ACTOR_STATE_REQUIRED) {
            continue;
        }
    valid: {
        Vec3 d;
        VecSub(&d, &actor->pos, &controller->actor->pos);
        distance = (d.x.value >> 12) * (d.x.value >> 12) + (d.y.value >> 12) * (d.y.value >> 12) +
                   (d.z.value >> 12) * (d.z.value >> 12);
        if (distance >= nearest) {
            continue;
        }
        if (angles) {
            Fixed dz = d.z;
            Fixed dx = d.x;
            if (!Within(controller->actor->yaw, Atan2(dz, dx), Angle(YAW_LIMIT_15_DEG))) {
                continue;
            }
            if (!Within(pitch, Elevation(d.y, HorizontalLength(d.x.value, d.z.value)), limit)) {
                continue;
            }
        }
        nearest = distance;
        best    = actor;
    }
    }
    return best;
}
