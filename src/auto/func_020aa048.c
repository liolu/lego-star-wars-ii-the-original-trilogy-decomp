#include "m2c.h"

typedef struct {
    u16 v;
} Half;

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct {
    u8 pad000[0x28 - 0x0];
    Vec3 pos;
    u8 pad034[0x49 - 0x34];
    u8 f49;
    u8 f4a;
    u8 pad04b[0x1d0 - 0x4b];
    s32 f1d0;
    s32 f1d4;
    Vec3 v1d8;
    Vec3 v1e4;
    Vec3 v1f0;
    u8 pad1fc[0x200 - 0x1fc];
    s32 f200;
    s32 f204;
    u8 f208[4];
    Vec3 v20c;
    s32 f218;
    s32 f21c;
    s32 f220;
    Vec3 v224;
    Vec3 v230;
    u8 f23c;
    u8 f23d;
    u8 f23e;
    u8 f23f;
    u8 f240;
    u8 f241;
    u8 f242;
    u8 pad243[0x244 - 0x243];
    s32 f244;
    s32 f248;
    s32 f24c;
    Vec3 v250;
    u8 f25c;
    u8 pad25d[0x264 - 0x25d];
    Vec3 v264;
    Half f270;
    u8 pad272[0x280 - 0x272];
    s32 f280;
    s32 f284;
    s32 f288;
    s32 f28c;
    u8 f290;
    u8 f291;
    u8 f292;
    u8 pad293[0x297 - 0x293];
    u8 f297;
    u8 f298;
    u8 pad299[0x29c - 0x299];
    Vec3 v29c;
    s32 f2a8;
    s32 f2ac;
    Vec3 v2b0;
    Vec3 v2bc;
    s32 f2c8;
    s32 f2cc;
    s32 f2d0;
    u8 f2d4;
    u8 pad2d5[0x2d8 - 0x2d5];
    s32 f2d8;
    s32 f2dc;
    u8 pad2e0[0x2f4 - 0x2e0];
    s32 f2f4;
    u8 f2f8;
    u8 pad2f9[0x2fc - 0x2f9];
    s32 f2fc;
    s32 f300;
    s32 f304;
    s32 f308;
    u8 f30c;
    u8 pad30d[0x340 - 0x30d];
    Vec3 v340;
    u8 pad34c[0x360 - 0x34c];
    u8 f360;
    u8 pad361[0x364 - 0x361];
    s32 f364;
    u8 pad368[0x374 - 0x368];
    s32 f374;
    u8 f378;
    u8 pad379[0x37c - 0x379];
    s32 f37c;
    s32 f380;
    s32 f384;
    s32 f388;
    s32 f38c;
    u8 pad390[0x3cc - 0x390];
    Vec3 v3cc;
    u8 f3d8;
    u8 f3d9;
} Obj;

void func_020aaa48(Obj *obj);
void func_02007f24(s32 *out, f64 value);
void func_020b04f0(void *obj, u8 id, void *src, u8 size);

extern const Vec3 data_020ff9b4;
extern s32 data_027e02a8;
extern u8 data_0210794c;

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

typedef struct {
    s32 v;
} Fx;

static inline Half half(u16 v) {
    Half r;
    r.v = v;
    return r;
}

static inline void half_set(Half *d, Half s) {
    d->v = s.v;
}

/* Stores v into *f and yields a copy for a by-value Fx argument. */
static inline Fx fx_init(Fx *f, s32 v) {
    f->v = v;
    return *f;
}

static inline void vec_init(Vec3 *d, Fx x, Fx y, Fx z) {
    d->x = x.v;
    d->y = y.v;
    d->z = z.v;
}

/* Copies (x, y, z) into obj->pos and sets f49 only when the position changed. */
static inline void obj_set_pos(Obj *obj, Fx x, Fx y, Fx z) {
    Vec3 v;
    Vec3 *pv;
    s32 changed;

    vec_init(&v, x, y, z);
    pv      = &v;
    changed = 0;
    if (obj->pos.x != pv->x || obj->pos.y != pv->y || obj->pos.z != pv->z) {
        changed = 1;
    }
    if (changed != 0) {
        vec_set(&obj->pos, pv);
        obj->f49 = 1;
    }
}

