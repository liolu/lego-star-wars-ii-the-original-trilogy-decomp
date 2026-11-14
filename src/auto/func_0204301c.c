#include "m2c.h"

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct {
    s32 z;
} ZVec;

typedef struct {
    u16 v;
} Ang;

typedef struct {
    s32 a;
    s32 b;
} Res;

enum {
    NZ_INIT = -0x8000
};
enum {
    RES_B_MIN = 0x2000
};

extern M2C_UNK data_0210930c;
extern void func_02042cdc(Vec3 *dst, Vec3 *src, ZVec v, ...);
extern void func_02028fb0(Vec3 *out, Vec3 *in, Ang ang);
extern void func_0200d6d0(Vec3 *dst, Vec3 *src, ZVec v);
extern void func_01ff97c4(Res *out, void *obj, Vec3 *a, Vec3 *b, s32 flag);

static inline void vec_add(Vec3 *o, const Vec3 *a, const Vec3 *b) {
    s32 z = a->z + b->z;
    s32 y = a->y + b->y;
    s32 x = a->x + b->x;
    o->x  = x;
    o->y  = y;
    o->z  = z;
}

/* Reads field +4 of the 8-byte result that func_01ff97c4 writes through its first argument. */
static inline s32 res_dist(const Res *r) {
    const Res *q = r; // FAKE: redundant pointer copy; with the null test below it keeps &res in a register
    s32 v;            // FAKE: left uninitialised so the dead NULL path adds no code

    // FAKE: always-true null test (r is &res); forces the address form add r0,sp,#8; ldr r0,[r0,#4]
    // after the call instead of a direct ldr r0,[sp,#12]
    if (q) {
        v = q->b;
    }
    return v;
}

s32 func_0204301c(Vec3 *pos, Ang ang, ZVec z, ...) {
    Vec3 t3;
    Vec3 t2;
    Vec3 t1;
    Vec3 zero2;
    Vec3 t4;
    Vec3 zero1;
    Res res;
    ZVec nz;
    void *obj;
    void *ctx;

    nz.z    = NZ_INIT;
    zero1.x = 0;
    zero1.y = 0;
    zero1.z = 0;
    zero2.x = 0;
    zero2.y = 0;
    zero2.z = 0;
    func_02042cdc(&t1, &zero2, z);
    func_02028fb0(&t2, &t1, ang);
    ctx = M2C_FIELD(&data_0210930c, void **, 0x68);
    vec_add(&t3, pos, &t2);
    obj = M2C_FIELD(ctx, void **, 0x384);
    func_0200d6d0(&t4, &zero1, nz);
    func_01ff97c4(&res, obj, &t3, &t4, 0);
    return res_dist(&res) >= RES_B_MIN;
}
