/*
 * func_0207bb54: per-frame input step of a three-slot menu with a back entry
 * (entry 3); self->sel is the selected entry.  Runs only while self->state is
 * 2.
 *
 * self->sub == 1, or sub == 0 with self->mode == 1 (the latter first copies
 * the 6-byte record data_02104d48 into data_02104de2[sel], calls
 * func_02086458(&data_02104d90, sel + SLOT_BIT_BASE), stores sel +
 * SLOT_BIT_BASE at data_02106f90 + 0x48, writes four bytes of data_02106d90
 * and stops unless func_02085cb0(&data_02104d90, 1) returns nonzero): sends
 * the same two func_0205ecc4(&data_02102900, ...) requests that
 * func_0207c118 sends when mode is 0.
 *
 * Otherwise, touch (data_021044a4 bit 2): the x/y point is hit-tested against
 * the ENTRY_COUNT rects of data_020efee8.  Touching another entry plays
 * VOICE_MOVE, selects it with func_0207c1b0 and reloads self->timer with
 * HOLD_TICKS; staying on the selected entry counts the timer down and at 0
 * (or at once with bit 4 set) plays VOICE_ACCEPT and activates the entry:
 * func_0207c118 for ENTRY_BACK, else func_0207c054.
 *
 * No touch: the timer is reloaded and the current key record of
 * data_021027f8 is tested by func_0205d0d0 (NDS keypad bits).  On the back
 * entry A or B goes back and Up moves to the first selectable slot; on a
 * slot A activates, B goes back, Left/Right step to the previous/next
 * selectable slot with wrap-around and Down selects the back entry.  A slot
 * is selectable when mode is nonzero or its bit (slot + SLOT_BIT_BASE) is
 * set (func_02086534 on data_02104d90).
 */
#include "m2c.h"

typedef struct {
    u16 x;
    s16 y;
} Point;

typedef struct {
    u16 x;
    u16 y;
    u16 w;
    u16 h;
} Rect;

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
    u16 x;
    u16 y;
} Touch;

typedef struct {
    u8 v[6];
} Stats;

typedef struct {
    u8 pad0[0x48];
    u16 f48;
} Data6f90;

typedef struct {
    u8 pad0[0x262];
    u8 f262;
    u8 f263;
    u8 pad264[0xbb2 - 0x264];
    u8 fbb2;
    u8 fbb3;
} Game;

typedef struct {
    u8 pad0[0xdc];
    u8 sel;   /* selected entry (0..2 slot, 3 back) */
    u8 mode;
    u8 state;
    u8 sub;
    u8 timer; /* touch hold countdown */
} Obj;

extern s32 func_02064998(void *unused, Point pt, const Rect *rects, s32 count, s32 *out);
extern s32 func_0205d0d0(void *keys, s32 mask, s32 rec);
extern void func_0205ecc4(void *arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5);
extern void func_0207c054(Obj *self);
extern void func_0207c118(Obj *self);
extern void func_0207c1b0(Obj *self, s32 idx);
extern s32 func_02085cb0(void *arg0, s32 arg1);
extern void func_02086458(void *base, s32 bit);
extern s32 func_02086534(void *base, s32 bit);
extern void func_020b8f70(s32 voice, s32 arg1);

extern Touch data_021044a4;
extern const Rect data_020efee8[];
extern u8 data_021027f8;
extern u8 data_02102900;
extern Stats data_02104d48;
extern u8 data_02104d90;
extern Stats data_02104de2[];
extern Game data_02106d90;
extern Data6f90 data_02106f90;

enum {
    STATE_ACTIVE  = 2,    /* self->state value this step runs in */
    ENTRY_COUNT   = 4,    /* entries and touch rects */
    ENTRY_BACK    = 3,    /* back entry; slots are 0..2 */
    LAST_SLOT     = 2,
    CUR_RECORD    = 4,    /* func_0205d0d0 record index: current record */
    HOLD_TICKS    = 10,   /* steps the touch must stay on an entry */
    SLOT_BIT_BASE = 0x49, /* bit index of slot 0 in data_02104d90 */
    KEY_A         = 0x1,
    KEY_B         = 0x2,
    KEY_A_B       = 0x3,
    KEY_RIGHT     = 0x10,
    KEY_LEFT      = 0x20,
    KEY_UP        = 0x40,
    KEY_DOWN      = 0x80,
    VOICE_ACCEPT  = 0xB4,
    VOICE_MOVE    = 0xB6,
};

