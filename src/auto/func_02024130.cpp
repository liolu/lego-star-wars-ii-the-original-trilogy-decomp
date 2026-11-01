#include "m2c.h"
class Entity {
public:
    virtual void unknown00();
    virtual void unknown04();
    virtual s32 unknown08();
    virtual void unknown0c();
    virtual void unknown10();
    virtual void unknown14();
    virtual void unknown18();
    virtual void unknown1c();
    virtual void unknown20();
    virtual void action(int, int, int, int);
};
typedef struct {
    s32 v;
} UnkVal;
typedef struct {
    u16 a : 1;
    u16 b : 1;
} Flags;
typedef struct {
    s32 v;
} Fx;
typedef struct {
    Fx x, y, z;
} Vec3;
typedef struct {
    s32 id;
    s8 index;
    s16 angle;
    u8 pad[20];
} Record;
extern "C" void func_02012744(void *, UnkVal);
extern "C" void func_02004b78(void *, s32);
extern "C" void func_02001990(void *, s32);
extern "C" void *func_0205e80c(s32, s32, s32);
extern "C" void *func_020363f4(void *, void *, s8);
extern "C" void func_0202c1a8(void *, void *);
extern "C" s16 func_020985d8(s32, s32);
extern "C" void func_020b04f0(void *, u8, void *, u8);
extern "C" s32 data_027e02a8;
extern "C" u8 data_0210794c[];
/* XZ-plane angle of a vector: func_020985d8(x, z) returns an s16 angle. */
static inline s16 angle2(Fx x, Fx z) {
    return func_020985d8(x.v, z.v);
}
static inline s16 angle(const Vec3 *v) {
    Fx z = v->z;
    return angle2(v->x, z);
}
/* *out = a - b (component-wise), then return the XZ angle of the difference. */
static inline s16 diff_angle(Vec3 *out, const Vec3 *a, const Vec3 *b) {
    Fx z, y, x;
    z.v    = a->z.v - b->z.v;
    y.v    = a->y.v - b->y.v;
    x.v    = a->x.v - b->x.v;
    out->x = x;
    out->y = y;
    out->z = z;
    return angle(out);
}
/*
 * Four-step state machine on the u8 at 0x1bc, then forwards to func_02012744.
 * 0: when 0x21c != 0x220 and 0x21c == 5, run vcall action(0x56), set byte 0x5d6
 *    of the linked object (0x218) and point its 0x6ac back at self, call
 *    func_02004b78(linked, 0x2c), go to 1; always copy 0x220 into 0x21c.
 *    When equal, clear both.
 * 1-3: wait for bit 1 of the halfword at 0x14e, then action(0x57/0x58/0) and
 *    advance (1->2->3->0). Step 2 also calls func_02001990(linked, 1) and either
 *    attaches a new func_0205e80c/func_020363f4 object at linked+0x5e0 or, when
 *    bit 0x40 of data_027e02a8 is set, sends record 0x11 (linked vcall 0x08 result,
 *    the XZ angle of self->0xb0 minus linked->0xb0, s8 at 0x225) to func_020b04f0.
 *    Step 3 clears byte 0x5d6 of the linked object.
 */
extern "C" void func_02024130(Entity *self, UnkVal arg1, ...) {
    switch (M2C_FIELD(self, u8 *, 0x1bc)) {
        case 0:
            if (M2C_FIELD(self, s32 *, 0x21c) != M2C_FIELD(self, s32 *, 0x220)) {
                if (M2C_FIELD(self, s32 *, 0x21c) == 5) {
                    self->action(0x56, 0, 1, 0);
                    M2C_FIELD(M2C_FIELD(self, Entity **, 0x218), u8 *, 0x5d6)      = 1;
                    M2C_FIELD(M2C_FIELD(self, Entity **, 0x218), Entity **, 0x6ac) = self;
                    func_02004b78(M2C_FIELD(self, Entity **, 0x218), 0x2c);
                    M2C_FIELD(self, u8 *, 0x1bc) = 1;
                }
                M2C_FIELD(self, s32 *, 0x21c) = M2C_FIELD(self, s32 *, 0x220);
            } else {
                M2C_FIELD(self, s32 *, 0x220) = 0;
                M2C_FIELD(self, s32 *, 0x21c) = 0;
            }
            break;
        case 1:
            if (((Flags *) ((u8 *) self + 0x14e))->b == 1) {
                M2C_FIELD(self, u8 *, 0x1bc) = 2;
                self->action(0x57, 0, 1, 0);
            }
            break;
        case 2:
            if (((Flags *) ((u8 *) self + 0x14e))->b == 1) {
                M2C_FIELD(self, u8 *, 0x1bc) = 3;
                self->action(0x58, 0, 1, 0);
                func_02001990(M2C_FIELD(self, Entity **, 0x218), 1);
                if (!(data_027e02a8 & 0x40)) {
                    void *p = func_0205e80c(0x10, 1, 4);
                    if (p) {
                        p = func_020363f4(p, M2C_FIELD(self, Entity **, 0x218), M2C_FIELD(self, s8 *, 0x225));
                    }
                    func_0202c1a8((u8 *) M2C_FIELD(self, Entity **, 0x218) + 0x5e0, p);
                } else {
                    Record r;
                    Vec3 d;
                    r.id    = M2C_FIELD(self, Entity **, 0x218)->unknown08();
                    r.angle = diff_angle(&d, (Vec3 *) ((u8 *) self + 0xb0),
                                         (Vec3 *) ((u8 *) M2C_FIELD(self, Entity **, 0x218) + 0xb0));
                    r.index = M2C_FIELD(self, s8 *, 0x225);
                    func_020b04f0(data_0210794c, 0x11, &r, 0xff);
                }
            }
            break;
        case 3:
            if (((Flags *) ((u8 *) self + 0x14e))->b == 1) {
                M2C_FIELD(self, u8 *, 0x1bc) = 0;
                self->action(0, 0, 1, 0);
                M2C_FIELD(M2C_FIELD(self, Entity **, 0x218), u8 *, 0x5d6) = 0;
            }
            break;
    }
    func_02012744(self, arg1);
}
