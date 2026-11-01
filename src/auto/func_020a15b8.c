#include "m2c.h"

typedef struct {
    s32 v;
} Fx;
typedef struct {
    Fx x, y, z;
} VecFx;
typedef struct {
    u16 v;
} Ang;

typedef struct {
    u8 pad0[0xB0];
    VecFx pos;
} Target;

typedef struct {
    u8 pad0[0x1D0];
    Target *target;
    u8 pad1[0x36C - 0x1D4];
    s8 mode;
    u8 pad2;
    Ang limit;
    u8 pad3[2];
    Ang ang;
} Obj;

extern s16 func_020985d8(s32, s32);

/* Values written to Obj.ang (0x372); the second is the 16-bit negation of the first. */
enum {
    ANG_DEFAULT = 0x38E,
    ANG_FLIPPED = 0xFC72
};

static inline void ang_init(Ang *a, u16 v) {
    a->v = v;
}

static inline int ang_lt(u16 a, Ang b) {
    return a < b.v;
}

static inline void set_ang(Obj *o, Ang a) {
    o->ang.v = a.v;
}

/* func_020985d8 is the angle-from-(x, z) helper, named fx_atan2 as in func_0202abc8.c. */
static inline u16 fx_atan2(Fx x, Fx z) {
    return func_020985d8(x.v, z.v);
}

/*
 * Stores the s8 mode at 0x36C and sets ang (0x372) to ANG_DEFAULT. When the
 * stored mode is -1 and the angle from the linked object's (x, z) position
 * (0xB0, 0xB8; y zeroed and never read) is below limit (0x36E), ang is set
 * to ANG_FLIPPED instead.
 */
void func_020a15b8(Obj *self, s8 mode) {
    VecFx pos;
    Fx qz;
    Fx qx;
    Ang ang_default;
    Ang ang_flipped;

    pos.x      = self->target->pos.x;
    pos.z      = self->target->pos.z;
    pos.y.v    = 0;
    self->mode = mode;
    ang_init(&ang_default, ANG_DEFAULT);
    set_ang(self, ang_default);
    if (self->mode == -1) {
        qz = pos.z;
        qx = pos.x;
        if (ang_lt(fx_atan2(qx, qz), self->limit)) {
            ang_init(&ang_flipped, ANG_FLIPPED);
            set_ang(self, ang_flipped);
        }
    }
}
