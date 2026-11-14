#include "m2c.h"

typedef struct Vec3 {
    s32 x, y, z;
} Vec3;

struct Ang {
    Ang() {}
    u16 v;
};

typedef struct Fx {
    s32 v;
} Fx;

typedef struct Rot {
    Ang x, y, z;
} Rot;

// Partial interface: unknown virtuals retain their observed vtable slots.
// Declarations only: the original vtable remains in the ROM data.
class Entity {
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

    u8 pad4[4];
    u32 flags;
    u8 padC[0x60 - 0xC];
    Vec3 vel;
    u8 pad6C[0xB0 - 0x6C];
    Vec3 pos;
    u8 padBC[0xC8 - 0xBC];
    Rot rot;
    u8 padCE[2];
    u8 rot_dirty;
    u8 pos_dirty;
    u8 padD2[0xF4 - 0xD2];
    u8 sub[0x2C];
    void *link;
    u8 pad124[0x1A8 - 0x124];
    u16 field_1a8;
    u8 pad1AA[0x212 - 0x1AA];
    u16 kind;
    u8 pad214[0x5D6 - 0x214];
    u8 busy;
    u8 pad5D7[3];
    u8 player;
    u8 pad5DB[0x6AC - 0x5DB];
    Entity *target;
    u8 pad6B0[0x6BA - 0x6B0];
    u8 field_6ba;
};

typedef struct Msg {
    u8 pad0[4];
    Entity *obj;
    u8 pad8[4];
    u8 flags_c;
    u8 state;
    u8 padE[2];
    Entity *target;
    u8 field_14;
    s8 field_15;
    u8 pad16[2];
    s32 pitch;
    s32 yaw;
    s32 goal_pitch;
    s32 goal_yaw;
    s32 speed;
    u8 pad2C[0x3C - 0x2C];
    u8 active;
    u8 timer;
    u8 pad3E[2];
    Vec3 pos;
    u8 field_4c;
    u8 field_4d;
} Msg;

typedef struct Globals {
    u8 pad[0x2A];
    u8 field_2a;
} Globals;

typedef struct Cam {
    u8 pad[0x340];
    Vec3 offset;
} Cam;

typedef struct Player {
    u8 pad[4];
    s32 id;
} Player;

typedef struct Session {
    u8 pad[4];
    Player *player;
} Session;

