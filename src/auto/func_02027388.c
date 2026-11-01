#include "m2c.h"

typedef struct {
    u16 v;
} Ang;

typedef struct {
    Ang v;
    u8 f2;
    u8 f3;
    u8 f4;
    u8 f5;
} Key;

enum {
    ANGLE_LIMIT = 0x71c, /* 10 degrees in 0x10000-per-turn units */
};

extern void func_0202732c(Key *);

/* Blends `count` keys into `out` (cleared first by func_0202732c), weighting key i by
 * weights[i]:
 *   - out->v  = weighted mean of each key's signed angle, clamped to +/-ANGLE_LIMIT, with
 *               per-key weight weights[i] * f4;
 *   - out->f2 = 1 when the f2 values weighted by weights[i] * f5 reach at least half of the
 *               total weights[i] * f5, else 0;
 *   - out->f3 = 1 when any key has f3 == 1.
 * The quotient passes through a stack aggregate of halfword wrappers, which keeps the
 * intermediate halfword copy as a real store. */
void func_02027388(Key *out, Key *keys, u8 *weights, u32 count) {
    s32 angle_sum;
    s32 angle_weight_sum;
    s32 f2_sum;
    s32 f2_weight_sum;
    u32 i;
    s32 angle;
    struct {
        Ang copy;
        Ang quotient;
    } l;

    func_0202732c(out);
    angle_sum        = 0;
    f2_sum           = 0;
    angle_weight_sum = 0;
    f2_weight_sum    = 0;
    for (i = 0; i < count; i++) {
        angle = (s16) keys[i].v.v;
        if (angle < -ANGLE_LIMIT) {
            angle = -ANGLE_LIMIT;
        } else if (angle > ANGLE_LIMIT) {
            angle = ANGLE_LIMIT;
        }
        angle_sum += weights[i] * (angle * keys[i].f4);
        angle_weight_sum += weights[i] * keys[i].f4;
        f2_sum += weights[i] * (keys[i].f2 * keys[i].f5);
        f2_weight_sum += weights[i] * keys[i].f5;
        if (keys[i].f3 == 1) {
            out->f3 = 1;
        }
    }
    l.quotient.v = angle_sum / angle_weight_sum;
    l.copy       = l.quotient;
    out->v       = l.copy;
    if (f2_sum < f2_weight_sum / 2) {
        out->f2 = 0;
    } else {
        out->f2 = 1;
    }
}
