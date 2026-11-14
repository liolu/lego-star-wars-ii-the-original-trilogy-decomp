#include "m2c.h"

struct Vec3 {
    s32 x, y, z;
};

extern "C" void func_0200af88(s8 *flag, s8 value);
extern "C" s32 data_020ff9a8;
extern "C" u8 data_020fcd18[]; // vtable of the base shape
extern "C" u8 data_020fe454[]; // vtable of the sphere

// Partial interface: declarations only, the original vtables remain in the ROM data.
class Shape {
public:
    virtual Vec3 unknown00(void *other);
    virtual void unknown04(void *out);
    virtual s32 unknown08();
    virtual Vec3 unknown0c();
};

// Constructors in plain-name form: the vtables stay in the ROM data, so the vptr is set by hand.
struct ShapeData {
    void *vtbl;
    Vec3 pos;
    s32 f10, f14, f18, f1c, f20;
    s8 f24;
    s8 type;
    s8 f26;

    ShapeData(Vec3 p) {
        vtbl  = data_020fcd18;
        pos.x = p.x;
        pos.y = p.y;
        pos.z = p.z;
        f10   = 0;
        f14   = 0;
        f18   = 0;
        f1c   = 0;
        f20   = 0;
        func_0200af88(&f24, -1);
        type = -1;
        f26  = 0;
    }
};

struct SphereData : ShapeData {
    s32 radius;

    SphereData(Vec3 p) :
        ShapeData(p) {
        vtbl   = data_020fe454;
        type   = 1;
        radius = data_020ff9a8;
    }
};

struct Capsule {
    void *vtable;
    Vec3 pos;
    s32 pad[6];
    s32 halfHeight;
    s32 radius;
};

// Slot 0 of the vtable at data_020fe484. Builds a local sphere at the end of the axis picked by the sign of
// other[1] (pos.y +/- halfHeight) and forwards through its vtable data_020fe454, whose slot 0 is func_0204cf4c.
extern "C" Vec3 func_0204cdb0(Capsule *capsule, s32 *other) {
    if (other[1] > 0) {
        // FAKE: volatile local forces the target's never-read store of capsule->radius to [sp+4].
        volatile s32 r = capsule->radius;
        Vec3 center;
        s32 y = capsule->pos.y;
        y += capsule->halfHeight;
        center.x = capsule->pos.x;
        center.y = y;
        center.z = capsule->pos.z;
        SphereData sphere(center);
        Shape *shape = (Shape *) &sphere;
        return shape->unknown00(other);
    } else {
        // FAKE: volatile local forces the target's never-read store of capsule->radius to [sp].
        volatile s32 r = capsule->radius;
        Vec3 center;
        s32 y = capsule->pos.y;
        y -= capsule->halfHeight;
        center.x = capsule->pos.x;
        center.y = y;
        center.z = capsule->pos.z;
        SphereData sphere(center);
        Shape *shape = (Shape *) &sphere;
        return shape->unknown00(other);
    }
}