static inline void send_requests(void) {
    func_0205ecc4(&data_02102900, 4, 0, 4, 0xe, 1);
    func_0205ecc4(&data_02102900, 0x30, 1, 0, 0, 0);
}

void func_0207bb54(Obj *self) {
    Point pt;
    s32 idx;

    if (self->state != STATE_ACTIVE) {
        return;
    }
    if (self->sub == 0) {
        if (self->mode == 0) {
        } else if (self->mode == 1) {
            {
                Stats *dst = &data_02104de2[self->sel];
                dst->v[0]  = data_02104d48.v[0];
                dst->v[1]  = data_02104d48.v[1];
                dst->v[2]  = data_02104d48.v[2];
                dst->v[3]  = data_02104d48.v[3];
                dst->v[4]  = data_02104d48.v[4];
                dst->v[5]  = data_02104d48.v[5];
            }
            func_02086458(&data_02104d90, self->sel + SLOT_BIT_BASE);
            data_02106f90.f48  = self->sel + SLOT_BIT_BASE;
            data_02106d90.f262 = 4;
            data_02106d90.f263 = 1;
            data_02106d90.fbb2 = 0xe;
            data_02106d90.fbb3 = 0;
            if (func_02085cb0(&data_02104d90, 1) == 0) {
                return;
            }
            send_requests();
            return;
        }
    } else if (self->sub == 1) {
        send_requests();
        return;
    }

    if (data_021044a4.b2) {
        idx  = ENTRY_COUNT;
        pt.x = data_021044a4.x;
        pt.y = data_021044a4.y;
        if (func_02064998(&data_021044a4, pt, data_020efee8, ENTRY_COUNT, &idx) == 0) {
            return;
        }
        if (self->sel == idx) {
            if (!data_021044a4.b4) {
                if (--self->timer != 0) {
                    return;
                }
            }
            func_020b8f70(VOICE_ACCEPT, 0);
            if (idx == ENTRY_BACK) {
                func_0207c118(self);
            } else {
                func_0207c054(self);
            }
        } else {
            func_020b8f70(VOICE_MOVE, 0);
            func_0207c1b0(self, idx);
            self->timer = HOLD_TICKS;
        }
        return;
    }

    self->timer = HOLD_TICKS;
    if (self->sel == ENTRY_BACK) {
        if (func_0205d0d0(&data_021027f8, KEY_A_B, CUR_RECORD)) {
            func_0207c118(self);
        } else if (func_0205d0d0(&data_021027f8, KEY_UP, CUR_RECORD)) {
            s32 slot;
            func_020b8f70(VOICE_MOVE, 0);
            for (slot = 0; self->mode == 0 && !func_02086534(&data_02104d90, slot + SLOT_BIT_BASE); slot++) {
            }
            func_0207c1b0(self, slot);
        }
    } else if (func_0205d0d0(&data_021027f8, KEY_A, CUR_RECORD)) {
        func_0207c054(self);
    } else if (func_0205d0d0(&data_021027f8, KEY_B, CUR_RECORD)) {
        func_0207c118(self);
    } else if (func_0205d0d0(&data_021027f8, KEY_LEFT, CUR_RECORD)) {
        s32 slot;
        func_020b8f70(VOICE_MOVE, 0);
        slot = self->sel;
        do {
            if (--slot < 0) {
                slot = LAST_SLOT;
            }
        } while (self->mode == 0 && !func_02086534(&data_02104d90, slot + SLOT_BIT_BASE));
        func_0207c1b0(self, slot);
    } else if (func_0205d0d0(&data_021027f8, KEY_RIGHT, CUR_RECORD)) {
        s32 slot;
        func_020b8f70(VOICE_MOVE, 0);
        slot = self->sel;
        do {
            if (++slot >= ENTRY_BACK) {
                slot = 0;
            }
        } while (self->mode == 0 && !func_02086534(&data_02104d90, slot + SLOT_BIT_BASE));
        func_0207c1b0(self, slot);
    } else if (func_0205d0d0(&data_021027f8, KEY_DOWN, CUR_RECORD)) {
        func_020b8f70(VOICE_MOVE, 0);
        func_0207c1b0(self, ENTRY_BACK);
    }
}
