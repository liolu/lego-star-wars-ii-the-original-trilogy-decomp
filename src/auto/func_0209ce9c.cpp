#include "m2c.h"

typedef volatile u16 vu16;
typedef volatile u32 vu32;

// DS hardware square-root unit (ARM9 I/O registers).
enum {
    REG_SQRTCNT       = 0x040002B0, // Control: mode select, bit 15 = busy.
    REG_SQRT_RESULT   = 0x040002B4, // 32-bit result.
    REG_SQRT_PARAM    = 0x040002B8, // 64-bit input.
    SQRTCNT_64BIT     = 1,
    SQRTCNT_BUSY_MASK = 0x8000
};

struct Vec3 {
    s32 x;
    s32 y;
    s32 z;

    // Fieldwise copy guarded against self-assignment (same pattern as the
    // position copy in func_0209cd18).
    Vec3 &operator=(const Vec3 &v) {
        if (this != &v) {
            x = v.x;
            y = v.y;
            z = v.z;
        }
        return *this;
    }
};

// Pointed-to by data_02109358[i]; only the position at +0xB0 is read here.
struct Target {
    u8 pad[0xB0];
    Vec3 pos;
};

// The ctor func_0209cc5c installs vtable data_020ffb04, whose slot 0x00 is
// func_0209ce58: that method calls this function for every slot 0..3 whose
// data_02109358 entry is set.
class Tracker {
public:
    virtual void unknown00(); // Slot 0x00.
    u8 pad[0x20];
    s32 dist[4]; // +0x24: running total, advanced by the length of each move.
    Vec3 pos[4]; // +0x34: position seen on the previous update.
};

extern "C" Target *data_02109358[4];

static inline void vec_sub(Vec3 &out, const Vec3 &a, const Vec3 &b) {
    out.z = a.z - b.z;
    out.y = a.y - b.y;
    out.x = a.x - b.x;
}

// Adds the distance slot idx's target moved since the last call to
// dist[idx], then stores its current position as the new previous one.
extern "C" void func_0209ce9c(Tracker *t, u8 idx) {
    Vec3 d;
    vec_sub(d, data_02109358[idx]->pos, t->pos[idx]);
    t->pos[idx] = data_02109358[idx]->pos;

    // Length of d: sqrt(4 * |d|^2) is twice the length, so (root + 1) >> 1
    // gives the length rounded to nearest.
    s64 d2 = (s64) d.x * d.x;
    d2 += (s64) d.y * d.y;
    d2 += (s64) d.z * d.z;
    d2 <<= 2;
    *(vu16 *) REG_SQRTCNT   = SQRTCNT_64BIT;
    *(u64 *) REG_SQRT_PARAM = d2;
    while (*(vu16 *) REG_SQRTCNT & SQRTCNT_BUSY_MASK) {
    }
    M2C_FIELD((u8 *) t + idx * 4, s32 *, 0x24) += ((s32) (*(vu32 *) REG_SQRT_RESULT) + 1) >> 1;
}
