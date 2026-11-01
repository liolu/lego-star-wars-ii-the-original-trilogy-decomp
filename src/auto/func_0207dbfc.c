/* Reconstructed from USA ARM9 at 0x0207dbfc (516 bytes).
 *
 * Picks one of two choices (self->sel = 0 or 1) for the object and, when a
 * choice is confirmed, refreshes it with func_0207de1c and moves the object
 * to state 7 (field 0x27, the state the caller func_0207d684 switches on).
 *
 * Steps:
 *  1. Clear field 0x26.
 *  2. Unless bit 0x40 of data_027e02a8 is set, and if bit 4 of the first byte
 *     of data_021044a4 is set, test the 16-bit point at 0x2a / 0x2c of
 *     data_021044a4 against the two rectangles of data_020ff16c (x, y, w, h;
 *     inside = [x, x + w) by [y, y + h)).  No hit: return.  A hit on choice 1
 *     always selects it; a hit on choice 0 selects it only while
 *     func_02086204(&data_02104d90, QUERY_ARG) is 0 and self->count < 15.
 *     A selection then commits (step 4, with the refresh call).
 *  3. Otherwise, while func_02086204(...) is 0 and self->count < 15, mask
 *     0x40 of func_0205d0f8(&data_021027f8, mask) moves choice 1 to 0 and
 *     mask 0x80 moves choice 0 to 1, each with a refresh call.  Mask 1 not
 *     set: return.
 *  4. Commit: a nonzero func_02086204(...) forces choice 1; set state 7 and
 *     field 0x26 to 0x10.
 */
#include "m2c.h"

typedef struct {
    u16 x;
    u16 y;
    u16 w;
    u16 h;
} Rect;

typedef struct {
    u32 count;
    u8 pad[0x21];
    s8 sel;
    u8 f26;
    u8 state;
} Obj;

typedef struct {
    u8 flags;
    u8 pad[0x29];
    s16 x;
    s16 y;
} Touch;

s32 func_0205d0f8(void *, s32);
s32 func_02086204(void *, s32);
void func_0207de1c(void *, s32);

extern s32 data_027e02a8;
extern Touch data_021044a4;
extern Rect data_020ff16c[];
extern u8 data_02104d90;
extern u8 data_021027f8;

enum {
    CHOICE_COUNT     = 2, /* also "no rectangle hit" */
    SKIP_POINT_FLAG  = 0x40,
    COUNT_LIMIT      = 0xF,
    QUERY_ARG        = 10,
    MASK_TO_CHOICE_0 = 0x40,
    MASK_TO_CHOICE_1 = 0x80,
    MASK_CONFIRM     = 1,
    STATE_CONFIRMED  = 7,
    F26_CONFIRMED    = 0x10,
};

void func_0207dbfc(Obj *self) {
    s32 chosen;
    s32 i;
    s32 hit_choice;
    u16 y;

    chosen    = 0;
    self->f26 = 0;
    if (!(data_027e02a8 & SKIP_POINT_FLAG) && ((u32) (data_021044a4.flags << 0x1B) >> 0x1F) != 0) {
        hit_choice = CHOICE_COUNT;
        y          = (u16) (s16) (s32) data_021044a4.y;
        for (i = 0; i < CHOICE_COUNT; i++) {
            u16 rh = data_020ff16c[i].h;
            u16 rw = data_020ff16c[i].w;
            u16 ry = data_020ff16c[i].y;
            u16 rx = data_020ff16c[i].x;
            if ((u32) (u16) (s16) (s32) data_021044a4.x >= (u32) rx &&
                (s32) (u16) (s16) (s32) data_021044a4.x < (s32) ((u32) rx + (u32) rw) && (u32) y >= (u32) ry &&
                (s32) y < (s32) ((u32) ry + (u32) rh)) {
                hit_choice = i;
                i          = CHOICE_COUNT; /* leave the search loop */
            }
        }
        if (hit_choice == CHOICE_COUNT) {
            return;
        }
        if (hit_choice != 0) {
            if (hit_choice == 1) {
                self->sel = hit_choice;
                chosen    = 1;
            }
        } else if (func_02086204(&data_02104d90, QUERY_ARG) == 0 && self->count < COUNT_LIMIT) {
            self->sel = hit_choice;
            chosen    = 1;
        }
        if (chosen != 0) {
            if (func_02086204(&data_02104d90, QUERY_ARG) != 0) {
                self->sel = 1;
            }
            func_0207de1c(self, (u8) self->sel);
            self->state = STATE_CONFIRMED;
            self->f26   = F26_CONFIRMED;
            return;
        }
    }
    if (func_02086204(&data_02104d90, QUERY_ARG) == 0 && self->count < COUNT_LIMIT) {
        if (func_0205d0f8(&data_021027f8, MASK_TO_CHOICE_0) != 0 && self->sel == 1) {
            self->sel = 0;
            func_0207de1c(self, 0);
        }
        if (func_0205d0f8(&data_021027f8, MASK_TO_CHOICE_1) != 0 && self->sel == 0) {
            self->sel = 1;
            func_0207de1c(self, 1);
        }
    }
    if (func_0205d0f8(&data_021027f8, MASK_CONFIRM) == 0) {
        return;
    }
    if (func_02086204(&data_02104d90, QUERY_ARG) != 0) {
        self->sel = 1;
    }
    self->state = STATE_CONFIRMED;
    self->f26   = F26_CONFIRMED;
}
