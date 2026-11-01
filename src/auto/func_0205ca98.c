#include "m2c.h"

/* Places one attached target (targets[req->slot - FIRST_SLOT]) relative to a transform.
 * Sets the target's active byte from req->flag. Rebuilds the transform's matrix first when its
 * rotationDirty byte is set (func_01ffa6f8 from the rotation; matrixDirty is then set).
 * First slot: position = matrix * (req->offset scaled by xf->scale, func_0204a124);
 * the rotation is left alone.
 * Other slots: position = (matrix * req->offset), scaled per axis by xf->scale (20.12 fixed point,
 * skipped when all three are FX_ONE), plus xf->position; rotation = req->rotation + xf->rotation,
 * per axis in 16 bits.
 */

typedef struct {
    s32 x, y, z;
} Vec3;
typedef struct {
    u16 v;
} Angle;
typedef struct {
    Angle x, y, z;
} Rot3;
typedef struct {
    s32 matrix[9];
    Vec3 position;
    Vec3 scale;
    Rot3 rotation;
    u16 pad;
    u8 rotationDirty;
    u8 matrixDirty;
} Transform;
typedef struct {
    Vec3 offset;
    u8 flag;
    u8 unknown0d[2];
    u8 slot;
    u8 unknown10[0xc];
    Rot3 rotation;
} Request;
typedef struct {
    u8 unknown00[4];
    Vec3 position;
    u8 unknown10[0x16];
    u8 active;
    u8 unknown27;
    Rot3 rotation;
} Target;

enum {
    FIRST_SLOT   = 2,
    FX_ONE       = 0x1000,
    FX_HALF      = 0x800,
    FX_FRAC_BITS = 12,
};

extern void func_01ffa6f8(s32 *, Rot3 *);
extern void func_01ffa958(Vec3 *, const void *, const Vec3 *);
extern void func_0204a124(Vec3 *, const Vec3 *, const Vec3 *);

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

typedef struct {
    s32 value;
} Fixed;

static inline s32 multiplyFixed(s32 value, const Fixed *scale) {
    return (s32) (((s64) value * scale->value + FX_HALF) >> FX_FRAC_BITS);
}

static inline void updateMatrix(Transform *xf) {
    if (xf->rotationDirty) {
        xf->matrixDirty   = 1;
        xf->rotationDirty = 0;
        func_01ffa6f8(xf->matrix, &xf->rotation);
    }
}

static inline Vec3 *scaledOffset(Vec3 *result, Vec3 *tmp, Transform *xf, const Vec3 *offset) {
    updateMatrix(xf);
    func_0204a124(tmp, offset, &xf->scale);
    func_01ffa958(result, xf, tmp);
    return result;
}

static inline Vec3 *worldOffset(Vec3 *result, Transform *xf, const Vec3 *offset) {
    Fixed x, y, z;
    updateMatrix(xf);
    func_01ffa958(result, xf, offset);
    if (xf->scale.x != FX_ONE || xf->scale.y != FX_ONE || xf->scale.z != FX_ONE) {
        x         = *(Fixed *) &xf->scale.x;
        result->x = multiplyFixed(result->x, &x);
        y         = *(Fixed *) &xf->scale.y;
        result->y = multiplyFixed(result->y, &y);
        z         = *(Fixed *) &xf->scale.z;
        result->z = multiplyFixed(result->z, &z);
    }
    result->x += xf->position.x;
    result->y += xf->position.y;
    result->z += xf->position.z;
    return result;
}

static inline void rotAdd3(Rot3 *r, const Rot3 *a, const Rot3 *pb) {
    Rot3 b;
    /* copied z, y, x: this order gives the target's register assignment */
    b.z    = pb->z;
    b.y    = pb->y;
    b.x    = pb->x;
    r->x.v = a->x.v + b.x.v;
    r->y.v = a->y.v + b.y.v;
    r->z.v = a->z.v + b.z.v;
}

static inline void angle_set(Angle *d, Angle v) {
    d->v = v.v;
}

void func_0205ca98(Target **targets, Request *req, Transform *xf) {
    s32 idx = req->slot - FIRST_SLOT;
    Target *t;
    Vec3 pos0;
    Vec3 tmp;
    Vec3 pos1;
    Rot3 sum;

    targets[idx]->active = req->flag != 0;
    if (idx == 0) {
        scaledOffset(&pos0, &tmp, xf, &req->offset);
        vec_set(&targets[idx]->position, &pos0);
        return;
    }
    {
        Vec3 *from = worldOffset(&pos1, xf, &req->offset);
        vec_set(&targets[idx]->position, from);
    }
    rotAdd3(&sum, &req->rotation, &xf->rotation);
    t = targets[idx];
    angle_set(&t->rotation.x, sum.x);
    angle_set(&t->rotation.y, sum.y);
    angle_set(&t->rotation.z, sum.z);
}
