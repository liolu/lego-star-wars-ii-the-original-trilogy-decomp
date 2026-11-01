#include "m2c.h"

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
} Data;

typedef struct {
    u16 x;
    u16 y;
    u16 w;
    u16 h;
} Rect;

typedef struct {
    u8 pad[0x24];
    u8 f24;
    s8 f25;
    s8 f26;
    u8 pad27;
    u8 f28;
} Obj;

extern Data data_021044a4;
extern Rect data_020fe920[];
extern u8 data_021027f8;
extern u8 data_02102900;

extern s32 func_0205d0d0(void *, s32, s32);
extern s32 func_0205d0f8(void *, s32);
extern void func_02060158(void *, s32, s32, s32);
extern void func_0206e6cc(void *, s32);

enum {
    NUM_RECTS = 2,     /* touch rects in data_020fe920 */
    PAD_A     = 0x1,
    PAD_B     = 0x2,
    PAD_UP    = 0x40,
    PAD_DOWN  = 0x80,
};

/*
 * Per-frame input step of a two-entry selector (called by func_0206e2e0 while
 * obj->f28 selects this state). Clears f26, then:
 *  - touch active (bit 4 of data_021044a4): the touch point (0x2a/0x2c, read
 *    as u16) is hit-tested against the NUM_RECTS rects of data_020fe920; rect
 *    0 always selects entry 0, rect 1 selects entry 1 only if f24 is set. A
 *    selection stores f25, applies it via func_0206e6cc and sets f28 = 6,
 *    f26 = 8;
 *  - else pad buttons of data_021027f8: UP moves f25 from 1 to 0, DOWN from 0
 *    to 1 (only if f24), each applied via func_0206e6cc; A sets f28 = 6,
 *    f26 = 8; B (func_0205d0f8) sets f25 = 4, f28 = 6, f26 = 4.
 * Finally, if f28 == 6 and f25 == 0, func_02060158(&data_02102900, 1, 28, 0).
 */
void func_0206e4ac(Obj *obj) {
    s32 hit = 0;

    obj->f26 = 0;
    if (data_021044a4.b4) {
        s32 touched = NUM_RECTS;
        s32 i;
        for (i = 0; i < NUM_RECTS; i++) {
            u16 touch_y = (u16) (s16) (s32) data_021044a4.f2c;
            u16 touch_x = (u16) (s16) (s32) data_021044a4.f2a;
            u16 h       = data_020fe920[i].h;
            u16 w       = data_020fe920[i].w;
            u16 y       = data_020fe920[i].y;
            u16 x       = data_020fe920[i].x;
            if (touch_x >= x && touch_x < x + w && touch_y >= y && touch_y < y + h) {
                touched = i;
                break;
            }
        }
        if (touched != NUM_RECTS) {
            switch (touched) {
                case 0:
                    obj->f25 = touched;
                    hit      = 1;
                    break;
                case 1:
                    if (obj->f24 != 0) {
                        obj->f25 = touched;
                        hit      = 1;
                    }
                    break;
            }
        }
        if (hit) {
            func_0206e6cc(obj, obj->f25);
            obj->f28 = 6;
            obj->f26 = 8;
        }
    } else if (func_0205d0d0(&data_021027f8, PAD_UP, 4) != 0) {
        if (obj->f25 == 1) {
            obj->f25 = 0;
            func_0206e6cc(obj, 0);
        }
    } else if (func_0205d0d0(&data_021027f8, PAD_DOWN, 4) != 0) {
        if (obj->f25 == 0 && obj->f24 != 0) {
            obj->f25 = 1;
            func_0206e6cc(obj, 1);
        }
    } else if (func_0205d0d0(&data_021027f8, PAD_A, 4) != 0) {
        obj->f28 = 6;
        obj->f26 = 8;
    } else if (func_0205d0f8(&data_021027f8, PAD_B) != 0) {
        obj->f25 = 4;
        obj->f28 = 6;
        obj->f26 = 4;
    }
    if (obj->f28 == 6 && obj->f25 == 0) {
        func_02060158(&data_02102900, 1, 28, 0);
    }
}
