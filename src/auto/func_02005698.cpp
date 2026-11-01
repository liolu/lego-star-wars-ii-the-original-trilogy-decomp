#include "m2c.h"

typedef struct {
    s32 x, y, z;
} Vec3;

typedef struct {
    s32 m[18];
} Mtx;

typedef struct {
    u8 pad0[0x60];
    Vec3 pos;
    u8 pad6c[0x1bc - 0x6c];
    s32 unk1bc;
    u8 unk1c0;
} Obj;

// Declarations only: unknown virtuals keep their observed slots.
class Actor {
public:
    virtual void unknown00();
    virtual void unknown04();
    virtual s32 unknown08(); // Slot 0x8.
};

extern "C" void *func_0205e80c(s32 size, s32 a1, s32 a2);
extern "C" void func_020accac(void *out, void *pos, void *rot, void *scale);
extern "C" Obj *func_0200cb48(void *self, s32 a1, s32 a2, void *a3);
extern "C" void func_02007f24(s32 *out, f64 v);
extern "C" void func_0200d4c8(void *e, void *arg1);
extern "C" const s16 data_020f88c4[];

enum {
    SPAWN_SIZE = 0x1e0,
    SPAWN_KIND = 0x1f3,
    FX_ONE     = 0x1000,
};

static inline void vec_xyz(Vec3 *v, s32 x, s32 y, s32 z) {
    v->x = x;
    v->y = y;
    v->z = z;
}

// Creates an object (func_0205e80c alloc, SPAWN_SIZE bytes) and, if non-null,
// builds a transform from the source position (self+0x30 -> +4..+0xc), the
// actor's rotation (self+0xc8) and unit scale, and constructs it with
// func_0200cb48. Field 0x1bc comes from virtual slot 0x8 unless the actor's
// halfword at 0xc is 1 (then byte 0x5da, and 0x1c0 is set). The object's
// position is set to (0, 1.0, 1.4) in 20.12 fixed point (func_02007f24 =
// double * 4096), rotated about Y by angle self+0xca via the sin/cos table,
// then func_0200d4c8 is called with self+0xb0.
extern "C" Obj *func_02005698(Actor *self) {
    u8 *src = M2C_FIELD(self, u8 **, 0x30);
    Vec3 pos;
    Vec3 scale;
    Mtx mtx;
    s32 offset_y;
    s32 offset_z;
    Vec3 v;
    Obj *ent;
    s32 sin_a, cos_a, x, z;
    s32 nx, t;
    u16 ang;

    pos.x = M2C_FIELD(src, s32 *, 4);
    pos.y = M2C_FIELD(src, s32 *, 8);
    pos.z = M2C_FIELD(src, s32 *, 0xc);
    ent   = (Obj *) func_0205e80c(SPAWN_SIZE, 3, 4);
    if (ent != (Obj *) 0) {
        scale.x = FX_ONE;
        scale.y = FX_ONE;
        scale.z = FX_ONE;
        func_020accac(&mtx, &pos, (u8 *) self + 0xc8, &scale);
        ent = func_0200cb48(ent, SPAWN_KIND, 0, &mtx);
    }
    if (M2C_FIELD(self, u16 *, 0xc) != 1) {
        ent->unk1bc = self->unknown08();
    } else {
        ent->unk1c0 = 1;
        ent->unk1bc = M2C_FIELD(self, u8 *, 0x5da);
    }
    func_02007f24(&offset_z, 1.4);
    func_02007f24(&offset_y, 1.0);
    vec_xyz(&v, 0, offset_y, offset_z);
    {
        const Vec3 *sv = &v;
        if (&ent->pos != sv) {
            ent->pos.x = sv->x;
            ent->pos.y = sv->y;
            ent->pos.z = sv->z;
        }
    }
    ang   = M2C_FIELD(self, u16 *, 0xca);
    sin_a = data_020f88c4[(ang >> 4) * 2];
    cos_a = data_020f88c4[(ang >> 4) * 2 + 1];
    x     = ent->pos.x;
    z     = ent->pos.z;
    t     = (s32) (((s64) z * sin_a + 0x800) >> 12);
    nx    = (s32) (((s64) x * cos_a + 0x800) >> 12);
    nx += t;
    t          = (s32) (((s64) x * sin_a + 0x800) >> 12);
    t          = (s32) (((s64) z * cos_a + 0x800) >> 12) - t;
    ent->pos.x = nx;
    ent->pos.z = t;
    func_0200d4c8(ent, (u8 *) self + 0xb0);
    return ent;
}
