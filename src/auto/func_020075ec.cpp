#include "m2c.h"

// Input step for an object: bits of the input halfword turn the
// heading by a fixed 4-degree step in either direction, and move the
// position along the heading by a distance taken from virtual slot 0x3c.
// Flag 0x200 is cleared while moving and set again when no move bit is held.
// The heading is finally copied to the field at 0x6b4.

// 20.12 fixed-point value.
struct Fx {
    s32 v;
    Fx() {}
    Fx(s32 x) :
        v(x) {}
};

// Binary angle: 0x10000 = 360 degrees.
typedef struct {
    u16 v;
} Ang;

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

// Result of the slot-0x3c virtual; the non-trivial copy constructor makes
// the compiler return it through a hidden pointer in r0 (this in r1).
class Dist {
public:
    s32 v;
    Dist(const Dist &other);
};

struct Obj {
    u8 pad4[4];
    u32 flags;
    u8 padc[0x60 - 0xc];
    Vec3 pos;
    u8 pad6c[0xca - 0x6c];
    Ang heading;
    u8 padcc[0xd0 - 0xcc];
    u8 heading_changed;
    u8 padd1[0x182 - 0xd1];
    u16 f182;
    u8 pad184[0x5ce - 0x184];
    u16 input_bits;
    u8 pad5d0[0x6b4 - 0x5d0];
    Ang heading_copy;
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void v6();
    virtual void v7();
    virtual void v8();
    virtual void v9();
    virtual void v10();
    virtual void v11();
    virtual void v12();
    virtual void v13();
    virtual void v14();
    virtual Dist get_move_dist(u16 arg); // Slot 0x3c.
};

extern "C" {
extern s32 data_020ff9a8;
extern const s16 data_020f88c4[]; // Interleaved {sin, cos} table, 20.12.
}

enum {
    INPUT_TURN_INC = 0x20,
    INPUT_TURN_DEC = 0x10,
    INPUT_MOVE_POS = 0x40,
    INPUT_MOVE_NEG = 0x80,
    FLAG_NOT_MOVED = 0x200,
    TURN_STEP_DEG  = 0x4000, // 4.0 in 20.12.
    DEG_TO_ANG     = 0xb6, // 65536 / 360.
};

static inline s32 fx_mul(Fx a, s32 b) {
    return (s32) (((s64) a.v * b + 0x800) >> 12);
}

static inline void ang_from_deg(Ang *out, Fx deg) {
    out->v = fx_mul(deg, DEG_TO_ANG);
}

static inline void ang_add_to(Ang *out, u16 base, Ang d) {
    out->v = base + d.v;
}

static inline void ang_sub_to(Ang *out, u16 base, Ang d) {
    out->v = base - d.v;
}

static inline void obj_set_heading_copy(Obj *self, Ang a) {
    self->heading_copy = a;
}

static inline void obj_set_heading(Obj *self, Ang a) {
    if (self->heading.v != a.v) {
        self->heading.v       = a.v;
        self->heading_changed = 1;
    }
}

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

// Rotate v in the x/z plane: x' = x*cos + z*sin, z' = z*cos - x*sin.
static inline void vec_rot_y(Vec3 *v, u16 ang) {
    s32 s;
    s32 c;
    s32 x;
    s32 z;
    s32 nx;
    s32 t;

    s  = data_020f88c4[(ang >> 4) * 2];
    c  = data_020f88c4[(ang >> 4) * 2 + 1];
    x  = v->x;
    z  = v->z;
    t  = (s32) (((s64) z * s + 0x800) >> 12);
    nx = (s32) (((s64) x * c + 0x800) >> 12);
    nx += t;
    t    = (s32) (((s64) x * s + 0x800) >> 12);
    t    = (s32) (((s64) z * c + 0x800) >> 12) - t;
    v->x = nx;
    v->z = t;
}

extern "C" void func_020075ec(Obj *self) {
    s32 moved;

    if (self->input_bits & INPUT_TURN_INC) {
        Ang turned;
        Ang step;

        ang_from_deg(&step, Fx(TURN_STEP_DEG));
        ang_add_to(&turned, self->heading.v, step);
        obj_set_heading(self, turned);
    }
    if (self->input_bits & INPUT_TURN_DEC) {
        Ang turned;
        Ang step;

        ang_from_deg(&step, Fx(TURN_STEP_DEG));
        ang_sub_to(&turned, self->heading.v, step);
        obj_set_heading(self, turned);
    }
    moved = 0;
    if (self->input_bits & INPUT_MOVE_POS) {
        Vec3 v; // Offset in the object frame, rotated into position.

        moved = 1;
        self->flags &= ~FLAG_NOT_MOVED;
        Dist dist = self->get_move_dist(self->f182);

        v.x = data_020ff9a8;
        v.y = self->pos.y;
        v.z = dist.v;
        vec_rot_y(&v, self->heading.v);
        vec_set(&self->pos, &v);
    } else if (self->input_bits & INPUT_MOVE_NEG) {
        Vec3 v; // Offset in the object frame, rotated into position.

        moved = 1;
        self->flags &= ~FLAG_NOT_MOVED;
        Dist dist = self->get_move_dist(self->f182);

        v.x = data_020ff9a8;
        v.y = self->pos.y;
        v.z = -dist.v;
        vec_rot_y(&v, self->heading.v);
        vec_set(&self->pos, &v);
    }
    if (!moved) {
        self->flags |= FLAG_NOT_MOVED;
    }
    obj_set_heading_copy(self, self->heading);
}
