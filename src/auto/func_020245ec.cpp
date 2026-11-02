#include "m2c.h"

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
};

/* By-value argument kept in the register save area (variadic tail). */
typedef struct {
    s32 v;
} UnkVal;

typedef struct {
    u32 p;
    u32 q;
} Slot;

typedef struct {
    u16 flag_a : 1;
    u16 flag_b : 1;
} UnkFlags;

extern "C" void func_02012744(void *obj, UnkVal arg1);
extern "C" void func_020249a0(Entity *obj);
extern "C" void func_02024b20(Entity *obj);
extern "C" void func_02024e10(Entity *obj, void *other, s32 arg2);
extern "C" s32 func_02062a7c(s32 arg0, s32 arg1);

/*
 * Per-frame update of a state machine kept in the byte at 0x1bc.
 * Forces state 6 when byte 0x214 is 0x19..0x1b, drops the object linked at
 * 0x238 (and its count at 0x236) once that object's flags hit mask 0x802,
 * then runs the current state; the 8-byte slots at 0x220 (count at 0x230)
 * decide between states 7 and 5. Always ends by forwarding to func_02012744.
 */
extern "C" void func_020245ec(Entity *obj, UnkVal arg1, ...) {
    if ((u32) (M2C_FIELD(obj, u8 *, 0x214) - 0x19) <= 2) {
        M2C_FIELD(obj, u8 *, 0x1bc) = 6;
    }
    if (M2C_FIELD(obj, u32 *, 0x238) != 0) {
        if (M2C_FIELD(M2C_FIELD(obj, u32 *, 0x238), u32 *, 8) & 0x802) {
            M2C_FIELD(obj, u32 *, 0x238) = 0;
            M2C_FIELD(obj, u8 *, 0x236)  = 0;
        }
    }
    switch (M2C_FIELD(obj, u8 *, 0x1bc)) {
        case 0:
            M2C_FIELD(obj, u8 *, 0x1bc) = 1;
            break;
        case 1:
        case 7:
            func_020249a0(obj);
            break;
        case 2:
            if (M2C_FIELD(obj, u16 *, 0x212) == 0x2b) {
                obj->action(0x56, 0, 1, 0);
                M2C_FIELD(obj, u8 *, 0x1bc)  = 4;
                M2C_FIELD(obj, s16 *, 0x234) = func_02062a7c(5, 0xf);
            } else {
                func_02024b20(obj);
                obj->action(0x56, 0, 1, 0);
                M2C_FIELD(obj, u8 *, 0x1bc)  = 4;
                M2C_FIELD(obj, s16 *, 0x234) = func_02062a7c(0x2d, 0x5a);
            }
            break;
        case 4:
            if (M2C_FIELD(obj, u16 *, 0x212) == 0x2b) {
                if (((UnkFlags *) ((u8 *) obj + 0x14e))->flag_b == 1) {
                    s16 t                        = M2C_FIELD(obj, s16 *, 0x234);
                    M2C_FIELD(obj, s16 *, 0x234) = t - 1;
                    if (t == 0) {
                        obj->action(0x56, 0, 1, 1);
                        M2C_FIELD(obj, u8 *, 0x1bc) = 5;
                    } else if (M2C_FIELD(obj, s16 *, 0x234) == 3) {
                        if (M2C_FIELD(obj, u8 *, 0x236) != 0) {
                            func_02024e10(obj, M2C_FIELD(obj, void **, 0x238), (M2C_FIELD(obj, u8 *, 0x236) & 1) != 0);
                            M2C_FIELD(obj, u8 *, 0x236) = M2C_FIELD(obj, u8 *, 0x236) - 1;
                            if ((M2C_FIELD(obj, u8 *, 0x236) & 0xff) == 0) {
                                M2C_FIELD(obj, u32 *, 0x238) = 0;
                            } else {
                                M2C_FIELD(obj, s16 *, 0x234) += 8;
                            }
                        } else {
                            func_02024b20(obj);
                        }
                    }
                }
            } else {
                if (((UnkFlags *) ((u8 *) obj + 0x14e))->flag_b == 1) {
                    s16 t                        = M2C_FIELD(obj, s16 *, 0x234);
                    M2C_FIELD(obj, s16 *, 0x234) = t - 1;
                    if (t == 0) {
                        u32 entry;
                        s32 all_set = 1; /* cleared when any slot holds no object */
                        s32 i;
                        s32 count = M2C_FIELD(obj, u8 *, 0x230);
                        for (i = 0; i < count; i++) {
                            entry = ((Slot *) ((u8 *) obj + 0x220))[i].p;
                            if (entry != 0) {
                                entry -= 4; /* non-null pointer moved back 4 bytes (cast pattern) */
                            }
                            if (entry == 0) {
                                all_set = 0;
                            }
                        }
                        if (all_set != 0) {
                            M2C_FIELD(obj, u8 *, 0x1bc) = 7;
                        } else {
                            obj->action(0x56, 0, 1, 1);
                            M2C_FIELD(obj, u8 *, 0x1bc) = 5;
                        }
                    }
                }
            }
            break;
        case 5:
            if (((UnkFlags *) ((u8 *) obj + 0x14e))->flag_b == 1) {
                obj->action(0, 1, 1, 0);
                M2C_FIELD(obj, u8 *, 0x1bc) = 1;
            }
            break;
    }
    func_02012744(obj, arg1);
}
