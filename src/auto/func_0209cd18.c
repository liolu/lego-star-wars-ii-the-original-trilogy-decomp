#include "m2c.h"

typedef struct Vec3 {
    s32 x, y, z;
} Vec3;

typedef struct Obj {
    u8 pad0[6];
    s8 flags[0x1e];
    s32 a[4];
    Vec3 pos[4];
    s32 b[4];
    s32 c[4];
    s32 d[4];
    u16 e[4];
    s32 f[4];
    s32 g[4];
    s32 h[4];
    s32 k[4];
    u16 l[4];
    u16 m[4];
    u8 n[4];
    s8 neg;
    u8 pad1;
    u16 o[4];
    u16 p[4];
    u16 q[4];
    u16 r[4];
    u16 s[4];
    u16 t[4];
    u16 w[4];
} Obj;

extern s32 data_02109258;
extern u32 data_0210930c[];
extern void func_02001688(s32);
extern void func_020016c0(void);
extern Vec3 *func_0209ce38(void);

void func_0209cd18(Obj *this) {
    u8 i;
    s32 *g;
    func_02001688(0);
    g = &data_02109258;
    for (i = 0; i < 4; i++) {
        Vec3 *src;
        this->a[i] = 0;
        this->b[i] = 0x1E13380;
        this->c[i] = 0;
        this->d[i] = 0;
        this->e[i] = 0;
        this->f[i] = *g;
        this->g[i] = 0;
        this->h[i] = 0;
        this->l[i] = 0;
        this->m[i] = 0;
        this->n[i] = 0;
        this->neg  = -1;
        this->o[i] = 0;
        this->p[i] = 0;
        this->q[i] = 0;
        this->r[i] = 0;
        this->s[i] = 0;
        this->t[i] = 0;
        this->w[i] = 0;
        this->k[i] = 0;
        src        = (Vec3 *) data_0210930c[i + 0x13];
        if (src != 0) {
            src = (Vec3 *) ((u8 *) src + 0xb0);
            if (&this->pos[i] != src) {
                this->pos[i].x = src->x;
                this->pos[i].y = src->y;
                this->pos[i].z = src->z;
            }
        } else {
            func_0209ce38();
        }
    }
    for (i = 0; i < 30; i++) {
        this->flags[i] = -1;
    }
    func_020016c0();
}