extern "C" {

extern Globals data_021027f8;
extern Session data_02102900;
extern u32 data_027e02a8;
extern u8 data_0210794c;

void func_02007f24(s32 *out, double d);
void func_0200c834(Entity *, s32);
void func_0204b29c(Msg *);
void func_0204b4b8(Msg *, Entity *);
void func_0204bd88(void *);
u32 func_0205d060(Globals *, s32, s32);
void func_020a99bc(Cam *, s32);
u8 func_020aae08(Cam *, s32);
Cam *func_020aaeb4(void);
void func_020b04f0(void *, s32, void *, s32);
void func_020c3c34(void *, void (*)(void *), s32, s8, s32);
s32 FX_Sqrt(s32);
s32 FX_Div(s32, s32);

static inline int vec_differs(const Vec3 *a, const Vec3 *b) {
    if (a->x != b->x || a->y != b->y || a->z != b->z) {
        return 1;
    }
    return 0;
}

static inline void vec_set(Vec3 *dst, const Vec3 *src) {
    if (dst != src) {
        dst->x = src->x;
        dst->y = src->y;
        dst->z = src->z;
    }
}

static inline bool ang_eq(const Ang *a, Ang b) {
    return a->v == b.v;
}

static inline void ang_set(Ang *a, Ang b) {
    a->v = b.v;
}

static inline int rot_eq(const Rot *a, const Rot *b) {
    return ang_eq(&a->x, b->x) && ang_eq(&a->y, b->y) && ang_eq(&a->z, b->z);
}

static inline void rot_set(Rot *a, const Rot *b) {
    ang_set(&a->x, b->x);
    ang_set(&a->y, b->y);
    ang_set(&a->z, b->z);
}

static inline void set_pos(Entity *e, const Vec3 *p) {
    if (vec_differs(&e->pos, p)) {
        vec_set(&e->pos, p);
        e->pos_dirty = 1;
    }
}

static inline void set_vel(Entity *e, const Vec3 *v) {
    vec_set(&e->vel, v);
}

static inline void set_rot(Entity *e, const Rot *r) {
    if (!rot_eq(r, &e->rot)) {
        ang_set(&e->rot.x, r->x);
        ang_set(&e->rot.y, r->y);
        ang_set(&e->rot.z, r->z);
        e->rot_dirty = 1;
    }
}

static inline void set_rot_x(Entity *e, Ang a) {
    if (e->rot.x.v != a.v) {
        e->rot.x.v   = a.v;
        e->rot_dirty = 1;
    }
}

static inline void set_rot_y(Entity *e, Ang a) {
    if (e->rot.y.v != a.v) {
        e->rot.y.v   = a.v;
        e->rot_dirty = 1;
    }
}

static inline s32 ang_to_deg(u16 a) {
    return (s32) (((s64) a * 0x1680000 + 0x80000) >> 20);
}

static inline Ang deg_to_ang(s32 d) {
    Ang a;
    a.v = (u16) (((s64) d * 0xB60B60B60BLL + 0x80000000000LL) >> 44);
    return a;
}

static inline s32 fx_mul(s32 a, s32 b) {
    return (s32) (((s64) a * b + 0x800) >> 12);
}

static inline s32 fx_val(const Fx *f) {
    return f->v;
}

static inline s32 fx_scale(s32 a, Fx s) {
    return fx_mul(a, fx_val(&s));
}

static inline Fx to_fx(s32 v) {
    Fx f;
    f.v = v;
    return f;
}

static inline s32 fx_sqrt(Fx a) {
    return FX_Sqrt(a.v);
}

// FAKE: same sqrt as length_for_normalize but with a named Fx local; sets the Fx stack slots at +0x734/+0x78c
static inline s32 length_precheck(s32 value) {
    Fx length = to_fx(value);
    return fx_sqrt(length);
}

// FAKE: second spelling of length_precheck without the local (see there)
static inline s32 length_for_normalize(s32 value) {
    return fx_sqrt(to_fx(value));
}

static inline Fx reciprocal_length(s32 length) {
    return to_fx(FX_Div(0x1000, length));
}

static inline Fx speed_value(s32 value) {
    return to_fx(value);
}

static inline Fx speed_factor(s32 value) {
    return speed_value(value);
}

static inline s32 scale_component(s32 value, Fx factor) {
    return fx_scale(value, factor);
}

static inline void normalize_delta(s32 *dx, s32 *dy, s32 sq) {
    s32 len = length_for_normalize(sq);
    if (len > 0) {
        Fx inv = reciprocal_length(len);
        *dx    = fx_scale(*dx, inv);
        *dy    = fx_scale(*dy, inv);
    } else {
        *dy = *dx = 0;
    }
}

static inline void scale_delta(s32 *dx, s32 *dy, s32 speed) {
    // dx goes through extra wrappers (speed_factor, scale_component), dy calls fx_scale directly.
    // FAKE: the mixed inline depth of the two scalings sets the Fx stack slot at +0x7c0
    Fx spd = speed_factor(speed);
    *dx    = scale_component(*dx, spd);
    *dy    = fx_scale(*dy, spd);
}

static inline void apply_speed(s32 *dx, s32 *dy, s32 speed) {
    scale_delta(dx, dy, speed);
}

static inline u32 pressed(Entity *e, s32 mask) {
    return func_0205d060(&data_021027f8, mask, e->player);
}

static inline s32 player_id(void) {
    Player *p = data_02102900.player;
    if (p != NULL) {
        return p->id;
    }
    return 0;
}

static inline void cam_set_offset(const Vec3 *v) {
    Cam *cam = func_020aaeb4();
    vec_set(&cam->offset, v);
}

static inline void set_rot_x_deg(Entity *e, s32 d) {
    set_rot_x(e, deg_to_ang(d));
}

static inline void set_rot_y_deg(Entity *e, s32 d) {
    set_rot_y(e, deg_to_ang(d));
}

static inline s32 len_sq2(s32 x, s32 y) {
    return fx_mul(x, x) + fx_mul(y, y);
}
void func_0204a938(Msg *arg0) {
    u8 buf[28];
    Vec3 zero;
    Vec3 *src;
    Entity *obj;
    s32 accel;
    s32 dx;
    s32 dy;
    s32 pitch;
    s32 yaw;
    s32 goal_pitch;
    s32 goal_yaw;

    if ((arg0->target != NULL && (arg0->target->flags & 2)) || (arg0->obj->flags & 0x802)) {
        func_0204b29c(arg0);
        return;
    }
    obj = arg0->obj;
    switch (arg0->state) {
        case 0:
            if (obj->target != NULL) {
                arg0->target = obj->target;
            } else {
                arg0->flags_c |= 1;
                return;
            }
            func_0200c834(obj, 5);
            func_0200c834(obj, 4);
            obj->field_6ba = 1;
            arg0->obj->flags |= 0x800000;
            arg0->obj->flags |= 0x2000000;
            obj->flags |= 0x2000;
            arg0->state = 1;
            src         = &obj->pos;
            vec_set(&arg0->pos, src);
            set_pos(obj, &arg0->target->pos);
        // FAKE: set_rot open-coded one inline level shallower; forces the target's Ang stack-slot order
            {
                Entity *tgt = arg0->target;
                if (!rot_eq(&tgt->rot, &obj->rot)) {
                    rot_set(&obj->rot, &tgt->rot);
                    obj->rot_dirty = 1;
                }
            }
            obj->action(0x6C, 1, 1, 0);
            zero.x = 0;
            zero.y = 0;
            zero.z = 0;
            src    = &zero;
            set_vel(obj, src);
            arg0->field_4d = player_id();
            arg0->field_4c = func_020aae08(func_020aaeb4(), data_021027f8.field_2a);
            if (obj->player == data_021027f8.field_2a) {
                if (data_027e02a8 & 0x40) {
                    buf[0] = data_021027f8.field_2a;
                    func_020b04f0(&data_0210794c, 0x44, &buf[0], 0xFF);
                }
                func_020a99bc(func_020aaeb4(), 10);
                switch (arg0->target->kind) {
                    case 0x166: {
                        Vec3 off;
                        off.x = 0;
                        off.y = 0xA000;
                        off.z = -0xC000;
                        src   = &off;
                        cam_set_offset(src);
                        break;
                    }
                    case 0x5D:
                    case 0x68: {
                        Vec3 off;
                        off.x = 0;
                        off.y = 0xF000;
                        off.z = -0x12000;
                        src   = &off;
                        cam_set_offset(src);
                        break;
                    }
                    default: {
                        Vec3 off;
                        off.x = 0x7000;
                        off.y = 0x14000;
                        off.z = -0x18000;
                        src   = &off;
                        cam_set_offset(src);
                        break;
                    }
                }
            }
            {
                Entity *tgt = arg0->target;
                s32 x       = ang_to_deg(tgt->rot.x.v);
                s32 y       = ang_to_deg(tgt->rot.y.v);
                arg0->pitch = y;
                arg0->yaw   = x;
            }
            {
                Entity *tgt      = arg0->target;
                s32 x            = ang_to_deg(tgt->rot.x.v);
                s32 y            = ang_to_deg(tgt->rot.y.v);
                arg0->goal_pitch = y;
                arg0->goal_yaw   = x;
            }
            arg0->speed = 0;
            if (arg0->target->kind == 0xD6 || arg0->target->kind == 0x166) {
                arg0->field_14 = 1;
                arg0->field_15 = 2;
                obj->action(0x6C, 1, 1, 0);
            } else if (arg0->target->kind == 0x5D || arg0->target->kind == 0x68) {
                arg0->field_14 = 3;
                arg0->field_15 = -1;
                obj->action(0x6D, 1, 1, 0);
            }
            {
                Vec3 zero2;
                zero2.x = 0;
                zero2.y = 0;
                zero2.z = 0;
                src     = &zero2;
                set_vel(obj, src);
            }
            arg0->target->field_1a8 &= ~0xC;
            arg0->target->link = arg0;
            func_020c3c34(&arg0->target->sub, func_0204bd88, 0, 6, 2);
            return;
        case 1:
            break;
        default:
            return;
    }
    if (pressed(obj, 0xF0) && obj->busy == 0) {
        arg0->active = 1;
        if (pressed(obj, 0x40) && !(arg0->target->kind == 0x5D || arg0->target->kind == 0x68)) {
            arg0->goal_yaw += 0x4000;
        } else if (pressed(obj, 0x80) && !(arg0->target->kind == 0x5D || arg0->target->kind == 0x68)) {
            arg0->goal_yaw -= 0x4000;
        }
        if (pressed(obj, 0x20)) {
            arg0->goal_pitch += 0x4000;
        } else if (pressed(obj, 0x10)) {
            arg0->goal_pitch -= 0x4000;
        }
        if (arg0->goal_yaw < -0x19000) {
            arg0->goal_yaw = -0x19000;
        }
        if (arg0->goal_yaw > 0x22000) {
            arg0->goal_yaw = 0x22000;
        }
    } else if (arg0->active) {
        arg0->active     = 0;
        arg0->goal_pitch = arg0->pitch;
        arg0->goal_yaw   = arg0->yaw;
    }
    // dx is built in two steps (dx = goal_pitch; ... dx -= pitch;) while dy is one expression.
    // FAKE: the split dx forces the target's early goal_pitch - pitch subtract and its registers at +0x6a0
    goal_yaw   = arg0->goal_yaw;
    yaw        = arg0->yaw;
    goal_pitch = arg0->goal_pitch;
    pitch      = arg0->pitch;
    dx         = goal_pitch;
    dy         = goal_yaw - yaw;
    dx -= pitch;
    func_02007f24(&accel, 0.5);
    arg0->speed += accel;
    if (arg0->speed > 0x3000) {
        arg0->speed = 0x3000;
    }
    // The squared length is formed twice (helper in the check, direct in normalize), Y from goal_yaw - yaw, not dy.
    // FAKE: mwcc shares the two sums, giving the target's Y*Y first, then X*X + Y*Y operand order (+0x710)
    if (length_precheck(len_sq2(dx, goal_yaw - yaw)) > arg0->speed) {
        normalize_delta(&dx, &dy, fx_mul(dx, dx) + fx_mul(goal_yaw - yaw, goal_yaw - yaw));
        apply_speed(&dx, &dy, arg0->speed);
    } else {
        arg0->speed = 0;
    }
    pitch       = arg0->pitch;
    yaw         = arg0->yaw;
    arg0->pitch = pitch + dx;
    arg0->yaw   = yaw + dy;
    set_rot_y_deg(obj, arg0->pitch);
    set_rot_x_deg(obj, arg0->yaw);
    if (arg0->timer != 0) {
        arg0->timer--;
    }
    if (pressed(obj, 0x800) && arg0->timer == 0 && obj->busy == 0) {
        func_0204b4b8(arg0, obj);
    }
    if (pressed(obj, 2) && obj->busy == 0) {
        func_0204b29c(arg0);
    }
    if (arg0->target->kind != 0x166) {
        return;
    }
    set_rot_x_deg(obj, 0);
}
}
