#include "m2c.h"

typedef struct {
    s32 x, y, z;
} Vec3;
typedef struct {
    s32 value;
} Fixed;
typedef struct {
    Fixed x, y, z;
} Scale;
struct Angle {
    u16 v;
    Angle(u16 a) {
        v = a;
    }
    Angle operator+(Angle o) const {
        return Angle(v + o.v);
    }
    Angle &operator=(Angle o) {
        v = o.v;
        return *this;
    }
};
struct Rot {
    Angle x, y, z;
    Rot(const Angle &ax, const Angle &ay, const Angle &az) :
        x(ax),
        y(ay),
        z(az) {}
};
typedef struct {
    s32 matrix[9];
    Vec3 position;
    Scale scale;
    Rot rotation;
    u16 pad42;
    u8 rotationDirty;
    u8 matrixDirty;
} Transform;

typedef struct {
    Vec3 pos;
    u8 flagC;
    u8 padD[2];
    u8 kindF;
    u8 pad10[0x1c - 0x10];
    Rot rot;
} Def;

// Partial interface: declarations only; the vtable stays in ROM data.
class Obj {
public:
    virtual void unknown00();
    virtual void unknown04();
    virtual void unknown08();
    virtual void unknown0c();
    virtual void init(s32 kind, Def *def, Transform *xf); // Slot 0x10.

    Vec3 pos;
    u8 pad10[0x24 - 0x10];
    s8 b24;
    u8 pad25;
    u8 b26;
    u8 pad27;
    Rot rot;
    u8 pad2e[0x34 - 0x2e];
    Angle yaw;
};

typedef struct {
    s8 v;
} Byte;

enum {
    FX_ONE       = 0x1000,
    FX_HALF      = 0x800,
    FX_FRAC_BITS = 12,
};

extern "C" void func_01ffa6f8(s32 *, Rot *);
extern "C" void func_01ffa958(Vec3 *, const void *, const Vec3 *);
extern "C" Obj *func_020082a4(void *arg0, s32 arg1, s32 arg2);
extern "C" void func_0204c594(Obj *o, Vec3 *src);

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

static inline s32 multiplyFixed(s32 value, Fixed scale) {
    return (s32) (((s64) value * scale.value + FX_HALF) >> FX_FRAC_BITS);
}

static inline Vec3 *worldOffset(Vec3 *result, Transform *xf, const Vec3 *offset) {
    if (xf->rotationDirty) {
        xf->matrixDirty   = 1;
        xf->rotationDirty = 0;
        func_01ffa6f8(xf->matrix, &xf->rotation);
    }
    func_01ffa958(result, xf->matrix, offset);
    if (xf->scale.x.value != FX_ONE || xf->scale.y.value != FX_ONE || xf->scale.z.value != FX_ONE) {
        result->x = multiplyFixed(result->x, xf->scale.x);
        result->y = multiplyFixed(result->y, xf->scale.y);
        result->z = multiplyFixed(result->z, xf->scale.z);
    }
    result->x += xf->position.x;
    result->y += xf->position.y;
    result->z += xf->position.z;
    return result;
}

extern "C" Obj *func_02008614(void *arg0, s32 arg1, Byte arg2, Def *def, Transform *xf, ...) {
    Obj *obj = 0;
    Vec3 v;
    Vec3 p0, p5, p3, p11;

    switch (arg1) {
        case 0:
            obj = func_020082a4(arg0, arg1, def->kindF);
            vec_set(&obj->pos, worldOffset(&p0, xf, &def->pos));
            {
            // Summed z, y, x: the operator+ argument copies take stack slots in this order.
                Angle az = def->rot.z + xf->rotation.z;
                Angle ay = def->rot.y + xf->rotation.y;
                Angle ax = def->rot.x + xf->rotation.x;
                Rot q(ax, ay, az);
                obj->rot.x = q.x;
                obj->rot.y = q.y;
                obj->rot.z = q.z;
            }
            obj->init(arg1, def, xf);
            break;
        case 5:
            obj = func_020082a4(arg0, arg1, def->kindF);
            vec_set(&obj->pos, worldOffset(&p5, xf, &def->pos));
            obj->init(arg1, def, xf);
            if (!((xf->rotation.y.v - 0x2000) & 0x4000)) {
                // FAKE: byte-offset pun: s32 word reads at +0x30/+0x2c/+0x28 overlap the declared Angle rot (+0x28..0x2d)
                // and pad fields; they only give the target's three ldr (field types here unknown, not a scheduling trick).
                v.x = M2C_FIELD(obj, s32 *, 0x30);
                v.y = M2C_FIELD(obj, s32 *, 0x2c);
                v.z = M2C_FIELD(obj, s32 *, 0x28);
                func_0204c594(obj, &v);
            }
            break;
        case 3:
            obj = func_020082a4(arg0, arg1, def->kindF);
            vec_set(&obj->pos, worldOffset(&p3, xf, &def->pos));
            obj->init(arg1, def, xf);
            break;
        case 11:
            obj = func_020082a4(arg0, arg1, def->kindF);
            vec_set(&obj->pos, worldOffset(&p11, xf, &def->pos));
            obj->yaw = def->rot.x + xf->rotation.y;
            obj->init(arg1, def, xf);
            break;
    }
    obj->b26 = def->flagC != 0;
    obj->b24 = arg2.v;
    return obj;
}
