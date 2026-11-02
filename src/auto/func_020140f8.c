#include "m2c.h"

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct {
    u16 v;
} Half;

typedef struct {
    Half x;
    Half y;
    Half z;
} Ang3;

typedef struct {
    u16 x;
    u16 y;
    u16 z;
} Rot3;

typedef struct {
    u8 pad0[0x24];
    Vec3 pos;
    u8 pad30[0x3c - 0x30];
    Ang3 ang;
    u8 pad42[2];
    u8 angDirty;
    u8 posDirty;
    u8 pad46[0x384 - 0x46];
    u32 flags384;
} Child;

typedef struct {
    u8 pad0[4];
    Vec3 pos;
} Parent;

typedef struct {
    u8 pad0[8];
    u32 flags;
    u8 pad0c[0x30 - 0x0c];
    Parent *parent;
    u8 pad34[0xb0 - 0x34];
    Vec3 pos;
    u8 padbc[0xc8 - 0xbc];
    Ang3 ang;
    u8 padce[0x1c4 - 0xce];
    Child *child;
    u8 pad1c8[0x212 - 0x1c8];
    u16 id;
} Obj;

typedef struct {
    u16 type;
    u16 dir;
    u8 pad[12];
} Entry;

typedef struct {
    u8 m[0x48];
} Xform;

extern const Entry data_020f03b8[];
extern const u16 data_020f0394[];
extern u8 data_0210930c[];

void *func_0205e80c(s32, s32, s32);
void *func_020bbfe8(void *);
s32 func_0205ab44(s32);
s32 func_0205ab58(s32);
void func_020bbeb0(void *, s32);
void func_020bbf78(void *, s32, s32, s32);
void func_020bc4cc(void *, s32);
void func_020b95bc(s32, void *);
u8 func_020917b4(void *);
u8 func_02093fa8(s32);
void func_020accac(void *, void *, void *, void *);
void *func_0200af90(void *, s32, s32, s32, s32, void *);

enum {
    CHILD_OBJECT_SIZE = 0x3a0,
    HIGH_BIT          = 0x80000000,
};

