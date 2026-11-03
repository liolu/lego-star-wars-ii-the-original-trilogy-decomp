#include "m2c.h"

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct {
    u16 r : 5;
    u16 g : 5;
    u16 b : 5;
    u16 pad : 1;
} Rgb;

typedef struct {
    Vec3 dir;
    u16 color;
    u16 pad;
} Light;

typedef union {
    u16 raw;
    Rgb rgb;
} Color;

typedef struct {
    u8 counter;
    u8 mode : 7;
    u8 flag : 1;
} Obj;

extern void NNS_G3dGlbMaterialColorSpecEmi(s32 arg0, s32 arg1, s32 arg2);
extern Light data_027e0874[];

enum {
    LIGHT_COUNT      = 4,
    SCALE_ONE        = 32,
    SCALE_QUARTER    = 8,
    SCALE_HALF       = 16,
    COLOR_WHITE      = 0x7fff,
    COLOR_R8_G31_B31 = 0x7fe8,
};

static inline u16 rgb_scale(u16 raw, s32 k) {
    Color c;
    c.raw   = raw;
    c.rgb.r = (u16) ((c.rgb.r * k) >> 5);
    c.rgb.g = (u16) ((c.rgb.g * k) >> 5);
    c.rgb.b = (u16) ((c.rgb.b * k) >> 5);
    return c.raw;
}

static inline void light_set(s32 i, const Light *src, u16 color) {
    data_027e0874[i].color = color;
    if (&data_027e0874[i] != src) {
        data_027e0874[i].dir.x = src->dir.x;
        data_027e0874[i].dir.y = src->dir.y;
        data_027e0874[i].dir.z = src->dir.z;
    }
}

static inline void lights_scale(s32 k) {
    s32 i;
    for (i = 0; i < LIGHT_COUNT; i++) {
        const Light *l = &data_027e0874[i];
        light_set(i, l, rgb_scale(l->color, k));
    }
}

/* Lighting mode o->mode: modes 1-3 scale the colour of each of the 4 lights in data_027e0874 by k/32
 * (k = 32 - counter*3/2, 8, or 16 when counter bit 3 is set) and copy the direction only when the source light
 * differs; every mode then sets the material specular colour to white and the emission colour (counter-based in modes
 * 1-2) with NNS_G3dGlbMaterialColorSpecEmi. */
void func_0200ba44(Obj *o) {
    switch (o->mode) {
        case 0:
            NNS_G3dGlbMaterialColorSpecEmi(COLOR_WHITE, 0, 0);
            break;
        case 1:
            lights_scale(SCALE_ONE - ((o->counter * 3) >> 1));
            NNS_G3dGlbMaterialColorSpecEmi(COLOR_WHITE, (u16) (o->counter * 2), 0);
            break;
        case 2:
            lights_scale(SCALE_QUARTER);
            NNS_G3dGlbMaterialColorSpecEmi(COLOR_WHITE, (u16) ((o->counter << 1) | (o->counter << 6) | (o->counter << 11)), 0);
            break;
        case 3:
            if (o->counter & 8) {
                lights_scale(SCALE_HALF);
                NNS_G3dGlbMaterialColorSpecEmi(COLOR_WHITE, COLOR_R8_G31_B31, 0);
            } else {
                NNS_G3dGlbMaterialColorSpecEmi(COLOR_WHITE, 0, 0);
            }
            break;
    }
}
