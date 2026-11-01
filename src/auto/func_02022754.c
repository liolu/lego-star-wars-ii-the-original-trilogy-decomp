/* Constructor for the entity kinds that the object factory func_02015138
 * routes here (kinds 84, 85, 110, 140, 168, 304, 305, 306 and 349, each in a
 * 0x24c-byte allocation). It runs the common entity constructor
 * func_02011384, installs this class's vtable (data_020fd810), clears the
 * class's own state and saves the current yaw.
 *
 * Kind 0x15d also spawns five child objects (factory kinds 0x158..0x15c) at
 * this object's position, rotated by its yaw, and records for each child the
 * child's sub-object angle plus this object's yaw (func_02022fa0 later reads
 * these angles back). Every other kind sets flag bits 0x120 and 0x1000000
 * and byte f42 = 2; all but kind 0x54 also replace f84 and the callback.
 * Returns the constructed object.
 *
 * The angle helpers return their result through a caller-provided temporary;
 * that copy is what places the halfword stack temporaries of the original.
 */
#include "m2c.h"

enum {
    KIND_WITH_CHILDREN  = 0x15d, /* spawns NUM_CHILDREN children */
    KIND_KEEPS_CALLBACK = 0x54,  /* keeps the base constructor's f84/callback */
    KIND_6E             = 0x6e,  /* gets flag bit 0x4000000 */
    FIRST_CHILD_KIND    = 0x158, /* child i is factory kind 0x158 + i */
    NUM_CHILDREN        = 5,
    F84_VALUE           = 0x13,
    XFORM_WORD_VALUE    = 0x1000, /* stored to transform record +0x30..+0x38 */
};

typedef struct {
    s32 x, y, z;
} Vec;

typedef struct {
    u16 v;
} Angle;

typedef struct {
    u8 pre[0x24];
    Vec pos;
    s32 s[3];
    u16 w[4];
    u8 f[2];
} Tpl;

typedef struct {
    u8 pad00[0x34];
    Angle yaw;
} Sub20;

typedef struct Entity {
    void *vtbl;
    u8 p4[4];
    u32 flags;
    u8 pc[4];
    void *callback;
    u8 p14[0xc];
    Sub20 *sub;
    u8 p24[0x1e];
    u8 b42;
    u8 p43[0x41];
    s32 f84;
    u8 p88[0x28];
    Vec pos;
    u8 pbc[0xe];
    Angle yaw;
    u8 pcc[0x5];
    u8 pos_changed; /* set to 1 when pos is rewritten */
    u8 pd2[0x144];
    u8 b216;
    u8 b217;
    s32 f218;
    s32 f21c;
    s32 f220;
    s32 f224;
    struct Entity *children[5];
    Angle child_angle[5];
    Angle saved_yaw;
} Entity;

extern Entity *func_02011384(Entity *self, u32 type, u32 attr, s32 arg3);
extern void func_020accac(void *, void *, void *, void *);
extern Entity *func_02015138(s32, s32, void *);
extern M2C_UNK data_020fd810;
extern void func_02022bbc(void);
extern void func_020230b0(void);

static inline void set_angle(Angle *d, Angle s) {
    d->v = s.v;
}

static inline Angle angle_copy(const Angle *a, Angle *r) {
    r->v = a->v;
    return *r;
}

static inline Angle angle_add(const Angle *a, Angle b, Angle *r) {
    r->v = a->v + b.v;
    return *r;
}

Entity *func_02022754(Entity *self, u32 type, u32 flags, void *transform) {
    Vec *own_pos;
    Entity *child;
    s32 i;
    u32 pos_differs;
    Tpl child_xform;
    Sub20 *child_sub[5];
    Sub20 *sub_ref[5];
    Angle sum;
    Angle yaw_tmp;

    func_02011384(self, type, flags, (s32) transform);
    M2C_FIELD(self, void **, 0) = &data_020fd810;
    self->f218                  = 0;
    self->f21c                  = 0;
    self->b216                  = 0;
    self->b217                  = 0;
    self->f224                  = 0;
    self->f220                  = 0;
    self->saved_yaw             = angle_copy(&self->yaw, &yaw_tmp);
    if (type == KIND_WITH_CHILDREN) {
        self->f84      = F84_VALUE;
        self->callback = func_02022bbc;
        self->flags |= 0x200;
        own_pos = &self->pos;
        for (i = 0; i < NUM_CHILDREN; i++) {
            s32 xform_words[3];
            u16 xform_angles[4];
            /* Build the child's transform record: own position, angles (0, yaw, 0). */
            xform_words[0]  = XFORM_WORD_VALUE;
            xform_words[1]  = XFORM_WORD_VALUE;
            xform_words[2]  = XFORM_WORD_VALUE;
            xform_angles[0] = 0;
            xform_angles[1] = self->yaw.v;
            xform_angles[2] = 0;
            func_020accac(&child_xform, own_pos, xform_angles, xform_words);
            child             = func_02015138(FIRST_CHILD_KIND + i, 0, &child_xform);
            self->children[i] = child;
            /* Snap the child to this object's position, flagging a change. */
            if ((child->pos.x != own_pos->x) || (child->pos.y != own_pos->y) || (child->pos.z != own_pos->z)) {
                pos_differs = 1;
            } else {
                pos_differs = 0;
            }
            if (pos_differs != 0) {
                if (&child->pos != own_pos) {
                    child->pos.x = own_pos->x;
                    child->pos.y = own_pos->y;
                    child->pos.z = own_pos->z;
                }
                child->pos_changed = 1;
            }
            child_sub[i] = self->children[i]->sub;
            sub_ref[i]   = child_sub[i];
            set_angle(&self->child_angle[i], angle_add(&sub_ref[i]->yaw, self->yaw, &sum));
        }
        return self;
    }
    if (type != KIND_KEEPS_CALLBACK) {
        self->f84      = F84_VALUE;
        self->callback = func_020230b0;
    }
    self->flags |= 0x120;
    self->flags |= 0x1000000;
    self->b42 = 2;
    if (type == KIND_WITH_CHILDREN) { /* unreachable here; kept from the original */
        self->flags &= ~0x120;
    } else if (type == KIND_6E) {
        self->flags |= 0x4000000;
    }
    return self;
}
