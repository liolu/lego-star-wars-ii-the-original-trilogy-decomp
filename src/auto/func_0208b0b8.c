/* Reconstructed from USA ARM9 at 0x0208b0b8 (700 bytes).
 *
 * Per-frame update of a display object (called from func_0208aaf0 and
 * func_0208cdb0). Steps: skip when flag bits 0x14 are set; refresh via
 * func_0208c4c8/func_0208b4f8; fetch the current state value (from
 * func_0208559c, or func_02085594 when func_02093fd0 reports 0x10) and,
 * when it changes, apply it (func_0208aedc) or clear it (func_0208ad84,
 * resetting four panel halfwords to 0x40); advance a 4-bit frame counter
 * every second call; update the two 3-byte piece records (cur, count,
 * last index; laid out by func_0208afb0/func_0208afd8) from the slot table
 * entry and the linked owner; every fourth call bounce a signed value
 * between 0 and 2 and redraw each record's last piece with it
 * (func_0208b064); finally consume flag bit 0x2 via func_0208bb5c.
 */
#include "m2c.h"

enum {
    FRAME_TIMER_RELOAD = 2,
    FRAME_MASK         = 0xf,
    PULSE_TIMER_RELOAD = 4,
    PULSE_LIMIT        = 3,
    NO_PIECE           = 0xff,
    PANEL_RESET_VALUE  = 0x40,
};

typedef struct {
    u8 cur;
    u8 count;
    u8 idx;
} PieceRecord;

typedef struct {
    u8 pad00[0x6b6];
    s16 f6b6;
    s16 f6b8;
} Owner;

typedef struct {
    u8 pad00[0x6c];
    u8 *owner_link;     /* 0x6c */
    u8 pad70[4];
    s32 state;          /* 0x74 */
    u32 flags;          /* 0x78 */
    u8 pad7c[4];
    u8 frame;           /* 0x80 */
    u8 frame_timer;            /* 0x81 */
    u8 pulse_timer;      /* 0x82 */
    s8 pulse;           /* 0x83 */
    s8 pulse_step;      /* 0x84 */
    u8 pad85[4];
    PieceRecord records[2];    /* 0x89 */
} Obj;

typedef struct {
    u8 pad00[0x4c];
    u16 f4c;
    u16 f4e;
    u8 pad50[0x3c];
    u16 f8c;
    u16 f8e;
} Panel;

static inline Owner *owner_of(u8 *link) {
    Owner *owner = (Owner *) link;
    if (link != 0) {
        owner = (Owner *) (link - 4);
    }
    return owner;
}

extern s32 func_02053b14(s32 *arg0);
extern s32 func_02085594(void *arg0);
extern s32 func_0208559c(void *arg0);
extern void *func_0208ad84(void *arg0);
extern void func_0208aedc(void *arg0, s32 arg1);
extern void func_0208af20(void *arg0);
extern void func_0208afb0(void *arg0, u8 *arg1, s32 arg2, s32 arg3);
extern void func_0208b064(void *arg0, u16 *arg1, s32 arg2);
extern void func_0208b4f8(void *arg0);
extern void func_0208b54c(void *self, u8 *arg1);
extern void func_0208bb5c(void *arg0, s32 arg1);
extern void func_0208c0c0(void *arg0, s32 arg1);
extern s32 func_0208c4c8(u8 *arg0, void *arg1);
extern u8 func_020917b4(void *arg0);
extern u8 func_02093fd0(s32 arg0);

extern u8 data_021027f8[];
extern u8 data_02104d94[];
extern u8 data_0210930c[];
extern u8 *data_02109358[];

void func_0208b0b8(Obj *self) {
    u8 slot;
    s32 state;
    u8 *entry;
    u32 i;

    if (self->flags & 0x14) {
        return;
    }
    if (func_0208c4c8((u8 *) self + 0x40, (u8 *) self + 8) != 0) {
        func_0208b4f8(self);
    }
    slot  = data_021027f8[0x2a];
    state = func_0208559c(data_02104d94);
    if (func_02093fd0(func_020917b4(data_0210930c)) == 0x10) {
        state = func_02085594(data_02104d94);
    }
    if (self->state != state) {
        if (state != 0) {
            func_0208aedc(self, state);
        } else {
            func_0208ad84(self);
            if (self->state != 0) {
                Panel *p = (Panel *) func_02053b14((s32 *) ((u8 *) self + 8));
                p->f4c   = PANEL_RESET_VALUE;
                p->f4e   = PANEL_RESET_VALUE;
                p->f8c   = PANEL_RESET_VALUE;
                p->f8e   = PANEL_RESET_VALUE;
            }
        }
        self->state = state;
    }
    if (--self->frame_timer == 0) {
        self->frame_timer = FRAME_TIMER_RELOAD;
        self->frame++;
        self->frame &= FRAME_MASK;
        if (state != 0) {
            func_0208af20(self);
        }
    }
    entry = data_02109358[slot];
    if ((*(u32 *) (entry + 8) & 1) == 1) {
        func_0208afb0(self, &self->records[0].cur, ((Owner *) entry)->f6b6, 1);
    }
    if (self->flags & 0x60) {
        func_0208c0c0(self, 0);
    } else if (owner_of(self->owner_link) != 0) {
        self->records[1].count = owner_of(self->owner_link)->f6b8;
        func_0208afb0(self, &self->records[1].cur, owner_of(self->owner_link)->f6b6, 0);
    } else if (self->flags & 8) {
        func_0208b54c(self, 0);
    }
    if (--self->pulse_timer == 0) {
        self->pulse_timer = PULSE_TIMER_RELOAD;
        self->pulse += self->pulse_step;
        if (self->pulse < 0 || self->pulse >= PULSE_LIMIT) {
            self->pulse_step *= -1;
            self->pulse += self->pulse_step;
            self->pulse += self->pulse_step;
        }
        for (i = 0; i < 2; i++) {
            if (self->records[i].idx != NO_PIECE) {
                u16 *row;
                s32 base = func_02053b14((s32 *) ((u8 *) self + 8));
                if (i == 0) {
                    row = (u16 *) (base + 0xcc);
                } else {
                    row = (u16 *) (base + 0x5a0 - ((u32) self->records[i].count >> 1) * 4);
                }
                func_0208b064(self, row + self->records[i].idx * 2, self->pulse);
            }
        }
    }
    if (self->flags & 2) {
        self->flags &= ~2;
        func_0208bb5c(self, data_02109358[data_021027f8[0x2a]][0x6bb]);
    }
}
