#include "m2c.h"

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec;

typedef struct {
    s32 value;
} Fixed;

typedef struct {
    s32 v;
} WordArg;

typedef struct {
    s16 x;
    s16 y;
    s16 z;
} SVec;

typedef struct {
    u8 pad0[0x60];
    Vec v60;
    u8 pad6c[0xb0 - 0x6c];
    Vec pos;
    u8 padbc[0xd1 - 0xbc];
    u8 dirty;
    u8 padd2[0xd4 - 0xd2];
    u8 moving;
    u8 padd5[0xd8 - 0xd5];
    Fixed speed;
    u8 path;
    u8 node;
    u8 mode;
    u8 forward;
} Obj;

extern Vec data_020ff9b4;
extern M2C_UNK data_0210930c;

s32 func_0200aef8(void *, void *, WordArg);
void func_02017bcc(Obj *, WordArg);
SVec *func_02067b0c(void *, s32);
u8 func_02067b18(void *);

static inline void VecSub(const Vec *a, const Vec *b, Vec *out) {
    s32 z  = a->z - b->z;
    s32 y  = a->y - b->y;
    s32 x  = a->x - b->x;
    out->x = x;
    out->y = y;
    out->z = z;
}

static inline void VecAdd(const Vec *a, const Vec *b, Vec *out) {
    s32 z  = a->z + b->z;
    s32 y  = a->y + b->y;
    s32 x  = a->x + b->x;
    out->x = x;
    out->y = y;
    out->z = z;
}

static inline s64 VecDot64(const Vec *a, const Vec *b) {
    s64 t = (s64) a->x * b->x;
    t += (s64) a->y * b->y;
    t += (s64) a->z * b->z;
    return t;
}

static inline void VecCopy(const Vec *src, Vec *dst) {
    if (dst != src) {
        dst->x = src->x;
        dst->y = src->y;
        dst->z = src->z;
    }
}

static inline s32 VecNe(const Vec *a, const Vec *b) {
    s32 ne = 0;
    if (!(a->x == b->x && a->y == b->y && a->z == b->z)) {
        ne = 1;
    }
    return ne;
}

static inline s32 multiplyFixed(s32 value, Fixed scale) {
    return (s32) (((s64) value * scale.value + 0x800) >> 12);
}

static inline void scaleVector(Vec *v, const Fixed *s) {
    Fixed x = *s;
    v->x    = multiplyFixed(v->x, x);
    {
        Fixed y = *s;
        v->y    = multiplyFixed(v->y, y);
        {
            Fixed z = *s;
            v->z    = multiplyFixed(v->z, z);
        }
    }
}

static inline void VecNormalize(Vec *v) {
    s64 t;
    s32 sqrt;
    s64 div;

    if (v->x == 0 && v->y == 0 && v->z == 0) {
        VecCopy(&data_020ff9b4, v);
        return;
    }
    t = VecDot64(v, v);

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

    v->x = (s32) ((div * v->x + (1LL << 44)) >> 45);
    v->y = (s32) ((div * v->y + (1LL << 44)) >> 45);
    v->z = (s32) ((div * v->z + (1LL << 44)) >> 45);
}

enum {
    ARRIVE_RADIUS = 0xa000, /* 10.0 in 20.12 fixed point, passed as the radius to func_0200aef8 */
};

/* Path follower update. Looks up the current node of path `path` (u8 at 0xdc) in the level's path
 * table, converts its s16 coordinates to 20.12 fixed point and, while `moving` is set, steps `pos`
 * towards it by `speed` along the normalised direction (marking `dirty` if pos changed). Once pos
 * is within ARRIVE_RADIUS of the node, `node` advances according to `mode`:
 *   0/3: wrap around the ends, 1/4: stop at the ends, 2/5: reverse direction at the ends.
 * Finally runs the common object update func_02017bcc. */
void func_0201a42c(Obj *self, WordArg arg1, ...) {
    WordArg radius;
    Vec target;
    Vec dir;
    Vec next;
    void *path;
    SVec *pt;

    radius.v = ARRIVE_RADIUS;
    path = (u8 *) M2C_FIELD(M2C_FIELD(M2C_FIELD(M2C_FIELD(&data_0210930c, void **, 0x68), void **, 0x38c), void **, 4), u8 **,
                            0x34) +
           self->path * 8;
    pt = func_02067b0c(path, self->node);
    {
        s32 z    = pt->z << 12;
        s32 y    = pt->y << 12;
        s32 x    = pt->x << 12;
        target.x = x;
        target.y = y;
        target.z = z;
    }
    if (self->moving) {
        VecSub(&target, &self->pos, &dir);
        VecNormalize(&dir);
        scaleVector(&dir, &self->speed);
        VecAdd(&self->pos, &dir, &next);
        if (VecNe(&self->pos, &next)) {
            VecCopy(&next, &self->pos);
            self->dirty = 1;
        }
        self->moving = 0;
    } else {
        VecCopy(&data_020ff9b4, &self->v60);
    }
    if (func_0200aef8(&target, &self->pos, radius) != 0) {
        switch (self->mode) {
            case 0:
            case 3:
                if (self->forward) {
                    self->node++;
                    if (self->node >= func_02067b18(path)) {
                        self->node = 0;
                    }
                } else {
                    if (--self->node == 0) {
                        self->node = func_02067b18(path) - 1;
                    }
                }
                break;
            case 1:
            case 4:
                if (self->forward) {
                    if (self->node < (u32) (func_02067b18(path) - 1)) {
                        self->node++;
                    }
                } else if (self->node != 0) {
                    self->node--;
                }
                break;
            case 2:
            case 5:
                if (self->forward) {
                    if (self->node < (u32) (func_02067b18(path) - 1)) {
                        self->node++;
                    } else {
                        self->node--;
                        self->forward = 0;
                    }
                } else if (self->node != 0) {
                    self->node--;
                } else {
                    self->node++;
                    self->forward = 1;
                }
                break;
        }
    }
    func_02017bcc(self, arg1);
}
