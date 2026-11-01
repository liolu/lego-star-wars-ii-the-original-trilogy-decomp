#include "m2c.h"

typedef struct {
    s32 x, y, z;
} Vec3;

typedef struct {
    s32 x, y;
} Vec2;

typedef struct {
    u16 v;
} HalfWord;

typedef struct {
    s16 f00;
    s16 f02;
} ShortPair;

typedef struct {
    u8 f00;
    u8 f01;
    u8 f02;
    u8 f03;
} Byte4;

typedef struct {
    s32 v;
} UnkVal;

typedef struct {
    u8 d[8];
} Byte8;

typedef struct {
    s32 m[9];
    Vec3 pos;
    Vec3 scale;
    s16 rot[3];
    u8 pad42[2];
    u8 f44;
    u8 f45;
    u8 pad46[2];
} Xform;

typedef struct {
    u8 pad0[0x28];
    Vec3 target;
    u8 pad34[0x49 - 0x34];
    u8 dirty;
} Camera;

typedef struct {
    u8 pad0[8];
    u32 flags;
    u8 pad0c[0xb0 - 0xc];
    UnkVal px;
    s32 py;
    s32 pz;
    u8 padbc[0xd1 - 0xbc];
    u8 posDirty;
} Obj;

typedef struct {
    u8 pad0[8];
    s32 f08;
    u8 pad0c[0x28 - 0xc];
    u8 f28[8];
    u8 f30[8];
    void *f38;
    void *f3c;
    u8 f40[0x98 - 0x40];
    Obj *f98;
    Obj *f9c;
    u8 padA0[0xa9 - 0xa0];
    u8 fA9;
} Self;

extern u8 data_021027f8[];
extern u16 data_020ee020[];
extern u8 data_027e0874[];

Camera *func_020aaeb4(void);
void func_020a99bc(void *, u8);
void func_020a019c(void *, void *);
void func_020a0440(void *, UnkVal, u8);
void func_020534f4(void *, s32, s32, HalfWord);
void func_0205365c(void *, s32);
void func_02065600(void *, s32);
void func_02065658(void *, void *);
void func_0206574c(void *, s32, s32);
void func_0206653c(M2C_UNK, s32, s32, s32, s32, s32, s32);
void func_02001688(s32);
void func_020016c0(void);
void func_02055a18(s32, s32, s32, s32, s32, s32);
void func_020ac278(void *, s32, s32);
void G3X_SetClearColor(s32, s32, s16, s32, s32);
void func_02054c80(void *, void *, u32, u32, u32, u32, u32, u32, ShortPair *, Byte4);
void func_020536ec(void *, void *);
void func_0206d6cc(void *);
void func_0206d7fc(void *, s8);
void func_020b90e4(u32, u8);
Obj *func_02015138(u32, u32, void *);
void func_02001990(void *, s32);
void func_02074704(void *, s32);

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

static inline int vec_ne(const Vec3 *a, const Vec3 *b) {
    if (a->x != b->x || a->y != b->y || a->z != b->z) {
        return 1;
    }
    return 0;
}

static inline int fx_ne(const UnkVal *a, const UnkVal *b) {
    return a->v != b->v;
}

static inline void fx_set(UnkVal *d, const UnkVal *s) {
    d->v = s->v;
}

static inline void obj_set_x(Obj *o, UnkVal x) {
    if (fx_ne(&o->px, &x)) {
        fx_set(&o->px, &x);
        o->posDirty = 1;
    }
}

enum {
    OBJ_KIND_A      = 0x1a0, /* object kinds passed to the factory func_02015138 */
    OBJ_KIND_B      = 0x1a2,
    OBJ_FLAGS_CLEAR = 0x20120, /* bits cleared in each new object's flags word (+0x8) */
    OBJ_POS_X       = -0x5a000, /* x written to each new object's position (+0xb0) */
};

/* Entry 2 of the function table data_020fec0c (src/data/main_data_020fec0c.c).
 * Sets up the object returned by func_020aaeb4: func_020a99bc(.., 0), the vector at +0x28 is
 * replaced by (0, 0, -0x1000) when it differs (setting the byte at +0x49, the same compare/copy/flag
 * pattern as func_0207d394), then func_020a019c with (0, 0, 0x1e000) and func_020a0440 with 0x1000.
 * Initialises the members of self at +0x30 and +0x40 and runs a fixed sequence of engine calls.
 * Only when self->f08 is 0: func_020ac278, G3X_SetClearColor, the member at +0x28 (with func_02054c80 and
 * offset (0, -0x20000)) and the two sub-objects at +0x38/+0x3c.
 * Always: sets self->fA9 and creates two objects through func_02015138 from a 0x48-byte record
 * (only its fields from +0x24 on are written here), storing them at +0x98/+0x9c; each gets its
 * position x set (dirty byte +0xd1 when it changed), flag bits cleared and func_02001990(obj, 0).
 * Finally func_02074704(self, 0). x1/x2 stay separate locals: one shared local gives the wrong
 * stack-slot order for the by-value copies. */
