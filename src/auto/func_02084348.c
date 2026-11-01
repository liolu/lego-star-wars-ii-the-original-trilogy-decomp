#include "m2c.h"

/*
 * func_02084348 (reached through the pointer table data_020ff3b0): per-frame
 * step of a scrolling object.
 *
 * self->pos (16.16; row = pos >> 16) advances by self->speed; a changed row is
 * handed to func_0208477c(self, row) before self->row is updated.  When
 * index >= count and cur is NULL, done_flag is set, and the step reports done
 * once draw_count + 11 < row.  The object at +0x30 is turned to self->angle
 * (func_020602ec) and placed (func_0206024c) at x = sin * 0x24000 + 0x80000,
 * y = pos + 0x70000 + cos * 0x24000 (data_020f88c4: {sin, cos} s16 pairs per
 * 16 angle units, products rounded to fx 20.12), then func_02060328 runs.
 *
 * A press of any non-d-pad button also reports done: with data_027e02a8 bit
 * 0x40 set every key record 0..func_02073034()-1 of data_021027f8 is tested
 * (func_0205d0d0), otherwise only CUR_RECORD, or data_021044a4.b4.  The same
 * test runs first when row > 8 and then unconditionally.
 *
 * On done: data_02104d90 bytes 0x50/0x51 become 0/-1, then two func_0205ecc4
 * requests go to data_02102900: 0x11 then 0xF when colour_codes is set and
 * f0c == 0x4A; otherwise 4 (args f0c, 0 when colour_codes is set, else f34,
 * f10 after func_02088b78(data_02104df4)) then 0x30.
 */

typedef struct Ang {
    u16 v;
} Ang;

typedef struct Self {
    u8 pad0[0xC];
    s32 f0c;
    s32 f10;
    u8 pad14[0x30 - 0x14];
    u8 obj[0xD0C - 0x30];
    u16 angle;
    u16 padd0e;
    s32 pos;
    s32 speed;
    u8 pad18[0xD1C - 0xD18];
    s32 row;
    s32 count;
    s32 base;
    s32 draw_count;
    s32 index;
    u8 *cur;
    s32 f34;
    u8 pad38[0xEB8 - 0xD38];
    u8 done_flag;
    u8 padeb9;
    u8 colour_codes;
} Self;

typedef struct Fx {
    s32 v;
} Fx;

typedef struct Bits {
    u8 b0 : 1;
    u8 b1 : 1;
    u8 b2 : 1;
    u8 b3 : 1;
    u8 b4 : 1;
} Bits;

typedef struct Sel {
    u8 pad[0x50];
    u8 a;
    s8 b;
} Sel;

extern s32 func_0205d0d0(void *keys, s32 mask, s32 rec);
extern void func_0205ecc4(void *arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4, s32 arg5);
extern void func_0206024c(void *p, Fx x, Fx y, ...);
extern void func_020602ec(void *obj, Ang a, s32 flag);
extern void func_02060328(void *obj);
extern s32 func_02073034(void);
extern void func_0208477c(Self *self, s32 arg1);
extern void func_02088b78(void *);

extern const s16 data_020f88c4[];
extern u8 data_021027f8[];
extern u8 data_02102900[];
extern Bits data_021044a4;
extern Sel data_02104d90;
extern u8 data_02104df4[];
extern s32 data_027e02a8;

/* NDS keypad bits (A B Select Start R L X Y debug); the d-pad 0xF0 is left out. */
enum {
    KEYS_ALL_BUTTONS = 0x2F0F
};
/* key record holding the current input (func_020696f0) */
enum {
    CUR_RECORD = 4
};

static inline s32 fxmul(s32 a, s32 b) {
    return (s32) (((s64) a * b + 0x800) >> 12);
}

static inline s32 ang_sin(Ang a) {
    return data_020f88c4[(a.v >> 4) * 2];
}

static inline s32 ang_cos(Ang a) {
    return data_020f88c4[(a.v >> 4) * 2 + 1];
}

void func_02084348(Self *self) {
    s32 done;
    u8 i;
    s32 pos;
    s32 row;
    Ang a;

    pos  = self->pos + self->speed;
    row  = pos >> 16;
    done = 0;
    if (row != self->row) {
        func_0208477c(self, row);
    }
    if (self->index >= self->count && self->cur == NULL) {
        self->done_flag = 1;
        if (self->draw_count + 11 < self->row) {
            done = 1;
        }
    }
    self->pos = pos;
    self->row = row;
    a.v       = self->angle;
    func_020602ec(self->obj, a, 0);
    {
        Ang c;
        Ang s;
        Fx x;
        Fx y;
        s   = a;
        c   = a;
        y.v = self->pos + 0x70000 + fxmul(ang_cos(c), 0x24000);
        x.v = fxmul(ang_sin(s), 0x24000) + 0x80000;
        func_0206024c(self->obj, x, y);
    }
    func_02060328(self->obj);
    if (row > 8) {
        if (data_027e02a8 & 0x40) {
            for (i = 0; i < func_02073034(); i++) {
                if (func_0205d0d0(data_021027f8, KEYS_ALL_BUTTONS, i) != 0) {
                    done = 1;
                }
            }
        } else if (func_0205d0d0(data_021027f8, KEYS_ALL_BUTTONS, CUR_RECORD) != 0 || data_021044a4.b4) {
            done = 1;
        }
    }
    if (data_027e02a8 & 0x40) {
        for (i = 0; i < func_02073034(); i++) {
            if (func_0205d0d0(data_021027f8, KEYS_ALL_BUTTONS, i) != 0) {
                done = 1;
            }
        }
    } else if (func_0205d0d0(data_021027f8, KEYS_ALL_BUTTONS, CUR_RECORD) != 0 || data_021044a4.b4) {
        done = 1;
    }
    if (done == 0) {
        return;
    }
    data_02104d90.a = 0;
    data_02104d90.b = -1;
    if (self->colour_codes) {
        if (self->f0c != 0x4A) {
            func_0205ecc4(data_02102900, 4, 0, 4, self->f0c, 0);
            func_0205ecc4(data_02102900, 0x30, 1, 0, 0, 0);
        } else {
            func_0205ecc4(data_02102900, 0x11, 0, 0, 0, 0);
            func_0205ecc4(data_02102900, 0xF, 1, 0, 0, 0);
        }
    } else {
        func_02088b78(data_02104df4);
        func_0205ecc4(data_02102900, 4, 0, 4, self->f34, self->f10);
        func_0205ecc4(data_02102900, 0x30, 1, 0, 0, 0);
    }
}
