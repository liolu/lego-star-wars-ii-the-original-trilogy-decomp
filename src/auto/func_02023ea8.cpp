#include "m2c.h"

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
};

typedef struct {
    s32 v;
} UnkVal;

typedef struct {
    u16 flag_a : 1;
    u16 flag_b : 1;
} UnkFlags;

/* 16-bit angle held in an object; copies of it are kept as stack temps. */
class Angle {
public:
    u16 v;
    Angle() {}
    Angle(u16 x) :
        v(x) {}
    Angle operator+(Angle o) const {
        return Angle(v + o.v);
    }
    bool operator!=(Angle o) const {
        return v != o.v;
    }
};

/* The two per-frame angle increments added to each item. */
struct Rot {
    Angle x;
    Angle y;
};

enum {
    ITEM_COUNT = 6
};    /* Item pointers at self+0x218. */
enum {
    ANGLE_STEP = 0xb6
}; /* 182 = 0x10000 / 360. */

extern "C" void func_02012744(void *, UnkVal);

/* Item i of the pointer array at self+0x218. */
static inline Entity *ITEM(Entity *o, int i) {
    return ((Entity **) ((u8 *) o + 0x218))[i];
}

/* Store the angle at +0xca; set the byte at +0xd0 to 1 only if the value changed. */
static inline void setAngle(Entity *it, Angle a) {
    if (*(Angle *) ((u8 *) it + 0xca) != a) {
        M2C_FIELD(it, u16 *, 0xca) = a.v;
        M2C_FIELD(it, u8 *, 0xd0)  = 1;
    }
}

/* Copy an angle to +0x34 of the object pointed to by +0x20. */
static inline void syncChild(Entity *it, Angle a) {
    M2C_FIELD(M2C_FIELD(it, u8 **, 0x20), u16 *, 0x34) = a.v;
}

/*
 * State machine on the byte at self+0x1bc.
 * 1: add two ANGLE_STEP increments to each item's angle at +0xca, copy it to the
 *    item's child object, and count items whose byte at +0x1c0 is non-zero. Once
 *    all ITEM_COUNT items are counted, set each item's +0x1c1 to -1, call
 *    virtual slot 0x24 with 0x33 and then 0x78, and go to state 2.
 * 2: when bit 1 of item 0's halfword at +0x14e is set, call slot 0x24 on each
 *    item with 0x35 and then 0x7a, and go to state 3.
 * Every state then calls func_02012744(self, a1).
 */
extern "C" void func_02023ea8(Entity *self, UnkVal a1, ...) {
    int i;
    int count;

    switch (M2C_FIELD(self, u8 *, 0x1bc)) {
        case 0:
            break;
        case 1: {
            Rot step;
            step.x.v = ANGLE_STEP;
            step.y.v = ANGLE_STEP;
            count    = 0;
            for (i = 0; i < ITEM_COUNT; i++) {
                Angle cur = *(Angle *) ((u8 *) ITEM(self, i) + 0xca);
                setAngle(ITEM(self, i), cur + Angle(step.y.v) + Angle(step.x.v));
                syncChild(ITEM(self, i), *(Angle *) ((u8 *) ITEM(self, i) + 0xca));
                if (M2C_FIELD(ITEM(self, i), u8 *, 0x1c0) != 0) {
                    count++;
                }
            }
            if (count >= ITEM_COUNT) {
                for (i = 0; i < ITEM_COUNT; i++) {
                    M2C_FIELD(ITEM(self, i), s8 *, 0x1c1) = -1;
                    ITEM(self, i)->action(0x33, 0, 1, 0);
                    ITEM(self, i)->action(0x78, 0, 1, 0);
                }
                M2C_FIELD(self, u8 *, 0x1bc) = 2;
            }
            break;
        }
        case 2:
            if (((UnkFlags *) ((u8 *) ITEM(self, 0) + 0x14e))->flag_b == 1) {
                for (i = 0; i < ITEM_COUNT; i++) {
                    ITEM(self, i)->action(0x35, 0, 1, 0);
                    ITEM(self, i)->action(0x7a, 0, 1, 0);
                }
                M2C_FIELD(self, u8 *, 0x1bc) = 3;
            }
            break;
        case 3:
            break;
    }
    func_02012744(self, a1);
}
