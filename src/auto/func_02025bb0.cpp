#include "m2c.h"

// 20.12 fixed point (func_02007f24 converts a double by multiplying it by 4096.0).
enum {
    FX_SHIFT = 12,
    FX_HALF  = 0x800,
    FX_ONE   = 0x1000
};

// Countdown reload value (+0x1da) used when it is still zero in state 0.
enum {
    DEFAULT_PERIOD = 0x78
};

typedef struct {
    s32 v;
} UnkVal;

typedef struct {
    s32 v;
} Fx;

typedef struct {
    s32 x, y, z;
} Vec3;

// Partial interface: unknown virtuals retain their observed vtable slots.
// Declarations only: the original vtable remains in the ROM data.
class Obj {
public:
    virtual void unknown00();
    virtual void unknown04();
    virtual void unknown08();
    virtual void unknown0c();
    virtual void unknown10();
    virtual void unknown14();
    virtual void unknown18();
    virtual void unknown1c();
    virtual void unknown20();
    virtual void action(int, int, int, int); // Slot 0x24.
};

// *out = (s32) (v * 4096.0).
extern "C" void func_02007f24(Fx *out, f64 v);
extern "C" void func_02012744(void *obj, UnkVal arg);

extern "C" Vec3 data_020ff9b4;

static inline s32 fx_mul(s32 a, Fx b) {
    return (s32) (((s64) a * b.v + FX_HALF) >> FX_SHIFT);
}

// Store one component and return the vector, so the stores can be chained:
// vec_set_z(z, vec_set_y(y, vec_set_x(x, &v))) stores x, then y, then z.
static inline Vec3 *vec_set_x(s32 x, Vec3 *v) {
    v->x = x;
    return v;
}

static inline Vec3 *vec_set_y(s32 y, Vec3 *v) {
    v->y = y;
    return v;
}

static inline Vec3 *vec_set_z(s32 z, Vec3 *v) {
    v->z = z;
    return v;
}

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

static inline void vec_sub(Vec3 *d, const Vec3 *a, const Vec3 *b) {
    s32 z = a->z - b->z;
    s32 y = a->y - b->y;
    s32 x = a->x - b->x;
    d->x  = x;
    d->y  = y;
    d->z  = z;
}

static inline int vec_ne(const Vec3 *a, const Vec3 *b) {
    if (a->x != b->x || a->y != b->y || a->z != b->z) {
        return 1;
    }
    return 0;
}

static inline void *get_partner(Obj *self) {
    void *p = M2C_FIELD(self, void **, 0x224);
    if (p != 0) {
        p = (u8 *) p - 4;
    }
    return p;
}

static inline Vec3 *partner_vel(Obj *self) {
    void *p = get_partner(self);
    return (Vec3 *) ((u8 *) p + 0x60);
}

static inline Vec3 *partner_acc(Obj *self) {
    void *p = get_partner(self);
    return (Vec3 *) ((u8 *) p + 0x78);
}

