#include "m2c.h"
// Declaration-only partial interface; action is slot 0x24, unknown54 is slot 0x54.
class Entity1F318 {
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
    virtual void action(int, int, int, int);
    virtual void unknown28();
    virtual void unknown2c();
    virtual void unknown30();
    virtual void unknown34();
    virtual void unknown38();
    virtual void unknown3c();
    virtual void unknown40();
    virtual void unknown44();
    virtual void unknown48();
    virtual void unknown4c();
    virtual void unknown50();
    virtual void unknown54();
};
typedef struct {
    s32 v;
} UnkVal;
typedef struct {
    u16 flag_a : 1;
    u16 flag_b : 1;
} UnkFlags;
extern "C" s32 func_0200aef8(void *, void *, UnkVal);
extern "C" void func_02007eb4(s32 *, s32 *, s32 *);
extern "C" void func_02012744(Entity1F318 *, UnkVal);
extern "C" s32 data_021027f8;
typedef struct {
    u8 pad[0x4c];
    Entity1F318 *entities[1];
} EntityList;
extern "C" EntityList data_0210930c;
// Optional state object referenced at +0x148. Its first word is a 20.12
// fixed-point position: func_0200b10c and func_0201ed28 read it (or 0 when the
// pointer is null) and compare its integer part with a u16 count.
struct PositionState {
    s32 position;
};
// Address of the position word, or null when no state object is attached.
static inline s32 *positionOf(PositionState *state) {
    return state ? &state->position : 0;
}
enum {
    PROXIMITY_RADIUS          = 0x19000,
    HORIZONTAL_DISTANCE_LIMIT = 0x32000,
    SEPARATION_RESET_VALUE    = 0x190,
    PROXIMITY_ACTION          = 0x57,
    COUNTDOWN_ACTION          = 0x56,
};

/* Advance this callback's entity state machine. States 1 and 2 test the global
 * entity list for proximity and horizontal separation, respectively. State 4
 * waits for a signed countdown, then dispatches an action, rewinds the +0x148
 * position to zero, clears flag bit 0 at +0x14e and updates the linked object. The state/flag meanings and distance units are
 * not established. Every path forwards the saved by-value argument to the common update.
 */
extern "C" void func_0201f318(Entity1F318 *self, UnkVal forwardedArg, ...) {
    s32 horizontalDistance;
    UnkVal radius;
    switch (M2C_FIELD(self, u8 *, 0x1bc)) {
        case 0:
        case 6:
            break;
        case 1: {
            u32 entityIndex;
            for (entityIndex = 0; entityIndex < M2C_FIELD(&data_021027f8, u8 *, 0x29); entityIndex++) {
                Entity1F318 *other = data_0210930c.entities[entityIndex];
                if (other != 0) {
                    radius.v = PROXIMITY_RADIUS;
                    if (func_0200aef8((u8 *) self + 0xb0, (u8 *) other + 0xb0, radius) != 0) {
                        self->action(PROXIMITY_ACTION, 0, 1, 0);
                        M2C_FIELD(self, u8 *, 0x1bc) = 2;
                        break;
                    }
                }
            }
        } break;
        case 2:
            if (((UnkFlags *) ((u8 *) self + 0x14e))->flag_b == 1) {
                u32 entityIndex;
                for (entityIndex = 0; entityIndex < M2C_FIELD(&data_021027f8, u8 *, 0x29); entityIndex++) {
                    Entity1F318 *other = data_0210930c.entities[entityIndex];
                    if (other != 0) {
                        func_02007eb4(&horizontalDistance, (s32 *) ((u8 *) self + 0xb0), (s32 *) ((u8 *) other + 0xb0));
                        if (horizontalDistance > HORIZONTAL_DISTANCE_LIMIT) {
                            self->action(0, 0, 1, 0);
                            M2C_FIELD(self, u8 *, 0x1bc)  = 1;
                            M2C_FIELD(self, s32 *, 0x218) = SEPARATION_RESET_VALUE;
                            break;
                        }
                    }
                }
            }
            break;
        case 3:
            if (M2C_FIELD(self, Entity1F318 **, 0x21c) != 0) {
                M2C_FIELD(self, u8 *, 0x1bc)  = 6;
                M2C_FIELD(self, s32 *, 0x194) = 0;
            }
            break;
        case 4:
            M2C_FIELD(self, s16 *, 0x1be) = M2C_FIELD(self, s16 *, 0x1be) - 1;
            if (M2C_FIELD(self, s16 *, 0x1be) == 0) {
                self->action(COUNTDOWN_ACTION, 0, 1, 1);
                *positionOf(M2C_FIELD(self, PositionState **, 0x148)) = 0;
                M2C_FIELD(self, u16 *, 0x14e) &= ~1;
                Entity1F318 *other = M2C_FIELD(self, Entity1F318 **, 0x21c);
                if (other != 0) {
                    other->unknown54();
                }
                M2C_FIELD(self, u8 *, 0x1bc) = 5;
            }
            break;
        case 5: {
            Entity1F318 *other = M2C_FIELD(self, Entity1F318 **, 0x21c);
            if (other != 0 && ((UnkFlags *) ((u8 *) other + 0x14e))->flag_b == 1) {
                M2C_FIELD(self, u8 *, 0x1bc) = 0;
            }
        } break;
    }
    func_02012744(self, forwardedArg);
}
