#include "m2c.h"

typedef struct {
    s32 v;
} UnkVal;

typedef struct AnmObj {
    s32 frame;
    s32 ratio;
} AnmObj;

typedef struct Anim {
    AnmObj *anm;
    u16 pad4;
    u16 active : 1;
    u16 wrapped : 1;
    u16 mode : 1;
    u16 reverse : 1;
} Anim;

typedef struct Event {
    u16 frame;
    u16 type;
    u16 val;
} Event;

typedef struct Self {
    u8 pad0[0x20];
    u8 renderObj[0x54];
    Anim anims[5];
    u8 pad9C[4];
    Event *events;
    s32 speed;
    s32 blend;
    u8 padAC[4];
    u8 b0;
    u8 eventIdx;
    u16 fade : 1;
    u16 blending : 1;
} Self;

extern s32 func_0209f30c(Anim *o, UnkVal delta, ...);
extern s32 func_0209fa28(void *self, s32 type, u32 val, void *other);
extern void func_0209f2cc(void *arg0);
extern void NNS_G3dRenderObjRemoveAnmObj(void *ro, AnmObj *anm);

enum {
    NUM_ANIMS  = 5,          /* animation slots at self->anims */
    FX_ONE     = 0x1000,        /* 1.0 in 20.12 fixed point */
    FX_HALF    = 0x800,        /* rounding term for the 20.12 multiply */
    FX_SHIFT   = 12,
    BLEND_STEP = 0x400,     /* blend weight added per call (1/4) */
    B0_RESET   = 0xFF,        /* value stored to b0 when anim 0 wrapped in mode 1 */
};

static inline s32 fire_event(Self *self, const Event *e, void *other) {
    return func_0209fa28(self, e->type, e->val, other);
}

static inline UnkVal neg_val(UnkVal v) {
    UnkVal r;
    r.v = -v.v;
    return r;
}

/*
 * Per-tick animation update.
 * 1. step = dt * self->speed in 20.12 fixed point (rounded). Each active
 *    animation slot records its integer frame before advancing, then is
 *    advanced by func_0209f30c with +step, or -step if its reverse bit is set.
 * 2. If slot 0 is not active, stop. With an event table, a wrap of slot 0
 *    (wrapped and mode bits) sets b0 = 0xFF and rewinds eventIdx to 0. If
 *    slot 0's integer frame changed, every event whose frame is <= the
 *    current frame is passed to func_0209fa28 (type, val, other); a return of
 *    1 re-reads the frame, otherwise eventIdx moves to the next event.
 * 3. While blending is set and slot 1 holds an animation, the blend weight
 *    rises by 1/4 per call: slot 1 ratio = 1 - blend, slot 0 ratio = blend.
 *    On reaching 1.0, slot 0 gets the full ratio, slot 1's animation is
 *    removed from the render object and the slot released by func_0209f2cc,
 *    and blend is reset to 0.
 * Field names are guessed from usage.
 */
void func_01ffad38(Self *self, UnkVal dt, void *other, ...) {
    UnkVal d;
    UnkVal step;
    s32 frames[NUM_ANIMS];
    s32 i;
    u32 cur;

    step.v = (s32) (((s64) dt.v * self->speed + FX_HALF) >> FX_SHIFT);
    for (i = 0; i < NUM_ANIMS; i++) {
        Anim *a = &self->anims[i];
        if (a->active == 1) {
            frames[i] = self->anims[i].anm->frame >> FX_SHIFT;
            d         = a->reverse == 1 ? neg_val(step) : step;
            func_0209f30c(a, d);
        }
    }
    if (self->anims[0].active != 1) {
        return;
    }
    if (self->events != NULL) {
        if (self->anims[0].wrapped == 1 && self->anims[0].mode == 1) {
            self->b0       = B0_RESET;
            self->eventIdx = 0;
        }
        cur = self->anims[0].anm->frame >> FX_SHIFT;
        if (cur != frames[0]) {
            while (cur >= self->events[self->eventIdx].frame) {
                if (fire_event(self, &self->events[self->eventIdx], other) == 1) {
                    cur = self->anims[0].anm->frame >> FX_SHIFT;
                } else {
                    self->eventIdx++;
                }
            }
        }
    }
    if (self->blending != 1) {
        return;
    }
    if (self->anims[1].anm == NULL) {
        return;
    }
    if (self->blend >= FX_ONE) {
        return;
    }
    self->blend += BLEND_STEP;
    if (self->blend >= FX_ONE) {
        self->anims[0].anm->ratio = FX_ONE;
        NNS_G3dRenderObjRemoveAnmObj(self->renderObj, self->anims[1].anm);
        func_0209f2cc(&self->anims[1]);
        self->blend = 0;
    } else {
        self->anims[1].anm->ratio = FX_ONE - self->blend;
        self->anims[0].anm->ratio = self->blend;
    }
}
