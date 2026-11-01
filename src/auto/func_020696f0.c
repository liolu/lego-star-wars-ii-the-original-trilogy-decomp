/*
 * func_020696f0: one input step of a four-entry menu (entry in self->sel; entry
 * 3 is the back entry).  func_0206962c calls it when no other request is
 * pending.  Always returns 0.
 *
 * Touch (data_021044a4.b2 set): the 16-bit point at 0x2a/0x2c (x, y) is tested
 * against the four {x, y, w, h} rectangles of data_020fe7b0 (hit when x in
 * [x, x+w) and y in [y, y+h)); the first hit wins.  Touching another entry
 * plays VOICE_MOVE (func_020b8f70 starts a voice), selects it and reloads the
 * hold counter self->timer with HOLD_TICKS.  Staying on the selected entry
 * counts it down; at 0, or when data_021044a4.b4 is set, the entry is
 * activated and the counter reloaded.  A miss or no touch reloads it too.
 *
 * Keys (no touch): data_021027f8 holds 10-byte records whose halfword at +4 is
 * tested by func_0205cf78 / func_0205d0d0 (record CUR_RECORD = the current one).
 * The bits follow the NDS keypad layout: func_0205cf78 steps sel through
 * 0..3 with wrap on Left (0x20) / Right (0x10); Up or Down (KEY_UP_DOWN) jumps
 * to entry 0 (Up) or 3; A activates the entry; B moves to entry 3 (playing
 * VOICE_CANCEL if it moved) and sets back.
 *
 * Activating: entry 3 plays VOICE_ACCEPT and sets back.  Entries 0-2 play
 * VOICE_REFUSED and do nothing more when self->f28 == 0 and
 * func_020861a0(&data_02104d90, sel) is true (the entry's byte in
 * data_021043ac is zero); otherwise they play VOICE_ACCEPT and set confirm.
 *
 * Afterwards a changed selection is shown by func_02069be8(self, sel) (mode 3
 * on entry sel, 0 on the others).  confirm then sends
 * func_020601ac(&data_02102900, 1, id, -1, 2, 0) with REQ_CONFIRM, or with
 * self->f28 set REQ_CONFIRM_F28_SET / _CLEAR by func_020861a0; back sends it
 * with REQ_BACK, or REQ_BACK_F28 when self->f28 == 1 (REQ_BACK_F28_OPT when
 * data_02106d90.opts->f6 is set).
 */
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
} Input;

typedef struct {
    u16 x;
    u16 y;
    u16 w;
    u16 h;
} Rect;

typedef struct {
    u8 pad0[6];
    u8 f6;
} Opts;

typedef struct {
    u8 pad0[0xBB8];
    Opts *opts;
} Game;

typedef struct {
    u8 pad00[0x1C];
    u32 sel;
    u8 pad20[4];
    s32 timer;
    u8 f28;
} Obj;

extern Input data_021044a4;
extern Rect data_020fe7b0[];
extern u8 data_021027f8;
extern u8 data_02102900;
extern u8 data_02104d90;
extern Game data_02106d90;

s32 func_0205cf78(void *, void *, s32, s32, s32);
s32 func_0205d0d0(void *, s32, s32);
void func_020601ac(void *, s32, s32, s32, s32, s32);
void func_02069be8(void *, s32);
s32 func_020861a0(void *, s32);
void func_020b8f70(s32, s32);

enum {
    ENTRY_COUNT         = 4,    /* menu entries and touch rectangles */
    ENTRY_BACK          = 3,    /* entry selected by B */
    CUR_RECORD          = 4,    /* record index: use the current record */
    HOLD_TICKS          = 10,   /* steps the touch must stay on an entry */
    KEY_A               = 0x1,
    KEY_B               = 0x2,
    KEY_UP              = 0x40,
    KEY_UP_DOWN         = 0xC0,
    VOICE_ACCEPT        = 0xB4,
    VOICE_CANCEL        = 0xB5,
    VOICE_MOVE          = 0xB6,
    VOICE_REFUSED       = 0xB7,
    REQ_CONFIRM         = 0x29,
    REQ_BACK            = 0x2A,
    REQ_CONFIRM_F28_SET = 0x2D,
    REQ_BACK_F28        = 0x2E,
    REQ_BACK_F28_OPT    = 0x2F,
    REQ_CONFIRM_F28_CLR = 0x30,
};