// Nonzero when the two positions differ in any component.
static inline int vec_ne(const Vec3 *a, const Vec3 *b) {
    if (!(a->x == b->x && a->y == b->y && a->z == b->z)) {
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

// Allocates one 0x3a0-byte child object and runs its initialiser when the
// allocation succeeded (same shape as make_obj in func_0200cef4).
static inline Child *make_child(void) {
    Child *o = (Child *) func_0205e80c(CHILD_OBJECT_SIZE, 3, 4);
    if (o != NULL) {
        o = (Child *) func_020bbfe8(o);
    }
    return o;
}

// Binds a child to a pair of resource ids.
static inline void bind(Child *o, s32 model, s32 anim) {
    func_020bbeb0(o, func_0205ab44(model));
    func_020bbf78(o, anim, func_0205ab58(anim), 0);
}

// Copies pos into the child (+0x24) and raises its position flag (+0x45)
// only when the position changed (func_020accac sets the same fields).
static inline void sync_pos(Child *o, const Vec3 *pos) {
    if (vec_ne(&o->pos, pos)) {
        vec_set(&o->pos, pos);
        o->posDirty = 1;
    }
}

// Copies the owner's three angle halves (+0xc8) into the child (+0x3c) and
// raises the child's angle flag (+0x44) only when any of them changed.
static inline void sync_ang(Child *o, Obj *a) {
    Half cur_x;
    Half cur_y;
    Half cur_z;
    Half new_x;
    Half new_y;
    Half new_z;
    int e;
    int f;
    f     = 0;
    e     = 0;
    cur_x = o->ang.x;
    if (a->ang.x.v == cur_x.v) {
        cur_y = o->ang.y;
        if (a->ang.y.v == cur_y.v) {
            e = 1;
        }
    }
    if (e) {
        cur_z = o->ang.z;
        if (a->ang.z.v == cur_z.v) {
            f = 1;
        }
    }
    if (!f) {
        new_x       = a->ang.x;
        o->ang.x.v  = new_x.v;
        new_y       = a->ang.y;
        o->ang.y.v  = new_y.v;
        new_z       = a->ang.z;
        o->ang.z.v  = new_z.v;
        o->angDirty = 1;
    }
}

static inline Vec3 *obj_pos(Obj *o) {
    return &o->pos;
}

static inline s32 has_parent(Parent *p) {
    s32 r;

    r = p != NULL;
    return r;
}

// Syncs a child's position and angle, runs one func_020bc4cc pass and sets
// the top bit of +0x384.
static inline void place(Child *o, Obj *self, const Vec3 *pos) {
    sync_pos(o, pos);
    sync_ang(o, self);
    func_020bc4cc(o, 1);
    o->flags384 |= HIGH_BIT;
}

// Spawns the child objects for self according to the type stored in
// data_020f03b8[self->id]: types 6/7, 8 and 9/10 keep one child in
// self->child, type 11 places two, other types place three (resource ids
// chosen for types 0..5), type 12 spawns none. Unless the entry's second halfword
// is 8, it then allocates a 0x1c0-byte object at self's position and passes
// it a transform built from (pos, zero angles, unit scale).
void func_020140f8(Obj *self) {
    Xform xf;
    Vec3 pos;
    Vec3 scale;
    Rot3 rot;
    Child *a;
    Child *b;
    Child *c;
    Child *p;
    u16 type;
    u32 dir;
    u8 mode;
    Parent *par;
    s32 visible;

    pos.x = self->pos.x;
    pos.y = self->pos.y;
    pos.z = self->pos.z;
    type  = data_020f03b8[self->id].type;
    if (type == 6 || type == 7) {
        par = self->parent;
        if (has_parent(par)) {
            vec_set(&pos, &par->pos);
        } else {
            vec_set(&pos, obj_pos(self));
        }
        self->child = make_child();
        if (data_020f03b8[self->id].type == 6) {
            func_020bbeb0(self->child, func_0205ab44(0x1160));
        } else {
            func_020bbeb0(self->child, func_0205ab44(0xd33));
        }
        func_020bbf78(self->child, 0x115f, func_0205ab58(0x115f), 0);
        sync_pos(self->child, &pos);
        sync_ang(self->child, self);
        func_020bc4cc(self->child, 1);
        return;
    }
    if (type == 8) {
        if (self->child != NULL) {
            return;
        }
        self->child = make_child();
        func_020bbeb0(self->child, func_0205ab44(0x557));
        func_020bbf78(self->child, 0x556, func_0205ab58(0x556), 0);
        sync_pos(self->child, obj_pos(self));
        sync_ang(self->child, self);
        func_020bc4cc(self->child, 1);
        return;
    }
    if (type == 9 || type == 10) {
        if (self->child != NULL) {
            return;
        }
        self->child = make_child();
        func_020bbeb0(self->child, func_0205ab44(0x8b8));
        func_020bbf78(self->child, 0x8a9, func_0205ab58(0x8a9), 0);
        sync_pos(self->child, obj_pos(self));
        sync_ang(self->child, self);
        func_020bc4cc(self->child, 1);
        if (data_020f03b8[self->id].type == 9) {
            self->child->flags384 |= HIGH_BIT;
        }
        return;
    }
    if (type != 12) {
        a = make_child();
        b = make_child();
        if (data_020f03b8[self->id].type == 11) {
            bind(a, 0x768, 0x767);
            bind(b, 0x76c, 0x76b);
            sync_pos(a, &pos);
            func_020bc4cc(a, 1);
            a->flags384 |= HIGH_BIT;
            sync_pos(b, &pos);
            func_020bc4cc(b, 1);
            b->flags384 |= HIGH_BIT;
            return;
        }
        c = make_child();
        if (self->id == 1) {
            pos.y += 0x16000;
        } else if (self->id == 0xe0 || self->id == 0xe1) {
            par = self->parent;
            if (has_parent(par)) {
                vec_set(&pos, &par->pos);
            } else {
                pos.y += 0x5000;
            }
        } else {
            pos.y += 0x5000;
        }
        switch (data_020f03b8[self->id].type) {
            case 0:
                bind(a, 0x8b2, 0x8ae);
                bind(b, 0x8b9, 0x8b0);
                bind(c, 0x8b6, 0x8b3);
                break;
            case 1:
                bind(a, 0x8a4, 0x8ae);
                bind(b, 0x8b8, 0x8b0);
                bind(c, 0x8b5, 0x8b3);
                break;
            case 2:
                bind(a, 0x8a0, 0x8ae);
                bind(b, 0x8b7, 0x8b0);
                bind(c, 0x8b4, 0x8b3);
                break;
            case 3:
                bind(a, 0x8b2, 0x8a8);
                bind(b, 0x8b9, 0x8a9);
                bind(c, 0x8b6, 0x8b3);
                break;
            case 4:
                bind(a, 0x8a4, 0x8a8);
                bind(b, 0x8b8, 0x8a9);
                bind(c, 0x8b5, 0x8b3);
                break;
            case 5:
                bind(a, 0x8a0, 0x8a8);
                bind(b, 0x8b7, 0x8a9);
                bind(c, 0x8b4, 0x8b3);
                break;
        }
        place(a, self, &pos);
        place(b, self, &pos);
        place(c, self, &pos);
    }
    type = data_020f03b8[self->id].type;
    if (type != 12 && data_020f0394[type] != 0xc9) {
        func_020b95bc(data_020f0394[type], self);
    }
    dir = data_020f03b8[self->id].dir;
    if (dir == 8) {
        return;
    }
    mode = dir;
    vec_set(&pos, obj_pos(self));
    switch (func_02093fa8(func_020917b4(data_0210930c))) {
        case 0:
        case 1:
            if (mode == 2) {
                mode = 1;
            }
            break;
        case 2:
            if (mode == 1) {
                mode = 2;
            }
            break;
    }
    if (self->id == 1) {
        pos.y += 0x11000;
    }
    p = (Child *) func_0205e80c(0x1c0, 0, 4);
    if (p == NULL) {
        return;
    }
    rot.x   = 0;
    rot.y   = 0;
    rot.z   = 0;
    scale.x = 0x1000;
    scale.y = 0x1000;
    scale.z = 0x1000;
    func_020accac(&xf, &pos, &rot, &scale);
    visible = 1;
    if (self->flags & HIGH_BIT) {
        visible = 0;
    }
    func_0200af90(p, 8, mode, 1, visible, &xf);
}
