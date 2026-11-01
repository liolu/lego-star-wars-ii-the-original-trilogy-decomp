/* Reconstructed from USA ARM9 at 0x020ac130 (328 bytes).
 *
 * Fills four 16-byte light entries of the caller's scene struct: each gets an
 * (x, y, z) vector, the diffuse colour (*diffuse) and enabled = 1. The vector
 * component func_02007f24 produces is 0.9 converted to fixed point (the
 * callee multiplies by 4096.0 and truncates):
 *   entry 0: (+0.9, k, k)   entry 1: (-0.9, k, k)
 *   entry 2: (k, k, +0.9)   entry 3: (k, k, -0.9)
 * where k = data_020ff9a8. Then the global material colours are set:
 * diffuse/ambient from the two GXRgb arguments (no vertex colour), and
 * specular 0x1f (GX_RGB red 31) with emission 0 and the shininess table on.
 * Caller func_020abf98 passes white (31,31,31) as diffuse and grey (12,12,12)
 * as ambient.
 */
#include "m2c.h"

typedef struct {
    s32 x;
    s32 y;
    s32 z;
    u16 color;
    u8 enabled;
    u8 pad;
} Light;

typedef struct {
    Light l[4];
} Lights;

extern s32 data_020ff9a8;

void func_02007f24(s32 *, double);
void NNS_G3dGlbMaterialColorDiffAmb(s32, s32, s32);
void NNS_G3dGlbMaterialColorSpecEmi(s32, s32, s32);

static inline s32 negp(s32 *a) {
    s32 r  = *a;
    s32 *p = a; // FAKE: local copy + always-true null test keeps the add rX,sp / ldr form (see func_020351c0)
    if (p) {
        r = -*p;
    }
    return r;
}

enum {
    SPECULAR_COLOR = 0x1f,
};

void func_020ac130(Lights *arg0, u16 *diffuse, u16 *ambient) {
    s32 a, b, c, d, nz;

    arg0->l[0].color = *diffuse;
    func_02007f24(&a, 0.9);
    arg0->l[0].x       = a;
    arg0->l[0].y       = data_020ff9a8;
    arg0->l[0].z       = data_020ff9a8;
    arg0->l[0].enabled = 1;

    arg0->l[1].color = *diffuse;
    func_02007f24(&b, 0.9);
    arg0->l[1].x       = negp(&b);
    arg0->l[1].y       = data_020ff9a8;
    arg0->l[1].z       = data_020ff9a8;
    arg0->l[1].enabled = 1;

    arg0->l[2].color = *diffuse;
    func_02007f24(&c, 0.9);
    arg0->l[2].x       = data_020ff9a8;
    arg0->l[2].y       = data_020ff9a8;
    arg0->l[2].z       = c;
    arg0->l[2].enabled = 1;

    arg0->l[3].color = *diffuse;
    func_02007f24(&d, 0.9);
    nz                 = negp(&d); // FAKE: negation hoisted above the data loads, as in the original schedule
    arg0->l[3].x       = data_020ff9a8;
    arg0->l[3].y       = data_020ff9a8;
    arg0->l[3].z       = nz;
    arg0->l[3].enabled = 1;

    NNS_G3dGlbMaterialColorDiffAmb(*diffuse, *ambient, 0);
    NNS_G3dGlbMaterialColorSpecEmi(SPECULAR_COLOR, 0, 1);
}
