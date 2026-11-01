#include "m2c.h"

struct Vec3 {
    s32 x;
    s32 y;
    s32 z;

    Vec3() {}
    Vec3(s32 vx, s32 vy, s32 vz) {
        x = vx;
        y = vy;
        z = vz;
    }
};

typedef struct {
    u8 pad[0x24];
    Vec3 pos;
    Vec3 scale;
    s16 r0;
    s16 r1;
    s16 r2;
    u8 gap[2];
    u8 f44;
    u8 f45;
} Cfg;

static inline s32 vec_ne(const Vec3 *a, const Vec3 *b) {
    if (a->x != b->x || a->y != b->y || a->z != b->z) {
        return 1;
    }
    return 0;
}

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

class Node0207e108 {
public:
    virtual void unknown00();
    virtual void unknown04();
    virtual void unknown08();
    virtual void unknown0c();
    virtual void unknown10();
    virtual void unknown14();
    virtual void unknown18();
    virtual void unknown1c();
    virtual void unknown20();
    virtual void unknown24(s32, s32, s32, s32);

    u8 pad_04[0xac];
    Vec3 pos;
    Vec3 scale;
    u8 pad_c8[9];
    u8 dirty;

    void SetPos(const Vec3 &v) {
        if (vec_ne(&pos, &v)) {
            vec_set(&pos, &v);
            dirty = 1;
        }
    }
    void SetScale(s32 s) {
        SetScale(Vec3(s, s, s));
    }
    void SetScale(const Vec3 &v) {
        if (vec_ne(&scale, &v)) {
            vec_set(&scale, &v);
            dirty = 1;
        }
    }
};

typedef struct {
    u8 pad[2];
    u16 h2;
} Anim;

typedef struct {
    u8 pad[6];
    u16 count;
} AnimInfo;

typedef struct {
    u8 pad[8];
    s32 flags;
    u8 pad_0c[0x14];
    Anim *anim;
} Model;

typedef struct {
    u8 pad[0x28];
    Model *model;
} Actor;

typedef struct {
    s32 f0;
    u8 pad_04[4];
    void *f8;
    void *fc;
    s32 *f10;
    Actor *f14;
    u8 pad_18[4];
    s32 f1c;
    s32 f20;
    u8 f24;
    u8 pad_25[2];
    u8 f27;
    u8 pad_28[0x10];
    Node0207e108 *node;
} Self;

typedef struct {
    u8 pad[0xf5];
    u8 scale;
} Settings;

extern "C" {
s32 func_0208559c(void *);
s32 func_02094038(s32);
u32 func_0208c7b0(void *);
void func_0205365c(s32 *, s32);
void func_0205ea38(Actor **, s32);
void func_0206574c(void *, s32, s32);
void func_02065be8(void *, s32, s32, s32, void *, s32);
void *func_02066708(s32);
void func_0207e97c(void *, s32, s32);
void *func_0205e80c(s32, s32, s32);
Node0207e108 *func_0200ff50(void *, s32, s32, s32, Cfg *);
AnimInfo *func_0205ab44(u16);
void func_0201b318(Model *, u8);

extern M2C_UNK data_02104d94;
extern M2C_UNK data_02104dc4;
extern s32 data_027e02a8;
extern u8 data_02106d90[];
extern Settings data_020ff16c;
}

/*
 * Progress step: pct = func_0208559c(&data_02104d94) * 100 / func_02094038(self->f0).
 * Clears f20/f24, picks the f1c flag (func_0208c7b0, or byte 0x8d2 of data_02106d90 when
 * bit 0x40 of data_027e02a8 is set). Returns 0 when pct <= 0. Otherwise it resets the
 * f10/f14/fc/f8 objects, runs func_0207e97c(self, 0, pct) and sets f27 = 0xd. With f1c set it
 * builds a node (0x1bc bytes, unit pos/scale config) into self->node, calls its slot-0x24
 * virtual twice, sets a uniform scale from data_020ff16c byte 0xf5 (fx32) and a fixed
 * position. Without f1c it shows frame pct / 10, clamped to the anim's last frame.
 * Returns 1.
 */
extern "C" s32 func_0207e108(Self *self) {
    enum {
        PERCENT    = 100,
        FX32_ONE   = 0x1000,
        NODE_SIZE  = 0x1bc,
        FRAME_STEP = 10,
    };
    s32 pct;

    pct       = func_0208559c(&data_02104d94) * PERCENT / func_02094038(self->f0);
    self->f1c = func_0208c7b0(&data_02104dc4);
    if (data_027e02a8 & 0x40) {
        self->f1c = data_02106d90[0x8d2];
    }
    self->f20 = 0;
    self->f24 = 0;
    if (pct > 0) {
        func_0205365c(self->f10, 0);
        func_0205ea38(&self->f14, 0);
        self->f14->model->flags |= 1;
        self->f14->model->flags |= 8;
        func_0206574c(self->fc, 9, -2);
        func_02065be8(self->fc, 0x8c, 0x80, 0x23, func_02066708(0x154), 1);
        func_0206574c(self->f8, 0xc, -2);
        func_0207e97c(self, 0, pct);
        self->f27 = 0xd;
        if (self->f1c != 0) {
            Cfg cfg;
            void *mem = func_0205e80c(NODE_SIZE, 3, 4);
            s32 uniform_scale;

            if (mem != (void *) 0) {
                cfg.pos.x   = 0;
                cfg.pos.y   = 0;
                cfg.pos.z   = 0;
                cfg.scale.x = FX32_ONE;
                cfg.scale.y = FX32_ONE;
                cfg.scale.z = FX32_ONE;
                cfg.r0      = 0;
                cfg.r1      = 0;
                cfg.r2      = 0;
                cfg.f44     = 1;
                cfg.f45     = 1;
                mem         = func_0200ff50(mem, 4, 0x246, 1, &cfg);
            }
            self->node = (Node0207e108 *) mem;
            self->node->unknown24(self->f20 + 0xe4, 0, 1, 0);
            self->node->unknown24(self->f20 + 0x79, 0, 1, 0);
            uniform_scale = data_020ff16c.scale << 12;
            self->node->SetScale(uniform_scale);
            self->node->SetPos(Vec3(0, -0x50000, -0xfa000));
            self->f14->model->flags |= 8;
        } else {
            u8 frame = pct / FRAME_STEP;

            if (frame >= func_0205ab44(self->f14->model->anim->h2)->count) {
                frame = func_0205ab44(self->f14->model->anim->h2)->count - 1;
            }
            func_0201b318(self->f14->model, frame);
        }
        return 1;
    }
    return 0;
}
