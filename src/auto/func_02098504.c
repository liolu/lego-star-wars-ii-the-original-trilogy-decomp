#include "m2c.h"

/* Moves the halfword *cur one step toward target.
 *
 * target and step are 1-halfword aggregates passed by value (homed by the
 * `push {r0-r3}` prologue); caller func_02018210 passes the address of a local
 * copy of a halfword field. The signed 16-bit difference cur - target picks
 * the direction; when adding (or subtracting) step would cross target, *cur
 * is clamped to target instead. The new value goes through a by-value
 * temporary, which keeps the two halfword stack slots per path.
 *
 * Returns 1 when *cur was below target, -1 when above, 0 when equal.
 */

typedef struct {
    u16 v;
} HalfVal;

enum {
    STEP_UP   = 1,
    STEP_DOWN = -1,
    AT_TARGET = 0,
};

static inline HalfVal hv_id(HalfVal a) {
    return a;
}

s32 func_02098504(HalfVal *cur, HalfVal target, HalfVal step) {
    u16 diff = cur->v - target.v;

    if ((s16) diff < 0) {
        if ((s16) (u16) (diff + step.v) > 0) {
            cur->v = target.v;
        } else {
            cur->v = hv_id((HalfVal) {cur->v + step.v}).v;
        }
        return STEP_UP;
    }
    if ((s16) diff > 0) {
        if ((s16) (u16) (diff - step.v) < 0) {
            cur->v = target.v;
        } else {
            cur->v = hv_id((HalfVal) {cur->v - step.v}).v;
        }
        return STEP_DOWN;
    }
    return AT_TARGET;
}
