#include "m2c.h"

typedef struct {
    u16 v;
} HalfWord;

typedef struct {
    u16 x;
    u16 y;
    u16 w;
    u16 h;
} Rect;

typedef struct {
    HalfWord head;
    Rect rects[4];
} MenuLayout;

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
} Touch;

extern void **data_021092f4;
extern Touch data_021044a4;
extern u8 data_020f7734[];
extern u8 data_021027f8[];

extern s32 func_0205d0d0(void *a, s32 b, s32 c);
extern void func_0209cb94(void *a);
extern void func_0209cadc(void *a);

void func_0209c94c(void *self) {
    u8 sel;
    u8 i;

    if (M2C_FIELD(*data_021092f4, s32(**)(), 0x1C)(data_021092f4) != 0) {
        return;
    }
    if (data_021044a4.b4) {
        sel = 4;
        for (i = 0; i < 4; i++) {
            Rect *rects = (Rect *) (data_020f7734 + 2);
            u16 y       = (u16) (s16) (s32) data_021044a4.f2c;
            u16 x       = (u16) (s16) (s32) data_021044a4.f2a;
            u16 h       = rects[i].h;
            u16 w       = rects[i].w;
            u16 y0      = rects[i].y;
            u16 x0      = rects[i].x;
            if (x >= x0 && x < x0 + w && y >= y0 && y < y0 + h) {
                sel = i;
                break;
            }
        }
        if (sel == 4) {
            return;
        }
        if (M2C_FIELD(self, u8 *, 0x8c) == sel) {
            func_0209cb94(self);
        }
        M2C_FIELD(self, u8 *, 0x8c) = sel;
        func_0209cadc(self);
        return;
    }
    if (func_0205d0d0(data_021027f8, 0x40, 4) != 0) {
        M2C_FIELD(self, u8 *, 0x8c) -= 1;
        if (M2C_FIELD(self, u8 *, 0x8c) >= 4) {
            M2C_FIELD(self, u8 *, 0x8c) = 3;
        }
        func_0209cadc(self);
        return;
    }
    if (func_0205d0d0(data_021027f8, 0x80, 4) != 0) {
        M2C_FIELD(self, u8 *, 0x8c) += 1;
        if (M2C_FIELD(self, u8 *, 0x8c) >= 4) {
            M2C_FIELD(self, u8 *, 0x8c) = 0;
        }
        func_0209cadc(self);
        return;
    }
    if (func_0205d0d0(data_021027f8, 1, 4) != 0) {
        func_0209cb94(self);
    }
}
