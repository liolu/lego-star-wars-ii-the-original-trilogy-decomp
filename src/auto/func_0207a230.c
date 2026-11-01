/* Step list positions forward by one, wrapping past the last index to 0.
 *
 * The object keeps seven signed byte positions for each of four modes
 * (0x130, 0x137, 0x13e, 0x145).  o->mode (0xd0) picks which set is moved.
 * Modes 0-2 move all seven positions; mode 3 moves only the position chosen
 * by o->sel (0xd2).  When a position passes the mode's last index, read from
 * the u16 table at data_020ee2a0 + 0xe, it wraps to 0.  In mode 1 a position
 * also keeps moving forward while the 20-byte record it points at in
 * data_020eedac has kind 6 (the "skip this entry" kind, see func_02076cb0).
 * func_0207a3e0 is the backward-step twin (wraps below 0 to the last index).
 */
#include "m2c.h"

enum {
    NUM_POSITIONS           = 7,
    LAST_INDEX_TABLE_OFFSET = 0xe,
    RECORD_SIZE             = 0x14,
    RECORD_KIND_SKIP        = 6,
};

typedef struct Obj {
    u8 pad0[0xd0];
    u8 mode;
    u8 pad1;
    s8 sel;
    u8 pad2[0x130 - 0xd3];
    s8 pos_mode0[NUM_POSITIONS];
    s8 pos_mode1[NUM_POSITIONS];
    s8 pos_mode2[NUM_POSITIONS];
    s8 pos_mode3[NUM_POSITIONS];
} Obj;

extern const u16 data_020ee2a0[];
extern u8 data_020eedac[];

/* Last valid position index for a mode. */
static inline u16 last_index(u8 mode) {
    return ((const u16 *) ((const u8 *) data_020ee2a0 + LAST_INDEX_TABLE_OFFSET))[mode];
}

void func_0207a230(Obj *o) {
    s32 i;
    s32 j;

    switch (o->mode) {
        case 0:
            for (i = 0; i < NUM_POSITIONS; i++) {
                o->pos_mode0[i] = o->pos_mode0[i] + 1;
                if (o->pos_mode0[i] > last_index(o->mode)) {
                    o->pos_mode0[i] = 0;
                }
            }
            break;
        case 1:
            for (j = 0; j < NUM_POSITIONS; j++) {
                o->pos_mode1[j] = o->pos_mode1[j] + 1;
                if (o->pos_mode1[j] > last_index(o->mode)) {
                    o->pos_mode1[j] = 0;
                }
                while (data_020eedac[o->pos_mode1[j] * RECORD_SIZE] == RECORD_KIND_SKIP) {
                    o->pos_mode1[j]++;
                    if (o->pos_mode1[j] > last_index(o->mode)) {
                        o->pos_mode1[j] = 0;
                    }
                }
            }
            break;
        case 2:
            for (i = 0; i < NUM_POSITIONS; i++) {
                o->pos_mode2[i] = o->pos_mode2[i] + 1;
                if (o->pos_mode2[i] > last_index(o->mode)) {
                    o->pos_mode2[i] = 0;
                }
            }
            break;
        case 3:
            o->pos_mode3[o->sel] = o->pos_mode3[o->sel] + 1;
            if (o->pos_mode3[o->sel] > last_index(o->mode)) {
                o->pos_mode3[o->sel] = 0;
            }
            break;
    }
}
