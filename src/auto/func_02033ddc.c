#include "m2c.h"

/* Reconstructed from USA ARM9 at 0x02033ddc (632 bytes); reached through the
 * pointer table data_020fdeec (the vtable func_02033d68 installs).
 *
 * Per-frame update of a controller that steers its entity (+0x04) on a
 * state byte (+0x0d):
 *  - state 1: if the entity's animation (+0x40) reports 3, set flag bit 0 and
 *    call func_02004b78(ent, 0x12). Otherwise, if func_0200aef8(entity
 *    position, goal +0x10, radius +0x2c) is nonzero, switch to state 4. Else,
 *    if func_0202cb44 yields another entity and func_0200aef8(entity
 *    position, point +0x1c, 0xa000) is zero, update the entity yaw towards
 *    it (func_0201811c, then func_02018210 with step 0x38e) and take its
 *    position as the new goal with radius 0xf000. Unless the entity's link (+0x2c)
 *    has its byte +0x26 set, push the entity along its heading (+0xca, read
 *    through the sine/cosine table) scaled by the speed at +0x28. Finally the
 *    timer at +0x30 counts down and switches to state 4 when it expires.
 *  - state 4: copy data_020ff9b4 into the entity vector at +0x60, set bit
 *    0x200 of its flags, set flag bit 0 and call func_02004b68.
 */

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct {
    u16 v;
} HalfVal;

typedef struct {
    s32 v;
} WordArg;

typedef struct {
    u8 pad0[0x26];
    u8 b26;
} Link;

typedef struct {
    u8 pad0[8];
    u32 flags;      /* +0x08 */
    u8 pad_c[0x2c - 0xc];
    Link *link;     /* +0x2c */
    u8 pad_30[0x40 - 0x30];
    u8 anim[0x20];  /* +0x40 */
    Vec3 vel;       /* +0x60 */
    u8 pad_6c[0xb0 - 0x6c];
    Vec3 pos;       /* +0xb0 */
    u8 pad_bc[0xca - 0xbc];
    u16 ang;        /* +0xca */
    u8 pad_cc[0x5e0 - 0xcc];
    u8 sub5e0[4];   /* +0x5e0 */
} Entity;

typedef struct {
    u8 pad0[4];
    Entity *ent;    /* +0x04 */
    u32 pad8;
    u8 flag_c;      /* +0x0c */
    u8 state;       /* +0x0d */
    u8 pad_e[2];
    Vec3 goal;      /* +0x10 */
    Vec3 point1c;   /* +0x1c */
    s32 speed;      /* +0x28 */
    WordArg radius; /* +0x2c */
    u16 timer;      /* +0x30 */
} Obj;

extern const s16 data_020f88c4[];
extern Vec3 data_020ff9b4;

extern u16 func_0204fa30(void *);
extern void func_02004b78(Entity *, s32);
extern void func_02004b68(Entity *, s32, Entity *, s32);
extern s32 func_0200aef8(void *, void *, WordArg);
extern Entity *func_0202cb44(void *);
extern void func_0201811c(u16 *, Entity *, Entity *, s32);
extern s32 func_02018210(Entity *, HalfVal, HalfVal);
extern void func_02017ee4(Entity *, Vec3 *, s32);

enum {
    STATE_MOVE       = 1,
    STATE_DONE       = 4,
    ANIM_RESULT_STOP = 3,
    FLAG_C_BIT0      = 1,
    ENT_FLAG_200     = 0x200,
    NEAR_RADIUS      = 0xa000,
    GOAL_RADIUS      = 0xf000,
    TURN_STEP        = 0x38e,
    MOVE_BLEND       = 0x4b,
};

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

static inline s32 FX_MUL(s32 a, s32 b) {
    return (s32) (((s64) a * b + 0x800) >> 12);
}

void func_02033ddc(Obj *o) {
    switch (o->state) {
        case STATE_MOVE: {
            Entity *other;
            s32 hit;
            Entity *ent;
            s32 blocked;

            if (func_0204fa30(o->ent->anim) == ANIM_RESULT_STOP) {
                o->flag_c |= FLAG_C_BIT0;
                func_02004b78(o->ent, 0x12);
                return;
            }
            if (func_0200aef8(&o->ent->pos, &o->goal, o->radius) == 0) {
                other = func_0202cb44(o->ent->sub5e0);
                hit   = 0;
                if (other != NULL) {
                    WordArg r;
                    r.v = NEAR_RADIUS;
                    if (func_0200aef8(&o->ent->pos, &o->point1c, r) == 0) {
                        hit = 1;
                    }
                }
                if (hit) {
                    HalfVal yaw;
                    HalfVal turn;
                    func_0201811c(&yaw.v, o->ent, other, 0);
                    turn.v = TURN_STEP;
                    func_02018210(o->ent, yaw, turn);
                    {
                        Vec3 *s = &other->pos;
                        vec_set(&o->goal, s);
                    }
                    o->radius.v = GOAL_RADIUS;
                }
                ent     = o->ent;
                blocked = 0;
                if (ent->link != NULL && ent->link->b26 != 0) {
                    blocked = 1;
                }
                if (!blocked) {
                    Vec3 v;
                    v.x = FX_MUL(o->speed, data_020f88c4[(ent->ang >> 4) * 2]);
                    v.y = 0;
                    v.z = FX_MUL(o->speed, data_020f88c4[(ent->ang >> 4) * 2 + 1]);
                    func_02017ee4(o->ent, &v, MOVE_BLEND);
                }
                if (o->timer == 0) {
                    return;
                }
                o->timer--;
                if (o->timer == 0) {
                    o->state = STATE_DONE;
                }
            } else {
                o->state = STATE_DONE;
            }
            break;
        }
        case STATE_DONE:
            vec_set(&o->ent->vel, &data_020ff9b4);
            o->ent->flags |= ENT_FLAG_200;
            o->flag_c |= FLAG_C_BIT0;
            func_02004b68(o->ent, 1, o->ent, 2);
            break;
    }
}