// Two-state timer on self, then a sync with the partner object.
// State byte +0x1bc: state 0 defaults the period +0x1da to DEFAULT_PERIOD, loads
// the countdown +0x1be from it and enters state 1. States 1 and 2 decrement the
// countdown; when it was already <= 0 they swap (1 -> 2, 2 -> 1) and reload it.
// On that swap, if the u16 at +0x212 is 0xe5, the Vec3 at +0x60 becomes
// (0, FX_ONE, 0) * 0.5 (state 1) or (0, -FX_ONE, 0) * 0.5 (state 2); otherwise
// the slot-0x24 virtual gets (0x56, 0, 1, 0) (state 1) or (0x56, 0, 1, 1) (state 2).
// The partner is *(self + 0x224) minus 4 (NULL stays NULL); a = *(self + 0x20),
// b = *(partner + 0x20). If b's flag +0x26 is set, partner +0x60 =
// a's Vec3 at +4 - partner +0xb0. Else, if a's flag is set, a's Vec3 is copied to
// partner +0xb0 (setting partner +0xd1 = 1 when it differed) and to b +4, and
// partner +0x60 / +0x78 are set to data_020ff9b4. b's flag then takes a's flag,
// and func_02012744(self, arg1) runs last.
extern "C" void func_02025bb0(Obj *self, UnkVal arg1, ...) {
    void *a;
    void *b;

    switch (M2C_FIELD(self, u8 *, 0x1bc)) {
        case 0:
            if (M2C_FIELD(self, u8 *, 0x1da) == 0) {
                M2C_FIELD(self, u8 *, 0x1da) = DEFAULT_PERIOD;
            }
            M2C_FIELD(self, s16 *, 0x1be) = M2C_FIELD(self, u8 *, 0x1da);
            M2C_FIELD(self, u8 *, 0x1bc)  = 1;
            break;
        case 1:
            if (M2C_FIELD(self, s16 *, 0x1be)-- > 0) {
                break;
            }
            if (M2C_FIELD(self, u16 *, 0x212) == 0xe5) {
                Vec3 v;
                Fx s;
                v.x = 0;
                v.y = FX_ONE;
                v.z = 0;
                func_02007f24(&s, 0.5);
                vec_set_z(fx_mul(v.z, s), vec_set_y(fx_mul(v.y, s), vec_set_x(fx_mul(v.x, s), &v)));
                vec_set((Vec3 *) ((u8 *) self + 0x60), &v);
                M2C_FIELD(self, u8 *, 0x1bc)  = 2;
                M2C_FIELD(self, s16 *, 0x1be) = M2C_FIELD(self, u8 *, 0x1da);
            } else {
                M2C_FIELD(self, u8 *, 0x1bc)  = 2;
                M2C_FIELD(self, s16 *, 0x1be) = M2C_FIELD(self, u8 *, 0x1da);
                self->action(0x56, 0, 1, 0);
            }
            break;
        case 2:
            if (M2C_FIELD(self, s16 *, 0x1be)-- > 0) {
                break;
            }
            if (M2C_FIELD(self, u16 *, 0x212) == 0xe5) {
                Vec3 v;
                Fx s;
                v.x = 0;
                v.y = -FX_ONE;
                v.z = 0;
                func_02007f24(&s, 0.5);
                vec_set_z(fx_mul(v.z, s), vec_set_y(fx_mul(v.y, s), vec_set_x(fx_mul(v.x, s), &v)));
                vec_set((Vec3 *) ((u8 *) self + 0x60), &v);
                M2C_FIELD(self, u8 *, 0x1bc)  = 1;
                M2C_FIELD(self, s16 *, 0x1be) = M2C_FIELD(self, u8 *, 0x1da);
            } else {
                M2C_FIELD(self, u8 *, 0x1bc)  = 1;
                M2C_FIELD(self, s16 *, 0x1be) = M2C_FIELD(self, u8 *, 0x1da);
                self->action(0x56, 0, 1, 1);
            }
            break;
    }

    a = M2C_FIELD(self, void **, 0x20);
    b = M2C_FIELD(get_partner(self), void **, 0x20);
    if (M2C_FIELD(b, u8 *, 0x26) != 0) {
        void *p = get_partner(self);
        Vec3 d;
        vec_sub(&d, (Vec3 *) ((u8 *) a + 4), (Vec3 *) ((u8 *) p + 0xb0));
        vec_set(partner_vel(self), &d);
    } else if (M2C_FIELD(a, u8 *, 0x26) != 0) {
        void *p = get_partner(self);
        if (vec_ne((Vec3 *) ((u8 *) p + 0xb0), (Vec3 *) ((u8 *) a + 4))) {
            vec_set((Vec3 *) ((u8 *) p + 0xb0), (Vec3 *) ((u8 *) a + 4));
            M2C_FIELD(p, u8 *, 0xd1) = 1;
        }
        vec_set((Vec3 *) ((u8 *) b + 4), (Vec3 *) ((u8 *) a + 4));
        vec_set(partner_vel(self), &data_020ff9b4);
        vec_set(partner_acc(self), &data_020ff9b4);
    }
    M2C_FIELD(b, u8 *, 0x26) = M2C_FIELD(a, u8 *, 0x26);

    func_02012744(self, arg1);
}
