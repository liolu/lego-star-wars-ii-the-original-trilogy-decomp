/*
 * func_020780a4: input step for a four-entry selection held in self->sel.
 *
 * When bit 4 of the first byte of data_021044a4 is set, the 16-bit point at
 * 0x2a/0x2c (x, y) is tested against the five read-only rectangles of
 * data_020ee334 ({x, y, w, h}, hit when x in [x, x+w) and y in [y, y+h));
 * the first hit stops the search.  Rectangles 0-3 select that entry
 * (func_0207759c(self) refreshes, then func_020b8f70(0xB4, 0)); rectangle 4
 * runs func_02087044 / func_0205ecc4 / func_0201b318(self->fdc, 1) and then
 * func_020b8f70(0xB5, 0).  Otherwise func_0205d0d0(&data_021027f8, mask, 4)
 * with mask 0x200 steps sel down and with 0x100 steps it up, wrapping in
 * 0..3.  Returns 1 when something was handled, else 0.  The sibling
 * func_020782a8 uses the same hit-test form.
 */
#include "m2c.h"

enum {
    RECT_COUNT    = 5,
    SEL_COUNT     = 4,
    MASK_SEL_PREV = 0x200,
    MASK_SEL_NEXT = 0x100,
};

typedef struct {
    u8 b0 : 1;
    u8 b1 : 1;
    u8 b2 : 1;
    u8 b3 : 1;
    u8 b4 : 1;
    u8 b5 : 1;
    u8 b6 : 1;
    u8 b7 : 1;
    u8 pad[0x29];
    s16 f2a;
    s16 f2c;
} Input;

typedef struct {
    u16 x;
    u16 y;
    u16 w;
    u16 h;
} Rect;

typedef struct {
    u8 pad00[0xD0];
    u8 sel;
    u8 padd1[0xB];
    void *fdc;
} Obj;

extern Input data_021044a4;
extern const Rect data_020ee334[];
extern u8 data_021027f8;
extern u8 data_02102900;
extern u8 data_02104d90;

void func_0201b318(void *, u8);
s32 func_0205d0d0(void *, s32, s32);
void func_0205ecc4(M2C_UNK, M2C_UNK, M2C_UNK, M2C_UNK, s32, s32);
void func_0207759c(M2C_UNK);
void func_02087044(void *);
void func_020b8f70(M2C_UNK, s32);

s32 func_020780a4(Obj *self) {
    s32 idx;
    s32 i;
    s32 handled;
    u16 point_y;

    if (data_021044a4.b4) {
        idx     = RECT_COUNT;
        handled = 0;
        point_y = (u16) (s16) (s32) data_021044a4.f2c;
        for (i = 0; i < RECT_COUNT; i++) {
            u16 rh = data_020ee334[i].h;
            u16 rw = data_020ee334[i].w;
            u16 ry = data_020ee334[i].y;
            u16 rx = data_020ee334[i].x;
            if ((u16) (s16) (s32) data_021044a4.f2a >= rx && (u16) (s16) (s32) data_021044a4.f2a < rx + rw && point_y >= ry &&
                point_y < ry + rh) {
                idx = i;
                i   = RECT_COUNT; /* leave the search loop */
            }
        }
        if (idx == RECT_COUNT) {
            return 0;
        }
        switch (idx) {
            case 0:
            case 1:
            case 2:
            case 3:
                self->sel = idx;
                func_0207759c(self);
                handled = 1;
                func_020b8f70(0xB4, 0);
                break;
            case 4:
                func_02087044(&data_02104d90);
                func_0205ecc4(&data_02102900, 0x30, 1, 0, 1, 0);
                func_0201b318(self->fdc, 1);
                handled = 1;
                func_020b8f70(0xB5, 0);
                break;
        }
        if (handled) {
            return 1;
        }
    }
    if (func_0205d0d0(&data_021027f8, MASK_SEL_PREV, 4)) {
        self->sel--;
        if (self->sel >= SEL_COUNT) {
            self->sel = SEL_COUNT - 1;
        }
        func_0207759c(self);
        func_020b8f70(0xB4, 0);
        return 1;
    }
    if (func_0205d0d0(&data_021027f8, MASK_SEL_NEXT, 4)) {
        self->sel++;
        if (self->sel >= SEL_COUNT) {
            self->sel = 0;
        }
        func_0207759c(self);
        func_020b8f70(0xB4, 0);
        return 1;
    }
    return 0;
}
