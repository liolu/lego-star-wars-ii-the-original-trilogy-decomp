#include "m2c.h"

typedef struct {
    u16 v;
} HalfVal;

typedef struct {
    u8 pad0[0xca];
    u16 angle; /* +0xca */
} Entity;

typedef struct {
    u8 pad0[4];
    Entity *ent; /* +0x04 */
} Obj;

/* 16 bit angle units: 0x10000 is a full turn, so 0x2aaa is about 60 degrees. */
enum {
    TURN_LIMIT = 0x2aaa, /* largest signed angle difference passed on */
    TURN_RATE  = 0x222,   /* second by-value argument (name guessed from callers) */
};

extern void func_0201811c(u16 *, Entity *, void *, s32);
extern void func_02098504(void *, HalfVal, HalfVal);

/* Computes the angle from the entity to the target, clamps the signed
 * difference to +/-TURN_LIMIT and hands it with TURN_RATE to func_02098504
 * on the sub-object at +0x30. Without a target the turn is zero. */
void func_02031d90(Obj *o, void *target) {
    HalfVal turn;
    // FAKE: volatile scratch locals reproduce the original's memory-resident halfword temporaries
    volatile u16 delta;
    u16 goal;
    volatile u16 lo_val;
    volatile u16 hi_val;
    HalfVal limit;
    volatile u16 base;
    volatile u16 copy_delta;
    volatile u16 copy_lo;
    volatile u16 copy_hi;
    u16 tmp;
    u16 *pg;

    turn.v = 0;
    if (target != NULL) {
        base = o->ent->angle;
        func_0201811c(&goal, o->ent, target, 0);
        pg = &goal;
        if (pg) { // FAKE: always-true test keeps the read through a pointer to goal
            delta = *pg - base;
        } else {
            delta = 0;
        }
        tmp        = delta;
        copy_delta = tmp;
        turn.v     = tmp;
        if ((s16) copy_delta < -TURN_LIMIT) {
            lo_val  = -TURN_LIMIT;
            tmp     = lo_val;
            copy_lo = tmp;
            turn.v  = tmp;
        } else if ((s16) turn.v > TURN_LIMIT) {
            hi_val  = TURN_LIMIT;
            tmp     = hi_val;
            copy_hi = tmp;
            turn.v  = tmp;
        }
    }
    limit.v = TURN_RATE;
    func_02098504((u8 *) o + 0x30, turn, limit);
}
