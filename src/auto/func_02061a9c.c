#include "m2c.h"

/* Clamps obj->pos to [-0x1000, 0xfff] per axis, then light_set: func_020ac828(data_027e0874, idx + 4, 1)
 * (its cases 4..7 store the s8 at +0xe of 16-byte entry idx), stores the inverted colour (0x1f - c per
 * channel) and copies obj->pos into entry idx. Finally the non-inverted colour goes to +0x42 and, with
 * 0x1f0000 or-ed in, to 0x4000358: the same +0x42 / 0x4000358 pair as func_02059a24 and func_020aca00,
 * which also call G3X_SetFog.
 */

enum {
    REG_FOG_COLOR = 0x4000358
};

typedef struct {
    s32 v;
} F;
typedef struct {
    F x, y, z;
} Vec3;
typedef struct {
    s32 x, y, z;
} IVec3;

typedef struct {
    Vec3 pos;
    u16 color;
} Slot;

typedef struct {
    u8 pad[0x4a0];
    Vec3 pos;
    s32 pad4ac;
    IVec3 col;
} Obj;

extern Slot data_027e0874[];
extern void func_020ac828(void *, u32, s8);

static inline void clamp_axis(F *p, s32 lo, s32 hi) {
    F mn, mx, lk, hk, lp, m, hp, v;
    F *t, *u;
    v    = *p;
    lk.v = lo;
    lp   = lk;
    if (v.v < lp.v) {
        mx = lp;
        t  = &mx;
    } else {
        mx = v;
        t  = &mx;
    }
    hk.v = hi;
    hp   = hk;
    m    = *t;
    if (m.v < hp.v) {
        mn = m;
        u  = &mn;
    } else {
        mn = hp;
        u  = &mn;
    }
    *p = *u;
}

static inline u16 rgb(s32 r, s32 g, s32 b) {
    return r | (g << 5) | (b << 10);
}

static inline void light_set(s32 idx, Vec3 *pos, u16 *col) {
    func_020ac828(data_027e0874, idx + 4, 1);
    // FAKE: volatile read keeps the caller's u16 col in its stack slot at sp+0 (strh before the call, ldrh
    // after it); without it mwcc promotes the address-taken u16 to a register.
    data_027e0874[idx].color = *(volatile u16 *) col;
    if (&data_027e0874[idx].pos != pos) {
        data_027e0874[idx].pos.x.v = pos->x.v;
        data_027e0874[idx].pos.y.v = pos->y.v;
        data_027e0874[idx].pos.z.v = pos->z.v;
    }
}

void func_02061a9c(Obj *obj, s32 idx) {
    u16 col;
    clamp_axis(&obj->pos.x, -0x1000, 0xfff);
    clamp_axis(&obj->pos.y, -0x1000, 0xfff);
    clamp_axis(&obj->pos.z, -0x1000, 0xfff);

    col = rgb(0x1f - (obj->col.x >> 12), 0x1f - (obj->col.y >> 12), 0x1f - (obj->col.z >> 12));
    light_set(idx, &obj->pos, &col);
    *(u16 *) ((u8 *) data_027e0874 + 0x42) = rgb(obj->col.x >> 12, obj->col.y >> 12, obj->col.z >> 12);
    *(s32 *) REG_FOG_COLOR                 = *(u16 *) ((u8 *) data_027e0874 + 0x42) | 0x1f0000;
}
