#include "m2c.h"

typedef struct Vec3 {
    s32 x, y, z;
} Vec3;

typedef struct Vec3s {
    s16 x, y, z;
} Vec3s;

typedef struct Fx {
    s32 v;
} Fx;

typedef struct Obj {
    u8 pad0[0x28];
    Vec3 cur_pos;              /* 0x28 */
    u8 pad34[0x21c - 0x34];
    void *points;             /* 0x21c */
    void *alt_points;         /* 0x220 */
    Vec3 apply_pos;            /* 0x224 */
    Vec3 path_pos;               /* 0x230 */
    u8 placed;                /* 0x23c */
    u8 use_path_pos;                /* 0x23d */
    u8 pad23e[0x293 - 0x23e];
    u8 near_idx;
    u8 next_idx;
    u8 lo_idx;
    u8 hi_idx;
    u8 next_valid;
    u8 near_valid;
    u8 pad299[3];
    Vec3 v29c;              /* 0x29c */
    s32 s2a8;
    s32 s2ac;
    Vec3 path_offset;            /* 0x2b0 */
    u8 pad2bc[0x2d4 - 0x2bc];
    u8 b2d4;
    u8 pad2d5[0x374 - 0x2d5];
    void *p374;
    u8 pad378[0x3d9 - 0x378];
    u8 b3d9;
} Obj;

/* All-zero Vec3 in .rodata. */
extern const Vec3 data_020ff9b4;

enum {
    FX_SHIFT = 12,
};   /* s16 point coordinates -> 20.12 fixed point */

Vec3s *func_02067b48(void *, s32);
u8 func_02067b54(void *);
void func_020a3c80(void *, Vec3 *, Vec3, s32, s32);
void func_020a59dc(void *, u8 *, u8 *, Fx *, Vec3 *);
void func_020a5d88(void *, void *, Vec3 *, u8 *, Vec3 *, Vec3 *, Fx *, Vec3 *);
void func_020a6a18(void *, Vec3 *, Vec3 *, Vec3 *, Fx *);
void func_020a6a9c(Fx *, void *, Vec3 *, Vec3 *, Vec3 *);
void func_020a6bb4(void *, void *, Vec3 *, Vec3 *, Vec3 *, u8, u8);
void func_020a6cac(void *, Vec3 *, Fx, Vec3, Vec3, void *);
void func_020a9634(void *, Vec3 *);
s32 func_020a96c8(void *, u32, Vec3 *);
s32 func_020a9804(void *, Vec3 *);
void func_020aa574(void *, u8, u8, u8 *, u8 *);
void func_020aa794(void *, Vec3 *);

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

/* Widens an s16 point (entries of the list read by func_02067b48) to a
 * fixed-point Vec3 in d and returns d. */
static inline Vec3 *point_to_fx(Vec3 *d, const Vec3s *s) {
    s32 z = s->z << FX_SHIFT;
    s32 y = s->y << FX_SHIFT;
    s32 x = s->x << FX_SHIFT;
    d->x  = x;
    d->y  = y;
    d->z  = z;
    return d;
}

static inline Vec3 *get_cur_pos(Obj *o) {
    return &o->cur_pos;
}

/* Same copy as vec_set, but s->x is read before the alias test. */
static inline void vec_set2(Vec3 *d, const Vec3 *s) {
    s32 x = s->x;
    if (d != s) {
        d->x = x;
        d->y = s->y;
        d->z = s->z;
    }
}

/* Stages either the path position or the current position and hands it to
 * func_020a9634 (which writes it to cur_pos and raises the flag at 0x49 when
 * it changed), then marks the object as placed. */
static inline void apply_path_pos(Obj *self) {
    if (self->use_path_pos) {
        vec_set(&self->apply_pos, &self->path_pos);
    } else {
        vec_set(&self->apply_pos, get_cur_pos(self));
    }
    func_020a9634(self, &self->apply_pos);
    self->placed = 1;
}

/* Per-frame path update.
 * 1. Run func_020a3c80 on v29c and fetch the reference position (func_020aa794).
 * 2. Once placed, re-apply path_pos.
 * 3. Cache the nearest point index (func_020a9804) and the next index
 *    (func_020a96c8) the first time through.
 * 4. A list with at most one point: snap path_pos to that point and place
 *    the object once.
 * 5. Otherwise refresh both indices (func_020aa574), sort them into
 *    lo_idx/hi_idx, sample the segment on points (or alt_points when set),
 *    add path_offset, store the result in path_pos and place the object once. */
