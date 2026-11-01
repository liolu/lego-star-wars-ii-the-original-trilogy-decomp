#include "m2c.h"

enum {
    FX32_SHIFT = 12,
    FX32_ONE   = 0x1000
};
enum {
    NODE_ALLOC_SIZE = 0x1bc
};

struct Vec3 {
    s32 x;
    s32 y;
    s32 z;
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

class Node0207ce0c {
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
    virtual void unknown24(s32, s32, s32, s32); // Slot 0x24.

    u8 pad_04[0xac];
    Vec3 pos;
    Vec3 scale;
    u8 pad_c8[9];
    u8 dirty;
};

typedef struct {
    s32 id;
    u8 pad[4];
} IdEntry;

typedef struct {
    u8 size;
    u8 pad[7];
} SizeEntry;

struct Obj0207ce0c {
    u8 pad_00[0x2c];
    u32 kind;
    u8 pad_30[0x10];
    u8 sub[0x60];
    s32 fa0;
    u8 pad_a4[0xc];
    Node0207ce0c *node;
};

extern "C" void *func_0200ff50(void *, s32, s32, s32, void *);
extern "C" s32 func_0205e80c(s32, s32, s32);
extern "C" void func_0206574c(void *, s32, s32);
extern "C" s32 func_02065be8(void *, s32, s32, s32, s32, s32);
extern "C" void func_020661b4(void *, s32);
extern "C" s32 func_02066708(s32);
extern "C" s32 func_0208c72c(void *, u32);

extern "C" u8 data_02104dc4[];
extern "C" s32 data_027e02a8;
extern "C" u8 data_02106d90[];
extern "C" const IdEntry data_020ff1dc[];
extern "C" const SizeEntry data_020ff1e0[];

static inline void vec_init(Vec3 *v, s32 x, s32 y, s32 z) {
    v->x = x;
    v->y = y;
    v->z = z;
}

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

static inline void node_set_scale(Node0207ce0c *n, const Vec3 *v) {
    if (vec_ne(&n->scale, v)) {
        vec_set(&n->scale, v);
        n->dirty = 1;
    }
}

static inline void node_set_pos(Node0207ce0c *n, const Vec3 *v) {
    if (vec_ne(&n->pos, v)) {
        vec_set(&n->pos, v);
        n->dirty = 1;
    }
}

/*
 * Updates self->sub from a value looked up for self->kind (replaced by byte
 * 0x8d1 of data_02106d90 when bit 0x40 of data_027e02a8 is set). If that value
 * is nonzero, allocates and constructs a node (model id from data_020ff1dc),
 * calls its slot 0x24 method twice with value-dependent ids, then gives it a
 * uniform scale (integer byte from data_020ff1e0, converted to fx32) and a
 * fixed fx32 position (-120, -100, -300). Changing either vector marks the node dirty.
 */
extern "C" void func_0207ce0c(Obj0207ce0c *self) {
    s32 level;
    Vec3 pos;
    Vec3 scale;
    const Vec3 *v;
    Cfg cfg;
    void *mem;
    Node0207ce0c *node;

    level = func_0208c72c(data_02104dc4, self->kind);
    if (data_027e02a8 & 0x40) {
        level = data_02106d90[0x8d1];
    }
    func_0206574c(self->sub, 0xc, -1);
    func_020661b4(self->sub, level);
    self->fa0 = func_02065be8(self->sub, self->fa0, 0x28, 0xa6, func_02066708(0x241), 1);
    if (level == 0) {
        return;
    }
    mem = (void *) func_0205e80c(NODE_ALLOC_SIZE, 3, 4);
    if (mem != NULL) {
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
        mem         = func_0200ff50(mem, 4, data_020ff1dc[self->kind].id, 0, &cfg);
    }
    self->node = (Node0207ce0c *) mem;
    if (level == 10) {
        self->node->unknown24(0xf4, 0, 1, 0);
        self->node->unknown24(0x89, 0, 1, 0);
    } else {
        self->node->unknown24(level + 0xe3, 0, 1, 0);
        self->node->unknown24(level + 0x78, 0, 1, 0);
    }
    node = self->node;
    {
        s32 s = data_020ff1e0[self->kind].size << FX32_SHIFT;
        vec_init(&scale, s, s, s);
    }
    v = &scale;
    node_set_scale(node, v);
    vec_init(&pos, -0x78000, -0x64000, -0x12c000);
    node = self->node;
    v    = &pos;
    node_set_pos(node, v);
}