s32 func_020696f0(Obj *self) {
    s32 confirm = 0;
    s32 touched = 0; /* the touch path ran: skip the key path */
    s32 back    = 0;
    s32 changed = 0;
    s32 idx;
    u32 i;

    if (data_021044a4.b2) {
        touched = 1;
        idx     = ENTRY_COUNT;
        for (i = 0; i < ENTRY_COUNT; i++) {
            u16 touch_y = (u16) (s16) (s32) data_021044a4.f2c;
            u16 touch_x = (u16) (s16) (s32) data_021044a4.f2a;
            u16 rh      = data_020fe7b0[i].h;
            u16 rw      = data_020fe7b0[i].w;
            u16 ry      = data_020fe7b0[i].y;
            u16 rx      = data_020fe7b0[i].x;
            if (touch_x >= rx && touch_x < rx + rw && touch_y >= ry && touch_y < ry + rh) {
                idx = i;
                break;
            }
        }
        if (idx != ENTRY_COUNT) {
            if (self->sel != idx) {
                func_020b8f70(VOICE_MOVE, 0);
                self->timer = HOLD_TICKS;
                self->sel   = idx;
                changed     = 1;
            } else if (--self->timer == 0 || data_021044a4.b4) {
                self->timer = HOLD_TICKS;
                if (self->sel > 2) {
                    func_020b8f70(VOICE_ACCEPT, 0);
                    back = 1;
                } else if (self->f28 == 0 && func_020861a0(&data_02104d90, self->sel)) {
                    func_020b8f70(VOICE_REFUSED, 0);
                } else {
                    func_020b8f70(VOICE_ACCEPT, 0);
                    confirm = 1;
                }
            }
        } else {
            self->timer = HOLD_TICKS;
        }
    } else {
        self->timer = HOLD_TICKS;
    }
    if (!touched) {
        if (func_0205cf78(&data_021027f8, &self->sel, 0, ENTRY_COUNT, CUR_RECORD)) {
            func_020b8f70(VOICE_MOVE, 0);
            changed = 1;
        } else if (func_0205d0d0(&data_021027f8, KEY_UP_DOWN, CUR_RECORD)) {
            func_020b8f70(VOICE_MOVE, 0);
            self->sel = func_0205d0d0(&data_021027f8, KEY_UP, CUR_RECORD) ? 0 : ENTRY_BACK;
            changed   = 1;
        } else if (func_0205d0d0(&data_021027f8, KEY_A, CUR_RECORD)) {
            if (self->sel > 2) {
                func_020b8f70(VOICE_ACCEPT, 0);
                back = 1;
            } else if (self->f28 == 0 && func_020861a0(&data_02104d90, self->sel)) {
                func_020b8f70(VOICE_REFUSED, 0);
            } else {
                func_020b8f70(VOICE_ACCEPT, 0);
                confirm = 1;
            }
        } else if (func_0205d0d0(&data_021027f8, KEY_B, CUR_RECORD)) {
            if (self->sel != ENTRY_BACK) {
                func_020b8f70(VOICE_CANCEL, 0);
                self->sel = ENTRY_BACK;
                changed   = 1;
            }
            back = 1;
        }
    }
    if (changed) {
        func_02069be8(self, self->sel);
    }
    if (confirm) {
        s32 id = REQ_CONFIRM;
        if (self->f28) {
            id = func_020861a0(&data_02104d90, self->sel) ? REQ_CONFIRM_F28_SET : REQ_CONFIRM_F28_CLR;
        }
        func_020601ac(&data_02102900, 1, id, -1, 2, 0);
    } else if (back) {
        s32 id = self->f28 == 1 ? REQ_BACK_F28 : REQ_BACK;
        if (id == REQ_BACK_F28 && data_02106d90.opts->f6) {
            id = REQ_BACK_F28_OPT;
        }
        func_020601ac(&data_02102900, 1, id, -1, 2, 0);
    }
    return 0;
}