void func_02073b68(Self *self) {
    Xform xf1;
    Xform xf2;
    Vec3 target;
    Vec3 eye;
    UnkVal uv;
    Byte8 out;
    ShortPair pair;
    Byte4 bytes;
    Vec2 ofs;
    UnkVal x1;
    UnkVal x2;
    const Vec3 *pv;
    Camera *cam;

    func_020a99bc(func_020aaeb4(), 0);
    target.x = 0;
    target.y = 0;
    target.z = -0x1000;
    pv       = &target;
    cam      = func_020aaeb4();
    if (vec_ne(&cam->target, pv)) {
        vec_set(&cam->target, pv);
        cam->dirty = 1;
    }
    eye.x = 0;
    eye.y = 0;
    eye.z = 0x1e000;
    func_020a019c(func_020aaeb4(), &eye);
    uv.v = 0x1000;
    func_020a0440(func_020aaeb4(), uv, data_021027f8[0x2a]);

    func_020534f4(self->f30, 1, 2, *(HalfWord *) &data_020ee020[1]);
    func_0205365c(self->f30, 0);
    func_02065600(self->f40, 4);
    func_02065658(self->f40, self->f30);
    func_0206574c(self->f40, 3, -1);
    func_0206653c(1, 0, 1, 3, 7, 7, 0);
    func_02001688(1);
    func_02055a18(0x8f9, 4, 3, 3, 6, 15);
    func_020016c0();

    if (self->f08 == 0) {
        func_020ac278(data_027e0874, 5, 0);
        G3X_SetClearColor(0, 0, 0x7fff, 0x3f, 0);
        func_020534f4(self->f28, 0, 1, *(HalfWord *) &data_020ee020[0]);
        func_0205365c(self->f28, 0);
        pair.f00  = 0;
        pair.f02  = 0;
        bytes.f00 = 0;
        bytes.f01 = 0;
        bytes.f02 = 0;
        bytes.f03 = 0;
        func_02054c80(&out, self->f28, 0x1399, 0, 1, 0x20, 0x20, 0xe0, &pair, bytes);
        ofs.x = 0;
        ofs.y = -0x20000;
        func_020536ec(self->f28, &ofs);
        func_0206d6cc(self->f38);
        func_0206d7fc(self->f38, 1);
        func_0206d6cc(self->f3c);
        func_0206d7fc(self->f3c, 1);
        func_020b90e4(0, 0xff);
    }

    self->fA9   = 1;
    xf1.pos.x   = 0;
    xf1.pos.y   = 0;
    xf1.pos.z   = 0;
    xf1.scale.x = 0x1000;
    xf1.scale.y = 0x1000;
    xf1.scale.z = 0x1000;
    xf1.rot[0]  = 0;
    xf1.rot[1]  = 0;
    xf1.rot[2]  = 0;
    xf1.f44     = 1;
    xf1.f45     = 1;
    self->f98   = func_02015138(OBJ_KIND_A, 0, &xf1);
    x1.v        = OBJ_POS_X;
    obj_set_x(self->f98, x1);
    self->f98->flags &= ~OBJ_FLAGS_CLEAR;
    func_02001990(self->f98, 0);

    xf2.pos.x   = 0;
    xf2.pos.y   = 0;
    xf2.pos.z   = 0;
    xf2.scale.x = 0x1000;
    xf2.scale.y = 0x1000;
    xf2.scale.z = 0x1000;
    xf2.rot[0]  = 0;
    xf2.rot[1]  = 0;
    xf2.rot[2]  = 0;
    xf2.f44     = 1;
    xf2.f45     = 1;
    self->f9c   = func_02015138(OBJ_KIND_B, 0, &xf2);
    x2.v        = OBJ_POS_X;
    obj_set_x(self->f9c, x2);
    self->f9c->flags &= ~OBJ_FLAGS_CLEAR;
    func_02001990(self->f9c, 0);

    func_02074704(self, 0);
}
