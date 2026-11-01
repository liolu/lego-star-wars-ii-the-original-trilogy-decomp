#include "m2c.h"

typedef struct {
    s32 v;
} UnkVal;

typedef struct {
    s32 x, y, z;
} Vec3;

extern void func_02012744(void *, UnkVal);
extern s32 func_02017800(s32 arg0, s32 arg1);
extern void func_0209f948(void *, UnkVal);

enum {
    FX_SHIFT = 12,
};

/* Passes an integer step to func_0209f948 on the sub-object at +0xd4,
 * converted to 20.12 fixed point. */
static inline void set_step(void *owner, s32 step) {
    UnkVal t;
    t.v = step << FX_SHIFT;
    func_0209f948((u8 *) owner + 0xd4, t);
}

static inline s32 vec_ne(const Vec3 *a, const Vec3 *b) {
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

static inline void vec_sub(Vec3 *out, const Vec3 *a, const Vec3 *b) {
    s32 z  = a->z - b->z;
    s32 y  = a->y - b->y;
    s32 x  = a->x - b->x;
    out->x = x;
    out->y = y;
    out->z = z;
}

static inline void set_pos(void *obj, const Vec3 *pos) {
    if (vec_ne((Vec3 *) ((u8 *) obj + 0xb0), pos)) {
        vec_set((Vec3 *) ((u8 *) obj + 0xb0), pos);
        M2C_FIELD(obj, u8 *, 0xd1) = 1;
    }
}

static inline s32 is_set(void *p) {
    return p != (void *) 0;
}

/* Update step driven by the state byte at +0x1bc:
 *   0: switch to state 1.
 *   1: while the halfword +0x1be of the linked object (+0x22c) differs from
 *      the cached copy at +0x220, raise the counter +0x218 by one up to the
 *      limit +0x21c and give the object returned by func_02017800(self, 0)
 *      the complement (its +0x21c minus our counter); refresh the cache. When
 *      they already agree, clear +0x224, the halfword and the cache.
 *   3: move the counter one step toward +0x234; on arrival return to state 1
 *      and clear the same fields as above.
 * Each counter change is pushed via set_step. Then func_02012744 runs with
 * the forwarded argument, and the linked object is synced: with a reference
 * object at +0x3c its Vec3 at +0x60 becomes ref(+4) - link(+0xb0); otherwise
 * its position +0xb0 copies ours and byte +0xd1 is set when it changed.
 * Finally the counter is pushed to the object at +0x230. */
void func_020224f4(void *self, UnkVal arg) {
    void *other;
    void *ref;
    void *link;
    Vec3 offset;

    switch (M2C_FIELD(self, u8 *, 0x1bc)) {
        case 0:
            M2C_FIELD(self, u8 *, 0x1bc) = 1;
            break;
        case 1:
            if (M2C_FIELD(self, s32 *, 0x220) != M2C_FIELD(M2C_FIELD(self, void **, 0x22c), s16 *, 0x1be)) {
                if (M2C_FIELD(self, s32 *, 0x218) < M2C_FIELD(self, s32 *, 0x21c)) {
                    M2C_FIELD(self, s32 *, 0x218)++;
                    set_step(self, M2C_FIELD(self, s32 *, 0x218));
                    other                          = (void *) func_02017800((s32) self, 0);
                    M2C_FIELD(other, s32 *, 0x218) = M2C_FIELD(other, s32 *, 0x21c) - M2C_FIELD(self, s32 *, 0x218);
                    set_step(other, M2C_FIELD(other, s32 *, 0x218));
                }
                M2C_FIELD(self, s32 *, 0x220) = M2C_FIELD(M2C_FIELD(self, void **, 0x22c), s16 *, 0x1be);
            } else {
                M2C_FIELD(self, s32 *, 0x224)                            = 0;
                M2C_FIELD(M2C_FIELD(self, void **, 0x22c), s16 *, 0x1be) = 0;
                M2C_FIELD(self, s32 *, 0x220)                            = 0;
            }
            break;
        case 3:
            if (M2C_FIELD(self, s32 *, 0x218) < M2C_FIELD(self, s32 *, 0x234)) {
                M2C_FIELD(self, s32 *, 0x218)++;
                set_step(self, M2C_FIELD(self, s32 *, 0x218));
            } else if (M2C_FIELD(self, s32 *, 0x218) > M2C_FIELD(self, s32 *, 0x234)) {
                M2C_FIELD(self, s32 *, 0x218)--;
                set_step(self, M2C_FIELD(self, s32 *, 0x218));
            } else if (M2C_FIELD(self, s32 *, 0x218) == M2C_FIELD(self, s32 *, 0x234)) {
                set_step(self, M2C_FIELD(self, s32 *, 0x218));
                M2C_FIELD(self, u8 *, 0x1bc)                             = 1;
                M2C_FIELD(self, s32 *, 0x224)                            = 0;
                M2C_FIELD(M2C_FIELD(self, void **, 0x22c), s16 *, 0x1be) = 0;
                M2C_FIELD(self, s32 *, 0x220)                            = 0;
            }
            break;
    }
    func_02012744(self, arg);
    ref = M2C_FIELD(self, void **, 0x3c);
    if (is_set(ref)) {
        Vec3 *dst;
        link = M2C_FIELD(self, void **, 0x22c);
        vec_sub(&offset, (Vec3 *) ((u8 *) ref + 4), (Vec3 *) ((u8 *) link + 0xb0));
        dst = (Vec3 *) ((u8 *) link + 0x60);
        vec_set(dst, &offset);
    } else {
        void *link_obj;
        Vec3 *pos;
        link_obj = M2C_FIELD(self, void **, 0x22c);
        pos      = (Vec3 *) ((u8 *) self + 0xb0);
        set_pos(link_obj, pos);
    }
    set_step(M2C_FIELD(self, void **, 0x230), M2C_FIELD(self, s32 *, 0x218));
}
