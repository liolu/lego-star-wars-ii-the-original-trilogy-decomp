#include "m2c.h"

typedef volatile u16 vu16;
typedef volatile u32 vu32;

typedef struct {
    u16 v;
} H;

typedef struct {
    s32 v;
} Fx;

typedef struct {
    Fx x;
    Fx y;
    Fx z;
} Vec3;

typedef struct {
    u8 pad0[0xb0];
    Vec3 pos;
    u8 padbc[0xe];
    H yaw;
} Ent;

typedef struct {
    u8 pad0[0xc8];
    H pitch;
    H yaw;
    u8 pad[4];
    u8 dirty;
} State;

typedef struct {
    Ent *a;
    u32 pad4;
    u32 *link;
    u32 pad;
    u32 pad2;
    State *st;
    u32 pad3;
    s32 count;
} Ctl;

extern void func_02028db8(Ctl *);
extern void func_02028ea8(Ctl *, Ent *, Ent *);
extern void func_02028f04(Ctl *);
extern void func_020290e0(H *, Ent *, Ent *);
extern s16 func_020985d8(s32, s32);
extern H data_020fdbb0;

static inline Ent *linked(Ctl *c) {
    Ent *e = (Ent *) c->link;
    if (e) {
        e = (Ent *) ((u8 *) e - 4);
    }
    return e;
}

static inline void set_yaw(State *s, H y) {
    if (s->yaw.v != y.v) {
        s->yaw.v = y.v;
        s->dirty = 1;
    }
}

static inline void set_pitch(State *s, H p) {
    if (s->pitch.v != p.v) {
        s->pitch.v = p.v;
        s->dirty   = 1;
    }
}

static inline Vec3 *vec_sub(Vec3 *o, const Vec3 *p, const Vec3 *q) {
    s32 z  = p->z.v - q->z.v;
    s32 y  = p->y.v - q->y.v;
    s32 x  = p->x.v - q->x.v;
    o->x.v = x;
    o->y.v = y;
    o->z.v = z;
    return o;
}

/* NDS hardware square-root unit. */
enum {
    REG_SQRTCNT        = 0x040002b0,
    REG_SQRT_RESULT    = 0x040002b4,
    REG_SQRT_PARAM     = 0x040002b8,
    SQRTCNT_MODE_64BIT = 1,
    SQRTCNT_BUSY       = 0x8000
};

/* Square root of v on the hardware unit: 64-bit input v << 2, waits for the
 * busy bit to clear, then rounds the result with (r + 1) >> 1. */
static inline s32 fx_sqrt(s64 v) {
    *(vu16 *) REG_SQRTCNT   = SQRTCNT_MODE_64BIT;
    *(u64 *) REG_SQRT_PARAM = v << 2;
    while (*(vu16 *) REG_SQRTCNT & SQRTCNT_BUSY) {
    }
    return ((s32) * (vu32 *) REG_SQRT_RESULT + 1) >> 1;
}

static inline s16 atan2fx(Fx y, s32 x) {
    return func_020985d8(y.v, x);
}

static inline H ang_make(u16 v) {
    H r;
    r.v = v;
    return r;
}

static inline int ang_lt(H a, H b) {
    return a.v < b.v;
}

static inline H *ang_set(H *o, u16 v) {
    o->v = v;
    return o;
}

/* The compare is unsigned (the target emits cmp/rsblo against 0), so the
 * negating branch never runs: the result is always a. */
static inline H *ang_abs(H *out, H a) {
    H z = ang_make(0);
    if (ang_lt(a, z)) {
        return ang_set(out, -a.v);
    }
    return ang_set(out, a.v);
}

static inline H *yaw_to(H *out, Ent *from, Ent *to) {
    func_020290e0(out, from, to);
    return out;
}

static inline void ang_sub(H *out, const H *a, H b) {
    if (a) {
        out->v = a->v - b.v;
    }
}

static inline void face_diff(H *out, H *tmp, Ctl *c) {
    H y = linked(c)->yaw;
    ang_sub(out, yaw_to(tmp, linked(c), c->a), y);
}

static inline void calc_pitch(H *out, const Vec3 *v) {
    s64 t = (s64) v->x.v * v->x.v;
    t += (s64) v->z.v * v->z.v;
    out->v = -(u16) atan2fx(v->y, fx_sqrt(t));
}

/*
 * Refreshes c->st from the entities c->a and linked(c) (c->link minus 4 bytes).
 * - Creates c->st with func_02028db8 when it is NULL.
 * - Copies linked(c)->yaw into st->yaw and stores
 *   -func_020985d8(d.y, sqrt(d.x * d.x + d.z * d.z)), d = c->a->pos -
 *   linked(c)->pos, into st->pitch; each changed field sets st->dirty = 1.
 * - Returns if c->count > 0. Else diff = func_020290e0(linked(c), c->a) -
 *   linked(c)->yaw; if diff is not above data_020fdbb0, calls
 *   func_02028f04(c), then func_02028ea8(c, linked(c), c->a).
 */
void func_02028be8(Ctl *c) {
    Vec3 d;
    H pitch;
    H ad;
    H diff;
    H tmp;

    if (c->st == 0) {
        func_02028db8(c);
    }
    set_yaw(c->st, linked(c)->yaw);
    calc_pitch(&pitch, vec_sub(&d, &c->a->pos, &linked(c)->pos));
    set_pitch(c->st, pitch);
    if (c->count > 0) {
        return;
    }
    face_diff(&diff, &tmp, c);
    if (ang_abs(&ad, diff)->v > data_020fdbb0.v) {
        return;
    }
    func_02028f04(c);
    func_02028ea8(c, linked(c), c->a);
}
