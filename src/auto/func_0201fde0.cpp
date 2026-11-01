#include "m2c.h"

// Partial interface: unknown virtuals keep their observed vtable slots (declarations only).
class Entity1FDE0 {
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
};

typedef struct {
    s32 v;
} UnkVal;

typedef struct {
    s32 v;
} WordArg;

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct {
    u16 flag_a : 1;
    u16 flag_b : 1;
} UnkFlags;

extern "C" void *func_02017800(Entity1FDE0 *, int);
extern "C" void func_02012744(Entity1FDE0 *, UnkVal);
extern "C" s32 func_0200aef8(void *, void *, WordArg);
extern "C" void func_02007eb4(s32 *out, void *a, void *b);
extern "C" u8 data_021027f8[];
extern "C" u8 data_0210930c[];

static inline int sub_flag_b(void *sub) {
    return ((UnkFlags *) ((u8 *) sub + 0x7a))->flag_b;
}

static inline WordArg make_word(s32 v) {
    WordArg r;
    r.v = v;
    return r;
}

/* Radius for func_0200aef8 (3D squared-distance-within-radius test) and the
 * threshold for the x/z distance returned by func_02007eb4. */
enum {
    NEAR_RADIUS = 0x19000
};
enum {
    FAR_DISTANCE = 0x32000
};

/* State machine on the byte at 0x1bc. States 1 and 4 wait until one of the
 * data_021027f8[0x29] objects listed at data_0210930c + 0x4c comes within
 * NEAR_RADIUS of this object's position (0xb0), fire two slot-0x24 actions and
 * advance to 2 / 5. States 2 and 5 (only while flag_b at 0x14e is set) wait
 * until a listed object is farther than FAR_DISTANCE on x/z, fire actions and
 * return to 1 (state 5 also stores 0x190 at 0x218). States 3 and 6 wait for
 * flag_b of the func_02017800(self, 0) object; 3 runs action 0x58 on the
 * object at 0x21c and moves to 4, 6 moves to 1. Always ends in func_02012744. */
extern "C" void func_0201fde0(Entity1FDE0 *self, UnkVal arg1, ...) {
    u8 i;

    switch (M2C_FIELD(self, u8 *, 0x1bc)) {
        case 0:
            break;
        case 1:
            for (i = 0; i < data_021027f8[0x29]; i++) {
                u8 *obj = *(u8 **) (data_0210930c + i * 4 + 0x4c);
                if (obj != 0) {
                    Vec3 pos;
                    pos.x = M2C_FIELD(obj, s32 *, 0xb0);
                    pos.y = M2C_FIELD(obj, s32 *, 0xb4);
                    pos.z = M2C_FIELD(obj, s32 *, 0xb8);
                    if (func_0200aef8((u8 *) self + 0xb0, &pos, make_word(NEAR_RADIUS)) != 0) {
                        self->action(0x57, 0, 1, 0);
                        self->action(0x79, 0, 1, 0);
                        M2C_FIELD(self, u8 *, 0x1bc) = 2;
                        break;
                    }
                }
            }
            break;
        case 2:
            if (((UnkFlags *) ((u8 *) self + 0x14e))->flag_b == 1) {
                for (i = 0; i < data_021027f8[0x29]; i++) {
                    u8 *obj = *(u8 **) (data_0210930c + i * 4 + 0x4c);
                    if (obj != 0) {
                        Vec3 pos;
                        s32 dist;
                        pos.x = M2C_FIELD(obj, s32 *, 0xb0);
                        pos.y = M2C_FIELD(obj, s32 *, 0xb4);
                        pos.z = M2C_FIELD(obj, s32 *, 0xb8);
                        func_02007eb4(&dist, (u8 *) self + 0xb0, &pos);
                        if (dist > FAR_DISTANCE) {
                            self->action(0, 0, 1, 0);
                            self->action(0x8a, 0, 1, 0);
                            self->action(0x78, 0, 1, 0);
                            M2C_FIELD(self, u8 *, 0x1bc) = 1;
                            break;
                        }
                    }
                }
            }
            break;
        case 3:
            if (sub_flag_b((u8 *) func_02017800(self, 0) + 0xd4) == 1) {
                Entity1FDE0 *other = M2C_FIELD(self, Entity1FDE0 **, 0x21c);
                other->action(0x58, 0, 1, 0);
                M2C_FIELD(self, u8 *, 0x1bc) = 4;
            }
            break;
        case 4:
            for (i = 0; i < data_021027f8[0x29]; i++) {
                u8 *obj = *(u8 **) (data_0210930c + i * 4 + 0x4c);
                if (obj != 0) {
                    Vec3 pos;
                    pos.x = M2C_FIELD(obj, s32 *, 0xb0);
                    pos.y = M2C_FIELD(obj, s32 *, 0xb4);
                    pos.z = M2C_FIELD(obj, s32 *, 0xb8);
                    if (func_0200aef8((u8 *) self + 0xb0, &pos, make_word(NEAR_RADIUS)) != 0) {
                        self->action(0x5b, 0, 1, 0);
                        self->action(0x7c, 0, 1, 0);
                        M2C_FIELD(self, u8 *, 0x1bc) = 5;
                        break;
                    }
                }
            }
            break;
        case 5:
            if (((UnkFlags *) ((u8 *) self + 0x14e))->flag_b == 1) {
                for (i = 0; i < data_021027f8[0x29]; i++) {
                    u8 *obj = *(u8 **) (data_0210930c + i * 4 + 0x4c);
                    if (obj != 0) {
                        Vec3 pos;
                        s32 dist;
                        pos.x = M2C_FIELD(obj, s32 *, 0xb0);
                        pos.y = M2C_FIELD(obj, s32 *, 0xb4);
                        pos.z = M2C_FIELD(obj, s32 *, 0xb8);
                        func_02007eb4(&dist, (u8 *) self + 0xb0, &pos);
                        if (dist > FAR_DISTANCE) {
                            self->action(0, 0, 1, 0);
                            M2C_FIELD(self, u8 *, 0x1bc)  = 1;
                            M2C_FIELD(self, s32 *, 0x218) = 0x190;
                            break;
                        }
                    }
                }
            }
            break;
        case 6:
            if (sub_flag_b((u8 *) func_02017800(self, 0) + 0xd4) == 1) {
                M2C_FIELD(self, u8 *, 0x1bc) = 1;
            }
            break;
    }
    func_02012744(self, arg1);
}