void func_020a542c(Obj *self) {
    Vec3 ref_pos;
    Vec3 ref_pos2;
    Vec3 seg_a;
    Vec3 seg_b;
    Vec3 seg_c;
    Vec3 alt_c;
    Vec3 alt_a;
    Vec3 alt_b;
    Vec3 result;
    Vec3 snap;
    Fx t;
    u8 ok;
    Vec3s *point;

    func_020a3c80(self, &self->v29c, data_020ff9b4, self->s2a8, self->s2ac);
    ref_pos.x = 0;
    ref_pos.y = 0;
    ref_pos.z = 0;
    func_020aa794(self, &ref_pos);
    if (self->placed) {
        vec_set2(&self->apply_pos, &self->path_pos);
        func_020a9634(self, &self->apply_pos);
    }
    if (self->near_valid == 0) {
        self->near_idx   = func_020a9804(self, &ref_pos);
        self->near_valid = 1;
    }
    if (self->next_valid == 0 && self->near_valid != 0) {
        self->next_idx   = func_020a96c8(self, self->near_idx, &ref_pos);
        self->next_valid = 1;
    }
    if (func_02067b54(self->points) <= 1) {
        point            = func_02067b48(self->points, self->near_idx);
        self->next_valid = 0;
        if (self->placed) {
            return;
        }
        vec_set(&self->path_pos, point_to_fx(&snap, point));
        apply_path_pos(self);
        return;
    }
    func_020aa574(self, self->near_idx, self->next_idx, &self->near_idx, &self->next_idx);
    self->lo_idx = self->near_idx;
    self->hi_idx = self->next_idx;
    if (self->lo_idx > self->hi_idx) {
        u8 tmp       = self->lo_idx;
        self->lo_idx = self->hi_idx;
        self->hi_idx = tmp;
    }
    ref_pos2.x = 0;
    ref_pos2.y = 0;
    ref_pos2.z = 0;
    func_020aa794(self, &ref_pos2);
    t.v = 0;
    if (self->b3d9) {
        func_020a59dc(self, &self->lo_idx, &self->hi_idx, &t, &ref_pos2);
    }
    seg_a.x = 0;
    seg_a.y = 0;
    seg_a.z = 0;
    seg_b.x = 0;
    seg_b.y = 0;
    seg_b.z = 0;
    seg_c.x = 0;
    seg_c.y = 0;
    seg_c.z = 0;
    func_020a6bb4(self, self->points, &seg_a, &seg_b, &seg_c, self->lo_idx, self->hi_idx);
    if (self->b3d9 == 0) {
        Fx r;
        func_020a6a9c(&r, self, &seg_a, &ref_pos2, &seg_c);
        t = r;
    }
    alt_c.x = 0;
    alt_c.y = 0;
    alt_c.z = 0;
    alt_a.x = 0;
    alt_a.y = 0;
    alt_a.z = 0;
    alt_b.x = 0;
    alt_b.y = 0;
    alt_b.z = 0;
    if (self->alt_points) {
        func_020a6bb4(self, self->alt_points, &alt_a, &alt_b, &alt_c, self->lo_idx, self->hi_idx);
    }
    result.x = 0;
    result.y = 0;
    result.z = 0;
    if (self->alt_points == 0) {
        func_020a6a18(self, &result, &seg_a, &seg_c, &t);
    } else {
        func_020a6a18(self, &result, &alt_a, &alt_c, &t);
    }
    ok = 1;
    if (self->p374) {
        if (self->alt_points == 0) {
            func_020a5d88(self, self->points, &result, &ok, &seg_a, &seg_b, &t, &seg_c);
        } else {
            func_020a5d88(self, self->alt_points, &result, &ok, &alt_a, &alt_b, &t, &alt_c);
        }
    }
    if (self->b2d4 && ok) {
        if (self->alt_points == 0) {
            func_020a6cac(self, &result, t, seg_a, seg_b, self->points);
        } else {
            func_020a6cac(self, &result, t, alt_a, alt_b, self->alt_points);
        }
    }
    result.x += self->path_offset.x;
    result.y += self->path_offset.y;
    result.z += self->path_offset.z;
    vec_set(&self->path_pos, &result);
    if (self->placed) {
        return;
    }
    apply_path_pos(self);
}
