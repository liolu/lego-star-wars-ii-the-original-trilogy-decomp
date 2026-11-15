#include "m2c.h"

extern s32 FX_Sqrt(s32 x);
extern s32 FX_Div(s32 a, s32 b);
extern void clearDataAndInstructionCache(void *a, void *b);

typedef struct Effect {
    void *vtbl;
    u8 pad0[4];
    u32 flags;
    u8 pad1[0x40];
    s32 pos_x;
    s32 pos_z;
} Effect;

typedef struct {
    u8 pad0[0xC];
    u16 kind;
    u8 pad1[0xB0 - 0xE];
    s32 x;
    u8 pad2[4];
    s32 z;
    u8 pad3[0x6BB - 0xBC];
    u8 type;
} Actor;

typedef struct {
    Actor *actor;
    s32 pad;
} Slot;

typedef struct {
    u8 pad0[0xB0];
    s32 x;
    u8 pad1[4];
    s32 z;
    u8 pad2[0x6F8 - 0xBC];
    Slot slots[32];
    u8 pad3[4];
    u32 count;
} World;

typedef struct {
    u8 pad0[0x18];
    Effect *eff[5];
    u8 pad1[0x2D - 0x2C];
    u8 cur[5];
} Owner;

extern u8 data_021027f8[];
extern World *data_02109358[];
extern u8 data_020f7804[];

typedef struct {
    s32 v;
} Fx;
static inline Fx dist_sq(s32 dx, s32 dz) {
    Fx r;
    r.v = (s32) (((s64) dx * dx + 0x800) >> 12) + (s32) (((s64) dz * dz + 0x800) >> 12);
    return r;
}

enum {
    EFFECT_COUNT = 5
};
enum {
    TYPE_FIRST = 0x54,
    TYPE_LAST  = 0x63
};
enum {
    NEAR_DIST = 0x58000
};   /* 88.0 in fx32 */
enum {
    FX32_TWO = 0x2000
};     /* 2.0 in fx32 */
enum {
    POS_X_OFFSET = 0x7F000,
    POS_Z_OFFSET = 0x54000
};   /* 127.0 and 84.0 in fx32 */

/* Walks the world's actor slots and gives each of the EFFECT_COUNT effects to one actor of kind 1 or 2 with a
   type in TYPE_FIRST..TYPE_LAST that lies closer than NEAR_DIST to the world position (x/z plane). The effect
   data is reloaded when the type changes, flag bit 0 is set and the effect position becomes half the x/z
   offset plus a fixed offset. Effects left over get flag bit 0 cleared. */
void func_0209ecc0(Owner *self) {
    u32 n;
    u32 i;
    Actor *obj;
    s32 dz, dx;
    World *world;
    world = data_02109358[data_021027f8[0x2A]];
    n     = 0;

    for (i = 0; n < EFFECT_COUNT && i < world->count; i++) {
        obj = world->slots[i].actor;
        if (obj != NULL && (u32) (obj->kind - 1) <= 1 && obj->type >= TYPE_FIRST && obj->type <= TYPE_LAST) {
            Fx d;
            dz = obj->z - world->z;
            dx = obj->x - world->x;
            d  = dist_sq(dx, dz);
            if (FX_Sqrt(d.v) < NEAR_DIST) {
                if (self->cur[n] != obj->type - TYPE_FIRST) {
                    clearDataAndInstructionCache(self->eff[n], data_020f7804 + 2 + (obj->type - TYPE_FIRST) * 6);
                    self->cur[n] = obj->type - TYPE_FIRST;
                }
                self->eff[n]->flags |= 1;
                {
                    s32 half_dz = FX_Div(dz, FX32_TWO);
                    s32 half_dx = FX_Div(dx, FX32_TWO);
                    s32 z       = half_dz + POS_Z_OFFSET;
                    s32 x       = half_dx + POS_X_OFFSET;
                    Effect *e   = self->eff[n];

                    // FAKE: each position field is stored twice; the repeat is dropped late, but it keeps z and x
                    // as separate values made before the eff load (single stores give eff r2, x r1, z r0).
                    e->pos_x = x;
                    e->pos_x = x;
                    e->pos_z = z;
                    e->pos_z = z;
                }
                n++;
            }
        }
    }
    if (n >= EFFECT_COUNT) {
        return;
    }
    do {
        self->eff[n]->flags &= ~1u;
        n++;
    } while (n < EFFECT_COUNT);
}
