#include "m2c.h"

typedef volatile u16 vu16;
typedef volatile u32 vu32;

struct Vec3 {
    s32 x;
    s32 y;
    s32 z;
};

struct H {
    u16 v;
};

// Partial interface: unknown virtuals retain their observed vtable slots.
// Declarations only: the original vtable remains in the ROM data.
class Entity {
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
    virtual void action(int, int, int, int); // Slot 0x24.
    u8 pad04[0x3c];
    u8 f40;
    u8 pad41[0x1f];
    Vec3 vel; // 0x60
    u8 pad6c[0x44];
    Vec3 pos; // 0xb0
    u8 padbc[0xe];
    H angle; // 0xca
    u8 padcc[0x82];
    u16 b0 : 1; // 0x14e
    u16 b1 : 1;
    u16 rest : 14;
};

extern "C" void func_0202915c(Entity *a, Entity *b);
extern "C" void func_02029468(Entity *a);
extern "C" s32 FX_Div(s32 a, s32 b);
extern "C" void func_02028fb0(Vec3 *out, Vec3 *in, H h);

extern "C" void func_020281dc(Entity *arg0, Entity *arg1) {
    Vec3 out;
    Vec3 vec;
    s64 t;
    s32 sq;
    s32 q;

    if (arg0 == NULL || arg1 == NULL) {
        return;
    }

    func_0202915c(arg0, arg1);

    if (arg1->f40 != 0) {
        arg1->action(43, 0, 1, 0);
        func_02029468(arg1);
        return;
    }

    s32 dx = arg0->pos.x - arg1->pos.x;
    s32 dy = arg0->pos.y - arg1->pos.y;
    s32 dz = arg0->pos.z - arg1->pos.z;
    t      = (s64) dz * dz;
    t += (s64) dx * dx + (s64) dy * dy;

    *(vu16 *) 0x040002b0 = 1;
    *(u64 *) 0x040002b8  = t << 2;
    while (*(vu16 *) 0x040002b0 & 0x8000) {
    }
    sq = (s32) * (vu32 *) 0x040002b4;

    q = FX_Div(0x1000, ((sq + 1) >> 1) + 0x1000);

    vec.x = 0;
    vec.y = (s32) (((s64) q * 0x1800 + 0x800) >> 12);
    vec.z = (s32) ((((s64) q << 12) + 0x800) >> 12);

    func_02028fb0(&out, &vec, arg0->angle);

    arg1->vel.x += out.x;
    arg1->vel.y += out.y;
    arg1->vel.z += out.z;

    if (arg1->b1 == 1) {
        arg1->action(42, 0, 1, 0);
    }
}
