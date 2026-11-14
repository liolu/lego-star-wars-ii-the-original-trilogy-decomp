#include "m2c.h"

typedef struct {
    s32 v;
} WordArg;

typedef struct {
    s32 v;
} Fx;

typedef struct {
    Fx x;
    Fx y;
    Fx z;
} VecFx;

typedef struct {
    u16 v;
} Ang;

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

extern s32 func_0200aef8(void *, void *, WordArg);
extern s32 func_020296a4(void *, void *, u32);
extern s32 func_02029894(void *, void *);
extern void func_0202aca4(void *, void *, void *, s32);
extern s32 func_0202adf8(void *, void *);
extern void func_0204a5f8(void *, u16 *);
extern u16 func_0204fa30(void *);
extern s16 func_020985d8(s32, s32);

static inline void vec_copy(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

static inline void vec_setd(VecFx *v, s32 x, s32 y, s32 z) {
    s32 *p = &v->x.v;

    /* Plain field stores let mwcc substitute the inline arguments into the stores. */
    // FAKE: pointer-walk stores keep the args as temps, computed z, y, x and stored x, y, z as in the target
    *p++ = x;
    *p++ = y;
    *p   = z;
}

static inline void vec_sub(VecFx *d, const Vec3 *a, const Vec3 *b) {
    vec_setd(d, a->x - b->x, a->y - b->y, a->z - b->z);
}

static inline u16 fx_atan2(Fx x, Fx z) {
    return func_020985d8(x.v, z.v);
}

/* func_020985d8 is a 16-bit atan2: its octant returns put 0x2000 at 45 degrees and 0x4000 at
 * 90 degrees, so 0x10000 is a full turn. */
enum {
    ANG_QUARTER_TURN = 0x4000
};

/* Absolute signed 16-bit difference between two yaws. */
static inline s32 ang_dist(Ang b, u16 a) {
    u16 diff = a - b.v;
    s32 ad   = (s16) diff;
    if (ad < 0) {
        ad = -ad;
    }
    return ad;
}

static inline Ang ang_get(const Ang *p) {
    return *p;
}

/* Nonzero when the yaws of d1 and d2 differ by at least a quarter turn. */
static inline s32 turned_away(VecFx *d1, VecFx *d2) {
    Fx qx2;
    Fx qz2;
    Fx qx1;
    Fx qz1;
    Ang e;

    qz2 = d2->z;
    qx2 = d2->x;
    e.v = fx_atan2(qx2, qz2);
    qz1 = d1->z;
    qx1 = d1->x;
    return ang_dist(ang_get(&e), fx_atan2(qx1, qz1)) >= ANG_QUARTER_TURN;
}

static inline void *link_owner(u8 *s) {
    u8 *p = *(u8 **) (s + 0x14);
    if (p != NULL) {
        p -= 4;
    }
    return p;
}

static inline s32 sub_linked(u8 *s) {
    s32 r = 0;

    if (s[0] != 0 && s[1] != 0 && link_owner(s) != NULL) {
        r = 1;
    }
    return r;
}

static inline Vec3 *node_pos(void *self) {
    u8 idx   = M2C_FIELD((u8 *) self + M2C_FIELD(self, u8 *, 0x56) * 8, u8 *, 4);
    u8 *node = (**(u8 ****) self)[idx];
    return (Vec3 *) (node + 0x14);
}

static inline void face_point(void *ctx, const Vec3 *pos, const Vec3 *target) {
    VecFx d;
    Fx qx;
    Fx qz;
    u16 yaw;

    vec_sub(&d, target, pos);
    /* Set bit 0x40 in the flags at +0x5ce, then store the newly set bits (+0x5d2) and the newly
     * cleared bits (+0x5d4) relative to the flags at +0x5d0. */
    M2C_FIELD(ctx, u16 *, 0x5ce) |= 0x40;
    M2C_FIELD(ctx, u16 *, 0x5d2) =
        M2C_FIELD(ctx, u16 *, 0x5ce) & (M2C_FIELD(ctx, u16 *, 0x5ce) ^ M2C_FIELD(ctx, u16 *, 0x5d0));
    M2C_FIELD(ctx, u16 *, 0x5d4) =
        M2C_FIELD(ctx, u16 *, 0x5d0) & (M2C_FIELD(ctx, u16 *, 0x5ce) ^ M2C_FIELD(ctx, u16 *, 0x5d0));
    qz  = d.z;
    qx  = d.x;
    yaw = fx_atan2(qx, qz);
    /* func_0204a5f8 stores the yaw at ctx+0x5dc and sets the byte at ctx+0x5db to 1. */
    func_0204a5f8((u8 *) ctx + 0x5ce, &yaw);
}

/* Steps ctx (position at +0xb0) along the 8-byte entry list in self (index +0x56, count +0x55).
 * When the current entry must be followed, the point at self+0x64 is updated and ctx is turned
 * toward it through face_point (yaw from func_020985d8). At the last entry (or with +0x90 set),
 * self+0x64 takes self+0x70 when the direction to self+0x70 differs from the direction to the
 * current entry's position by at least a quarter turn. Bytes +0x8c, +0x90 and +0x91 of self are
 * state flags whose meaning is not established. */
void func_02029acc(void *self, void *ctx) {
    s32 near;
    s32 ok;
    s32 go;
    Vec3 *pos;
    WordArg range;
    u8 bits;
    u32 mask;

    pos = (Vec3 *) ((u8 *) ctx + 0xb0);
    if (M2C_FIELD(ctx, u8 *, 0x40) != 0 && !sub_linked((u8 *) ctx + 0x40) && func_0204fa30((u8 *) ctx + 0x40) != 5) {
        vec_copy((Vec3 *) ((u8 *) ctx + 0x6a0), pos);
    }
    range.v = 0x8000;
    near    = func_0200aef8(pos, (u8 *) self + 0x70, range);
    if (M2C_FIELD(self, u8 *, 0x8f) != 0 || M2C_FIELD(self, u8 *, 0x54) == 0) {
        func_0202aca4(self, pos, (u8 *) self + 0x70, M2C_FIELD(ctx, s32 *, 0x1b8));
        M2C_FIELD(self, u8 *, 0x8f) = 0;
    }
    if (func_02029894(self, ctx) != 0) {
        return;
    }
    if (M2C_FIELD(self, u8 *, 0x54) != 0) {
        ok = 1;
        go = M2C_FIELD(self, u8 *, 0x90);
        if (go != 0 ||
            // FAKE: re-reads +0x90 (already in go); this redundant second test forces the target's dead bne
            (M2C_FIELD(self, u8 *, 0x90) == 0 && M2C_FIELD(self, u8 *, 0x56) >= M2C_FIELD(self, u8 *, 0x55) - 1U &&
             *M2C_FIELD((u8 *) self + M2C_FIELD(self, u8 *, 0x56) * 8, u8 **, 8) == 1)) {
            VecFx d1;
            VecFx d2;

            vec_sub(&d1, node_pos(self), pos);
            vec_sub(&d2, (Vec3 *) ((u8 *) self + 0x70), pos);
            if (go == 0) {
                go = turned_away(&d1, &d2);
            }
            if (go != 0) {
                vec_copy((Vec3 *) ((u8 *) self + 0x64), (Vec3 *) ((u8 *) self + 0x70));
                if (near == 0) {
                    face_point(ctx, pos, (Vec3 *) ((u8 *) self + 0x64));
                    return;
                }
                M2C_FIELD(self, u8 *, 0x90) = 1;
                M2C_FIELD(self, u8 *, 0x8c) = 0;
                return;
            }
        }
        if (M2C_FIELD(self, u8 *, 0x90) == 0) {
            if (func_0202adf8(self, pos) == 0) {
                M2C_FIELD(self, u8 *, 0x90) = 1;
            }
            if (M2C_FIELD(self, u8 *, 0x91) == 0) {
                bits = *M2C_FIELD((u8 *) self + M2C_FIELD(self, u8 *, 0x56) * 8, u8 **, 8);
                mask = bits & M2C_FIELD(ctx, s32 *, 0x1b8);
                if (bits & 0x40) {
                    ok = 0;
                } else if (mask & ~1) {
                    if (func_020296a4(self, ctx, mask) != 0) {
                        M2C_FIELD(self, u8 *, 0x91) = 1;
                    } else {
                        ok = 0;
                    }
                } else {
                    M2C_FIELD(self, u8 *, 0x91) = 1;
                }
            }
        }
        if (M2C_FIELD(self, u8 *, 0x90) != 0) {
            return;
        }
        if (ok != 0) {
            vec_copy((Vec3 *) ((u8 *) self + 0x64), node_pos(self));
        }
        if (ok == 0) {
            return;
        }
        if (near != 0) {
            M2C_FIELD(self, u8 *, 0x90) = 1;
            M2C_FIELD(self, u8 *, 0x8c) = 0;
        }
        face_point(ctx, pos, (Vec3 *) ((u8 *) self + 0x64));
    } else {
        M2C_FIELD(self, u8 *, 0x8c) = 0;
    }
}
