#include "m2c.h"

/* Per-frame motion update for an object driven by a mode byte at 0x30C.
 * The caller (func_020a0440) runs this and then increments the frame
 * counter at 0x200; func_020a95d4 sets the mode, resets the offset vector
 * at 0x310..0x318, clears 0x200 and 0x31C, and loads the countdown at 0x204.
 *   mode 1: 0x31C = 700 * sin(frames * 170 angle units), whole units.
 *   mode 2: x (0x310) += sin(frames * 100 deg) / 2 and
 *           z (0x318) += cos(frames * 80 deg) / 2; 0x31C = 0.
 *   mode 3: 0x31C = 0.
 * Then, if the countdown is running, it is decremented and on reaching 0
 * the object switches to mode 3 with no countdown.
 */

typedef struct {
    u16 v;
} Angle;

typedef struct {
    u16 v;
} Value16;

enum {
    AMPLITUDE_700_FX = 0x2BC000, /* 700.0 in fx32 */
    HALF_DIVISOR_FX  = 0x2000,    /* 2.0 in fx32 */
};

extern s16 data_020f88c4[]; /* interleaved sin/cos table, index = angle >> 4 */
extern s32 FX_Div(s32, s32);
extern void func_020a95d4(void *arg0, s8 arg1, s32 arg2);

static inline s32 fx_mul(s32 a, s32 b) {
    return (s32) (((s64) a * b + 0x800) >> 12);
}

static inline s32 sin_of(Angle a) {
    return data_020f88c4[(a.v >> 4) * 2];
}

static inline s32 cos_of(Angle a) {
    return data_020f88c4[(a.v >> 4) * 2 + 1];
}

static inline void set_value_31c(void *self, Value16 h) {
    M2C_FIELD(self, u16 *, 0x31C) = h.v;
}

void func_020a9158(void *self) {
    s32 frames_left;

    switch (M2C_FIELD(self, u8 *, 0x30C)) {
        case 3: {
            u16 zero = 0;
            set_value_31c(self, (Value16) {zero});
            break;
        }
        case 1:
            set_value_31c(self,
                          (Value16) {fx_mul(sin_of((Angle) {M2C_FIELD(self, s32 *, 0x200) * 0xAA}), AMPLITUDE_700_FX) >> 12});
            break;
        case 2: {
            u16 zero = 0;
            M2C_FIELD(self, s32 *, 0x310) +=
                FX_Div(sin_of((Angle) {((M2C_FIELD(self, s32 *, 0x200) * 100) << 16) / 360}), HALF_DIVISOR_FX);
            M2C_FIELD(self, s32 *, 0x318) +=
                FX_Div(cos_of((Angle) {((M2C_FIELD(self, s32 *, 0x200) * 80) << 16) / 360}), HALF_DIVISOR_FX);
            set_value_31c(self, (Value16) {zero});
            break;
        }
    }
    frames_left = M2C_FIELD(self, s32 *, 0x204);
    if (frames_left != 0) {
        frames_left--;
        M2C_FIELD(self, s32 *, 0x204) = frames_left;
        if (frames_left == 0) {
            func_020a95d4(self, 3, 0);
        }
    }
}
