#include "m2c.h"

typedef struct Vec3 {
    s32 x, y, z;
} Vec3;

typedef struct Node {
    Vec3 *head;   /* 0x00 */
    Vec3 data;    /* 0x04 */
    Vec3 offset;  /* 0x10 */
    s32 weight;   /* 0x1c */
    s32 max;      /* 0x20 */
    s32 state;    /* 0x24 */
} Node;

typedef struct Link {
    Node *node;         /* 0x00 */
    struct Link *next;  /* 0x04 */
    struct Link *prev;  /* 0x08 */
} Link;

typedef struct Owner {
    u8 pad0[0x240];
    u8 instant;         /* 0x240 */
    u8 pad241[0x394 - 0x241];
    u8 list[4];         /* 0x394 */
    Link *head;         /* 0x398 */
    u8 pad39c[0x3cc - 0x39c];
    Vec3 pos;           /* 0x3cc */
} Owner;

/* DS math coprocessor square-root unit: SQRTCNT at 0x040002B0, result at
 * +4, 64-bit parameter at +8. SQRTCNT bit 0 selects 64-bit mode, bit 15 is busy. */
typedef struct CPRegs {
    u16 mode;
    u16 pad;
    u32 result;
    s64 input;
} CPRegs;

enum {
    SQRT_REGS = 0x040002B0
};
enum {
    SQRT_MODE_64BIT = 1
};
enum {
    SQRT_BUSY = 0x8000
};
enum {
    FX_ONE = 0x1000
}; /* 1.0 in 20.12 fixed point (FX_Div result) */

typedef struct Fx {
    s32 v;
} Fx;

void func_02007f24(s32 *out, u32 lo, u32 hi);
void func_0205d4c8(void *list, Link *link);
void func_0205e84c(Node *node);
void func_020aa7c0(Owner *self, Node *node);
s32 FX_Div(s32, s32);

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

static inline Vec3 *vec_set_xyz(Vec3 *v, s32 x, s32 y, s32 z) {
    v->x = x;
    v->y = y;
    v->z = z;
    return v;
}

static inline s32 fx_mul_p(s32 a, const Fx *b) {
    return (s32) (((s64) a * b->v + 0x800) >> 12);
}

static inline s32 fx_mul(s32 a, Fx b) {
    return fx_mul_p(a, &b);
}

static inline void vec_madd(Vec3 *out, const Vec3 *a, const Vec3 *d, Fx t) {
    vec_set_xyz(out, a->x + fx_mul(d->x, t), a->y + fx_mul(d->y, t), a->z + fx_mul(d->z, t));
}

static inline s64 vec_sqx(const Vec3 *v) {
    return (s64) v->x * v->x;
}

// FAKE: total is a fake second parameter (the only caller, func_020a0630, passes self alone); as a parameter
// it ranks after the inline temps and before self, giving total r9 like the target (locals rank earlier)
void func_020aaad4(Owner *self, s32 total) {
    s32 locked = 0;
    Node *n;
    Link *it = self->head;
    Vec3 p;
    Vec3 zero;
    Vec3 d;
    vec_set(&self->pos, vec_set_xyz(&zero, 0, 0, 0));
    total = 0; // FAKE: the fake parameter is overwritten at once (its incoming r1 value is never used)
    if (it == NULL) {
        return;
    }
    do {
        n = it->node;
        if (n == NULL) {
            goto next;
        }
        if (n->state != 4) {
            if (n->weight < n->max) {
                if (self->instant) {
                    n->weight = n->max;
                } else {
                    s32 step;
                    /* 0x3fe99999a0000000 = (double) 0.8f, passed as lo/hi words */
                    func_02007f24(&step, 0xa0000000, 0x3fe99999);
                    n->weight += step;
                    if (n->weight >= n->max) {
                        n->weight = n->max;
                        if (n->state == 5) {
                            func_020aa7c0(self, n);
                        }
                    }
                }
            }
        } else {
            if (n->weight > 0) {
                s32 step;
                func_02007f24(&step, 0xa0000000, 0x3fe99999);
                n->weight -= step;
                if (n->weight <= 0) {
                    Link *prev = it->prev;
                    func_0205d4c8(self->list, it);
                    func_0205e84c(n);
                    if (prev != NULL) {
                        it = prev->next;
                    } else {
                        it = self->head;
                    }
                    continue;
                }
            }
        }
        if (n->state == 3 && n->weight == n->max) {
            locked = 1;
            vec_set(&self->pos, n->head);
        } else if (!locked && n->weight != 0) {
            Vec3 *h = n->head;
            p.x     = h->x;
            p.y     = h->y;
            p.z     = h->z;
            p.x += n->offset.x;
            p.y += n->offset.y;
            p.z += n->offset.z;
            if (total == 0) {
                vec_set(&self->pos, &p);
            } else {
                volatile CPRegs *cp = (CPRegs *) SQRT_REGS;
                Vec3 r;
                Fx t;
                s64 sq;
                d.x = p.x - self->pos.x;
                d.y = p.y - self->pos.y;
                d.z = p.z - self->pos.z;
                sq  = vec_sqx(&d);
                sq += (s64) d.y * d.y;
                sq += (s64) d.z * d.z;
                cp->mode  = SQRT_MODE_64BIT;
                cp->input = sq << 2;
                while (cp->mode & SQRT_BUSY) {
                }
                t.v = FX_ONE - FX_Div(total, total + n->weight);
                vec_madd(&r, &self->pos, &d, t);
                vec_set(&self->pos, &r);
            }
            total += n->weight;
        }
    next:
        it = it->next;
    } while (it != NULL);
}
