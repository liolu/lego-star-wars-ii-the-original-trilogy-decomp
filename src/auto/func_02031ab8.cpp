#include "m2c.h"

typedef struct {
    s32 v;
} Fx;

struct Ang {
    u16 v;
    Ang(u16 x) :
        v(x) {}
    Ang operator+(Ang off) const {
        return Ang(v + off.v);
    }
};

class Yaw {
public:
    u16 v;
    Yaw(const Yaw &other);
    s32 absdiff(Ang b) const {
        s32 d = (s16) (u16) (v - b.v);
        if (d < 0) {
            d = -d;
        }
        return d;
    }
};

typedef struct {
    u8 pad0[0xb0];
    s32 pos[3];     /* +0xb0 */
    u8 padbc[0xca - 0xbc];
    Ang heading;    /* +0xca */
} Ent;

typedef struct {
    u8 pad0[0xc];
    u16 kind;       /* +0x0c */
    u8 pade[0xb0 - 0xe];
    s32 pos[3];     /* +0xb0 */
} Obj;

typedef struct Node {
    Obj *obj;
    struct Node *next;
} Node;

typedef struct {
    u8 pad0[4];
    Ent *ent;       /* +0x04 */
    u8 pad8[0x1f - 0x8];
    u8 cooldown;    /* +0x1f */
    u8 pad20[0x30 - 0x20];
    Ang aim_offset; /* +0x30 */
} Self;

typedef struct {
    u8 pad0[0x29];
    u8 count;
} Info;

typedef struct {
    u8 pad0[0x4c];
    Obj *objs[1];
} Mgr;

typedef struct {
    u8 pad0[4];
    Node *head;
    u8 pad8[0x38 - 8];
} Slot;

extern "C" Info data_021027f8;
extern "C" Mgr data_0210930c;
extern "C" s32 data_027e02a4;
extern "C" Slot data_021019f4[8];

/* Angles are u16 turns (0x10000 = 360 degrees); radii are fx20.12. */
enum {
    NEAR_RADIUS    = 0x64000,   /* 100.0 */
    FAR_RADIUS     = 0x15e000,   /* 350.0 */
    QUARTER_TURN   = 0x4000,   /* 90 degrees */
    SIXTH_TURN     = 0x2aaa,     /* 60 degrees */
    COOLDOWN_RESET = 10
};

static inline Slot *cur_slot(void) {
    return &data_021019f4[data_027e02a4];
}

/* Nonzero when |a - b| < r (Vec3 positions); *out = squared distance >> 12. */
extern "C" s32 func_0200f5d0(void *a, void *b, Fx r, s32 *out);
extern "C" Yaw func_0201811c(void *from, void *to, s32 arg);
extern "C" void func_0202fbbc(void *obj, void *target, s32 a2);
extern "C" void func_02031d90(Self *self, Obj *target);
extern "C" u32 func_02062ae4(s32 arg0);

static inline s32 aim_delta(Ent *ent, Obj *o, const Ang &want) {
    return func_0201811c(ent, o, 0).absdiff(want);
}

static inline Fx fx_make(s32 v) {
    Fx r;
    r.v = v;
    return r;
}

static inline s32 aim_off(Self *self, Obj *o) {
    const Ang &want = self->ent->heading + self->aim_offset;
    return func_0201811c(self->ent, o, 0).absdiff(want);
}

static inline s32 aim_err(Self *self, Obj *target) {
    return aim_off(self, target);
}

static inline int better(Self *self, Obj *o, s32 dist, s32 best) {
    return dist < best && aim_delta(self->ent, o, self->ent->heading) < QUARTER_TURN;
}

/*
 * Picks a target for self->ent. First pass: the nearest object in data_0210930c.objs
 * (data_021027f8.count entries) within NEAR_RADIUS whose yaw from the entity is within a
 * quarter turn of its heading. If none, second pass: the nearest kind 0x400 object in the
 * current slot's list within FAR_RADIUS, same heading test. `dist`/`best` hold squared
 * distances. The result (or NULL) goes to func_02031d90. While cooldown is nonzero it
 * counts down; at zero, with a target, func_02062ae4(found ? 10 : 60) == 0 and the yaw to
 * the target within SIXTH_TURN of heading + aim_offset, it calls func_0202fbbc(self,
 * target, -1) and resets cooldown.
 */
extern "C" void func_02031ab8(Self *self) {
    Obj *target = (Obj *) 0;
    Obj *found  = (Obj *) 0;
    u32 i;
    Obj *o;
    s32 best = 0x7fffffff;

    for (i = 0; i < data_021027f8.count; i++) {
        o = data_0210930c.objs[i];
        if (o != (Obj *) 0) {
            s32 dist = 0;
            if (func_0200f5d0(self->ent->pos, o->pos, fx_make(NEAR_RADIUS), &dist)) {
                if (better(self, o, dist, best)) {
                    best  = dist;
                    found = o;
                }
            }
        }
    }
    if (found != (Obj *) 0) {
        target = found;
    } else {
        Node *n;
        best = 0x7fffffff;
        for (n = cur_slot()->head; n != (Node *) 0; n = n->next) {
            Obj *o = n->obj;
            if (o->kind == 0x400) {
                s32 dist = 0;
                if (func_0200f5d0(self->ent->pos, o->pos, fx_make(FAR_RADIUS), &dist)) {
                    if (better(self, o, dist, best)) {
                        best   = dist;
                        target = o;
                    }
                }
            }
        }
    }
    func_02031d90(self, target);
    if (self->cooldown == 0) {
        if (target == (Obj *) 0) {
            return;
        }
        if (func_02062ae4((u16) (found != (Obj *) 0 ? 10 : 60))) {
            return;
        }
        if (aim_err(self, target) >= SIXTH_TURN) {
            return;
        }
        func_0202fbbc(self, target, -1);
        self->cooldown = COOLDOWN_RESET;
    } else {
        self->cooldown--;
    }
}