/* Resets obj state: pos goes to the origin, most vectors are reset from
 * data_020ff9b4, and six fields take the value func_02007f24 writes for a
 * double constant (through the locals t0..t5). */
void func_020aa048(Obj *obj) {
    Vec3 v1;
    Vec3 v2;
    Vec3 v3;
    Fx x;
    Fx y;
    Fx z;
    s32 t0, t1, t2, t3, t4, t5;
    Vec3 *pv;
    u8 i;

    obj->f2f8 = 0;
    vec_set(&obj->v1e4, &data_020ff9b4);
    vec_set(&obj->v3cc, &data_020ff9b4);
    obj->f2f4 = 0;
    obj->f2fc = 0;
    obj->f300 = 0;
    obj->f304 = 0;
    obj->f308 = 0;
    obj_set_pos(obj, fx_init(&x, 0), fx_init(&y, 0), fx_init(&z, 0));
    obj->f25c = 0;
    obj->f1d0 = 0;
    vec_set(&obj->v1d8, &data_020ff9b4);
    vec_set(&obj->v1e4, &data_020ff9b4);
    obj->f30c = 3;
    obj->f21c = 0;
    obj->f220 = 0;
    func_020aaa48(obj);
    obj->f240 = 1;
    obj->f360 = 1;
    obj->f364 = 0x2000;
    vec_set(&obj->v1f0, &data_020ff9b4);
    obj->f200 = 0;
    vec_set(&obj->v20c, &data_020ff9b4);
    obj->f218 = 0x1000;
    vec_set(&obj->v224, &data_020ff9b4);
    vec_set(&obj->v230, &data_020ff9b4);
    obj->f23c = 0;
    obj->f23e = 1;
    obj->f23d = 1;
    obj->f204 = 0;
    obj->f2c8 = 0x5000;
    func_02007f24(&t0, 0.25);
    obj->f2cc = t0;
    func_02007f24(&t1, 0.2f);
    obj->f2d0 = t1;
    v1.x      = 0x7000;
    v1.y      = 0x14000;
    v1.z      = -0x18000;
    pv        = &v1;
    vec_set(&obj->v340, pv);
    for (i = 0; i < 4; i++) {
        obj->f208[i] = 3;
    }
    v2.x = 0;
    v2.y = 0x18000;
    v2.z = 0x28000;
    pv   = &v2;
    vec_set(&obj->v250, pv);
    obj->f297 = 0;
    obj->f298 = 0;
    vec_set(&obj->v29c, &data_020ff9b4);
    func_02007f24(&t2, 0.0);
    obj->f2a8 = t2;
    obj->f2ac = 0;
    obj->f280 = 0x1e000;
    obj->f284 = 0x1e000;
    obj->f288 = 0x3c000;
    obj->f28c = 0x5000;
    obj->f290 = 0;
    obj->f23f = 1;
    if (data_027e02a8 & 0x40) {
        func_020b04f0(&data_0210794c, 0x3e, 0, 0xff);
    }
    obj->f241 = 0;
    obj->f242 = 0;
    obj->f244 = -0x64000;
    obj->f248 = 0x64000;
    obj->f24c = 0x1e000;
    v3.x      = 0x1e000;
    v3.y      = 0x1e000;
    v3.z      = 0x1e000;
    pv        = &v3;
    vec_set(&obj->v264, pv);
    obj->f240 = 1;
    obj->f2d4 = 1;
    obj->f2d8 = 0x5000;
    obj->f2dc = 0x1000;
    vec_set(&obj->v2b0, &data_020ff9b4);
    vec_set(&obj->v2bc, &data_020ff9b4);
    obj->v2bc.y = 0x5000;
    obj->f374   = 0;
    obj->f3d8   = 0;
    half_set(&obj->f270, half(0));
    obj->f37c = 0;
    obj->f378 = 1;
    obj->f380 = 0x5000;
    func_02007f24(&t3, 0.5);
    obj->f384 = t3;
    func_02007f24(&t4, 9.0);
    obj->f388 = t4;
    func_02007f24(&t5, 1.25);
    obj->f38c = t5;
    obj->f1d4 = 0;
    obj->f3d9 = 0;
    obj->f291 = 1;
    obj->f292 = 0;
    obj->f4a  = 1;
}
