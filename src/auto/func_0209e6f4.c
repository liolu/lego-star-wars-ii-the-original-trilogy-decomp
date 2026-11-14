/* Handler for a message of kind 3: stores the 32-bit payload into two globals
 * and the 16-bit payload into the object, resets the per-slot state, then
 * queues two requests on the same object. */
#include "m2c.h"

typedef struct {
    u8 pad00[8];
    u16 f08;
    u8 pad0a[0x23 - 0x0a];
    u8 f23;
    u8 pad24[0x40 - 0x24];
} Player;

typedef struct {
    u8 pad[0x2240];
    Player players[8];
} Game;

typedef struct {
    u8 pad[0xb8];
    u16 fb8;
} Self;

extern u32 data_02107654;
extern u32 data_02101ad4;
extern Game data_02104d90;
extern u8 data_02106d90[];
extern M2C_UNK data_02102900;

void func_02001688(s32);
void func_020016c0(void);
s32 func_02073034(void);
void func_0205ecc4(void *, M2C_UNK, M2C_UNK, M2C_UNK, s32, s32);

// FAKE: identity accessor for &data_02107654; exists so the address can be held in a local and null-tested below
static inline u32 *data_02107654_ptr(void) {
    return &data_02107654;
}

// arg1 is the payload kind (only 3 does anything); arg2 points at the payload bytes:
// u32 at +0, u16 at +4.
void func_0209e6f4(Self *arg0, s32 arg1, u8 *arg2) {
    u32 *dst;
    u32 v;
    u8 i;

    func_02001688(1);
    if (arg1 == 3) {
        dst = data_02107654_ptr();
        // FAKE: always-true null test; keeps &data_02107654 in its own register (no copy propagation into
        // the store), which gives the target register allocation
        if (dst) {
            v             = *(u32 *) arg2;
            *dst          = v;
            data_02101ad4 = v;
        }
        arg0->fb8 = *(u16 *) (arg2 + 4);
        i         = 0;
        if (func_02073034() > 0) {
            do {
                data_02104d90.players[i].f08 = 0x14;
                data_02104d90.players[i].f23 = 1;
                i++;
            } while (i < func_02073034());
        }
        data_02106d90[0xc28] = 1;
        func_0205ecc4(&data_02102900, 4, 0, 0xf, 4, 0);
        func_0205ecc4(&data_02102900, 0x2a, 1, 0, 0, 0);
    }
    func_020016c0();
}
