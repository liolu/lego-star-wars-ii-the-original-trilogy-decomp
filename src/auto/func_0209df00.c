/*
 * func_0209df00: one input step of a four-entry menu whose current entry is
 * self->sel (byte at 0xac).  Called from func_0209dd64.
 *
 * Nothing happens while the data_021092f4 object's slot-7 method (+0x1c)
 * returns non-zero.
 *
 * Touch (data_021044a4 bit 4; point at 0x2a / 0x2c read as int, then as u16):
 * entry i (0..3) is hit when the point lies in [x, x+w) x [y, y+h) of one of
 * its rects; the first hit in entry order wins.  data_020f77e4[i] is the entry
 * itself; for i < LR_RECT_COUNT, data_020f77c4[i] and data_020f77a4[i] select
 * entry i at once and call func_0209e338(self, -1) / (self, 1), the same call
 * the Left / Right keys make.  A miss returns.  A hit on the entry rect calls
 * func_0209e338(self, 0) (as A does) when it is already the selected entry;
 * every hit then stores the entry in self->sel and calls func_0209e1ac(self).
 *
 * Keys (no touch): func_0205d0d0(&data_021027f8, bit, CUR_RECORD) tests an NDS
 * keypad bit (see func_020696f0).  Up / Down step self->sel back / forward with
 * wrap and call func_0209e1ac(self); Left, Right and A call func_0209e338(self,
 * dir) with dir -1, 1 and 0.
 */
#include "m2c.h"

typedef struct Rect {
    u16 x, y, w, h;
} Rect;
typedef struct Input {
    u8 lo : 4;
    u8 touching : 1;
    u8 hi : 3;
    u8 pad[0x29];
    s16 x;
    s16 y;
} Input;
typedef struct VObj {
    void **vtbl;
} VObj;
typedef struct Menu {
    u8 pad[0xac];
    u8 sel;
} Menu;

extern VObj *data_021092f4;
extern Input data_021044a4;
extern const Rect data_020f77a4[];
extern const Rect data_020f77c4[];
extern const Rect data_020f77e4[];
extern M2C_UNK data_021027f8;

extern s32 func_0205d0d0(M2C_UNK *, M2C_UNK, M2C_UNK);
extern void func_0209e1ac(void *arg0);
extern void func_0209e338(void *arg0, s32 arg1);

enum {
    ENTRY_COUNT   = 4,    /* menu entries; also "no entry hit" */
    LR_RECT_COUNT = 3,    /* entries with Left / Right touch rects */
    CUR_RECORD    = 4,    /* func_0205d0d0 record index: the current one */
    KEY_A         = 0x1,
    KEY_RIGHT     = 0x10,
    KEY_LEFT      = 0x20,
    KEY_UP        = 0x40,
    KEY_DOWN      = 0x80,
};

static inline s32 touch_x(void) {
    return data_021044a4.x;
}

static inline s32 touch_y(void) {
    return data_021044a4.y;
}

static inline int in_rect(const Rect *r, u16 ux, u16 uy) {
    u16 rh = r->h;
    u16 rw = r->w;
    u16 rx = r->x;
    u16 ry = r->y;
    return ux >= rx && ux < rx + rw && uy >= ry && uy < ry + rh;
}

void func_0209df00(Menu *arg0) {
    u8 i;
    u32 sel;

    if (((s32(*)(VObj *)) data_021092f4->vtbl[7])(data_021092f4) != 0) {
        return;
    }
    if (data_021044a4.touching) {
        sel = ENTRY_COUNT;
        for (i = 0; i < ENTRY_COUNT; i++) {
            u16 ty = touch_y();
            u16 tx = touch_x();
            if (in_rect(&data_020f77e4[i], tx, ty)) {
                sel = i;
                break;
            }
            if (i < LR_RECT_COUNT) {
                if (in_rect(&data_020f77c4[i], tx, ty)) {
                    sel       = i;
                    arg0->sel = i;
                    func_0209e338(arg0, -1);
                    break;
                }
                if (in_rect(&data_020f77a4[i], tx, ty)) {
                    sel       = i;
                    arg0->sel = i;
                    func_0209e338(arg0, 1);
                    break;
                }
            }
        }
        if (sel == ENTRY_COUNT) {
            return;
        }
        if (arg0->sel == sel) {
            func_0209e338(arg0, 0);
        }
        arg0->sel = sel;
        func_0209e1ac(arg0);
        return;
    }
    if (func_0205d0d0(&data_021027f8, KEY_UP, CUR_RECORD) != 0) {
        arg0->sel -= 1;
        if (arg0->sel >= ENTRY_COUNT) {
            arg0->sel = ENTRY_COUNT - 1;
        }
        func_0209e1ac(arg0);
        return;
    }
    if (func_0205d0d0(&data_021027f8, KEY_DOWN, CUR_RECORD) != 0) {
        arg0->sel += 1;
        if (arg0->sel >= ENTRY_COUNT) {
            arg0->sel = 0;
        }
        func_0209e1ac(arg0);
        return;
    }
    if (func_0205d0d0(&data_021027f8, KEY_LEFT, CUR_RECORD) != 0) {
        func_0209e338(arg0, -1);
        return;
    }
    if (func_0205d0d0(&data_021027f8, KEY_RIGHT, CUR_RECORD) != 0) {
        func_0209e338(arg0, 1);
        return;
    }
    if (func_0205d0d0(&data_021027f8, KEY_A, CUR_RECORD) != 0) {
        func_0209e338(arg0, 0);
    }
}
