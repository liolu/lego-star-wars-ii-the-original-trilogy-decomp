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
    s16 a;
    s16 b;
} ShortPair;
typedef struct {
    u8 a;
    u8 b;
    u8 c;
    u8 d;
} Quad;
typedef struct {
    s32 a;
    s32 b;
} Pair32;
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
    return a->x != b->x || a->y != b->y || a->z != b->z;
}

static inline void vec_set(Vec3 *d, const Vec3 *s) {
    if (d != s) {
        d->x = s->x;
        d->y = s->y;
        d->z = s->z;
    }
}

class Node02082ef0 {
public:
    virtual void unknown00();
    virtual void release(); // Slot 0x04.
    virtual void unknown08();
    virtual void unknown0c();
    virtual void unknown10();
    virtual void unknown14();
    virtual void unknown18();
    virtual void unknown1c();
    virtual void unknown20();
    virtual void unknown24(s32, s32, s32, s32); // Slot 0x24.

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
    void SetScale(const Vec3 &v) {
        if (vec_ne(&scale, &v)) {
            vec_set(&scale, &v);
            dirty = 1;
        }
    }
};

struct Obj02082ef0 {
    u8 pad_00[0x50];
    u8 sub[0x98];
    Node02082ef0 *node;
    u8 pad_ec[0x14];
    s32 sel;
};

extern "C" void *func_0200ff50(void *, s32, s32, s32, void *);
extern "C" void func_02054c80(Pair32 *, void *, s32, s32, s32, s32, s32, s32, ShortPair *, Quad);
extern "C" s32 func_0205e80c(s32, s32, s32);
extern "C" s32 func_020833b0(void *, s32);

static inline void node_set_pos(Node02082ef0 *n, const Vec3 *v) {
    if (vec_ne(&n->pos, v)) {
        vec_set(&n->pos, v);
        n->dirty = 1;
    }
}

static inline void node_set_scale(Node02082ef0 *n, const Vec3 *v) {
    if (vec_ne(&n->scale, v)) {
        vec_set(&n->scale, v);
        n->dirty = 1;
    }
}

extern "C" void func_02082ef0(Obj02082ef0 *self, s32 sel) {
    if (self->sel == sel) {
        return;
    }
    self->sel = sel;
    if (self->node != 0) {
        self->node->release();
    }
    self->node = 0;
    if (sel != -1) {
        s32 id    = func_020833b0(self, sel);
        void *mem = (void *) func_0205e80c(0x1bc, 3, 4);
        Pair32 out;
        ShortPair pair;
        Quad q;
        Cfg cfg;

        if (mem != NULL) {
            cfg.pos.x   = 0;
            cfg.pos.y   = 0;
            cfg.pos.z   = 0;
            cfg.scale.x = 0x1000;
            cfg.scale.y = 0x1000;
            cfg.scale.z = 0x1000;
            cfg.r0      = 0;
            cfg.r1      = 0;
            cfg.r2      = 0;
            cfg.f44     = 1;
            cfg.f45     = 1;
            mem         = func_0200ff50(mem, 4, id, 1, &cfg);
        }
        self->node = (Node02082ef0 *) mem;
        self->node->unknown24(0x62, 1, 1, 0);

        self->node->SetPos(Vec3(0xe4000, 0x3e000, 0xe4000 - 0x1ac000));
        Node02082ef0 *node = self->node;
        node->SetScale(Vec3(0x6000, 0x6000, 0x6000));

        q.a    = 0xc;
        q.b    = 0x24;
        q.c    = 4;
        q.d    = 6;
        pair.a = 0x1b;
        pair.b = 0xc;
        func_02054c80(&out, (u8 *) self + 0x50, 0x59e, 0x10d, 1, 0x10, 0, 0, &pair, q);
    } else {
        Pair32 out;
        ShortPair pair;
        Quad q;

        pair.a = 0x1b;
        pair.b = 0xc;
        q.a    = 8;
        q.b    = 0x24;
        q.c    = 4;
        q.d    = 6;
        func_02054c80(&out, (u8 *) self + 0x50, 0x59e, 0x10d, 1, 0x10, 0, 0, &pair, q);
    }
}
