#include "m2c.h"

extern M2C_UNK data_020f88c4;
static inline s16 TAB(s32 i) {
    return *(s16 *) ((s8 *) &data_020f88c4 + i * 2);
}
extern s32 data_020ff9b4[];
u32 func_02062b10(s32);
s32 func_02062ab4(s32, s32);
s32 func_020bba5c(s32);

typedef struct {
    u16 v;
} Ang;

static inline u16 angOf(Ang a) {
    return a.v;
}

typedef struct {
    s32 step;
    u8 flag;
    u8 count;
    u8 pad[2];
} Blk;
typedef struct {
    u8 pad0[0x48];
    Blk ang;
    Blk len;
    Blk hgt;
    s32 pad60[2];
    s32 lmin;
    s32 lmax;
    s32 range;
    s32 axis;
} Spec;
void func_020bdd74(s32 *out, Spec *spec) {
    s32 len, h, s, c, idx;
    u16 ang;
    out[0] = data_020ff9b4[0];
    out[1] = data_020ff9b4[1];
    out[2] = data_020ff9b4[2];
    if (spec->len.flag != 0) {
        len = spec->len.count * spec->len.step;
        len += spec->lmin;
    } else {
        len = func_02062ab4(spec->lmin, spec->lmax);
    }
    if (spec->hgt.flag != 0) {
        s32 r = *(volatile s32 *) &spec->range; // FAKE: keep the range load ahead of the product
        h     = spec->hgt.count * spec->hgt.step;
        h -= r >> 1;
    } else {
        s32 t = spec->range;
        h     = func_020bba5c(t);
        h -= t >> 1;
    }
    if (spec->ang.flag != 0) {
        ang = angOf((Ang) {(u16) (spec->ang.count * spec->ang.step)});
    } else {
        ang = angOf((Ang) {(u16) func_02062b10(0)});
    }
    idx = (ang >> 4) * 2;
    s   = TAB(idx);
    c   = TAB(idx + 1);
    switch (spec->axis) {
        case 0:
            out[0] = h;
            out[1] = (s32) (((s64) c * len + 0x800) >> 12);
            out[2] = (s32) (((s64) s * len + 0x800) >> 12);
            break;
        case 1:
            out[0] = (s32) (((s64) c * len + 0x800) >> 12);
            out[1] = h;
            out[2] = (s32) (((s64) s * len + 0x800) >> 12);
            break;
        case 2:
            out[0] = (s32) (((s64) c * len + 0x800) >> 12);
            out[1] = (s32) (((s64) s * len + 0x800) >> 12);
            out[2] = h;
            break;
    }
}
