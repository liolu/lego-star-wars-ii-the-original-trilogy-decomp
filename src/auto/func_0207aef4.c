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
    u8 pad0 : 4;
    u8 active : 1;
    u8 pad1 : 3;
    u8 pad01[0x29];
    u16 x;
    u16 y;
} Touch;

typedef struct {
    u8 kind;
    u8 value;
} Entry;

enum {
    NUM_SLOTS              = 5,
    NUM_VISIBLE_ROWS       = 4,
    NUM_ARROWS             = 2,
    SCROLL_STEP            = 5,
    ARROW_HIGHLIGHT_FRAMES = 6,
};

enum {
    ENTRY_TYPE_DEFAULTS = 0,
    ENTRY_CALL_0207B2C8 = 1,
    ENTRY_CALL_0207B240 = 2,
    ENTRY_SET_FLAG_400  = 3,
    ENTRY_SET_FLAG_800  = 4,
};

typedef struct {
    u8 pad00[0x8];
    s32 flags;
} Item;

typedef struct {
    u8 pad00[0x1c];
    Item *items[NUM_SLOTS];
    s32 top; /* first visible row (func_0207b49c) */
    s32 scroll; /* pixel offset of a scroll in progress, +-42 per row */
    s32 sel;
    s32 queued_dir; /* scroll direction requested mid-scroll */
    s32 count; /* number of entries */
    Entry **entries;
    s32 timers[2]; /* arrow button highlight countdowns */
} Menu;

typedef struct {
    u8 pad000[0x1a8];
    u16 flags;
    u8 pad1aa[0x6bb - 0x1aa];
    u8 type;
} Actor;

typedef struct {
    u8 a;
    u8 pad1[4];
    u8 b;
} Stats;

typedef struct {
    u8 pad00[0x2a];
    u8 slot;
} Game;

extern s32 func_02064998(void *unused, Point pt, const Rect *rects, s32 count, s32 *out);
extern void func_0207b240(Menu *menu, Actor *actor, s32 arg2, u8 arg3);
extern void func_0207b2c8(Menu *menu, Actor *actor, u8 arg2);
extern void func_0207b49c(Menu *menu, s32 a);
extern void func_0207b540(Menu *menu);
extern void func_0207b5dc(Menu *menu, s32 a, s32 b);
extern s32 func_0208da74(u32 arg0);

extern Touch data_021044a4;
extern const Rect data_020ee684[];
extern const Rect data_020ee674[];
extern Game data_021027f8;
extern Actor *data_02109358[];
extern Stats data_02104de2[];
extern u8 data_020ef581[][24];
extern u8 data_020eebbb[];

/*
 * Per-frame update of a scrolling touch-screen list (5 item slots, 4 visible
 * rows starting at menu->top; func_0207b49c/func_0207b540 own the scroll).
 * If the touch flag in data_021044a4 is set, its x/y point is hit-tested:
 *  - against the 4 row rects (data_020ee684): the touched entry
 *    entries[top + row] is selected (sel) and applied to the actor taken from
 *    data_02109358[data_021027f8.slot] according to its kind byte;
 *  - else against the 2 arrow rects (data_020ee674): that arrow's highlight
 *    countdown is armed and a scroll of -1 (arrow 0) or +1 is requested.
 * Then a scroll in progress moves 5 pixels toward 0; once within 5 it snaps,
 * clears bit 0 of the wrapped-around slot's item flags and starts any queued
 * scroll. Finally the arrow countdowns tick (func_0207b5dc(.., 0) on expiry)
 * and func_0207b540 lays out the slots.
 */
void func_0207aef4(Menu *menu) {
    s32 i;
    s32 idx;
    Point pt;

    if (data_021044a4.active) {
        idx  = 0;
        pt.x = data_021044a4.x;
        pt.y = data_021044a4.y;
        if (func_02064998(&data_021044a4, pt, data_020ee684, NUM_VISIBLE_ROWS, &idx)) {
            s32 pos = menu->top + idx;
            Actor *actor;
            Entry *entry;
            if (pos >= menu->count) {
                goto tail;
            }
            menu->sel = pos;
            actor     = data_02109358[data_021027f8.slot];
            if (actor == NULL) {
                goto tail;
            }
            entry = menu->entries[pos];
            switch (entry->kind) {
                case ENTRY_TYPE_DEFAULTS: {
                    u8 a     = 0xfa;
                    u8 b     = 0x28;
                    s32 type = actor->type;
                    if (type != 0x4f) {
                        if (type >= 0x49 && type <= 0x4b) {
                            Stats *st = &data_02104de2[type - 0x49];
                            a         = st->a;
                            b         = st->b;
                        } else {
                            a = data_020eebbb[type * 5];
                            b = data_020ef581[type][0];
                        }
                    }
                    func_0207b240(menu, actor, 3, a);
                    if (b != 0x28) {
                        func_0207b2c8(menu, actor, b);
                    }
                    actor->flags &= ~0xc00;
                    break;
                }
                case ENTRY_CALL_0207B240: {
                    u8 v = entry->value;
                    func_0207b240(menu, actor, func_0208da74(v), v);
                    break;
                }
                case ENTRY_CALL_0207B2C8:
                    func_0207b2c8(menu, actor, entry->value);
                    break;
                case ENTRY_SET_FLAG_400:
                    actor->flags |= 0x400;
                    break;
                case ENTRY_SET_FLAG_800:
                    actor->flags |= 0x800;
                    break;
            }
        } else if (func_02064998(&data_021044a4, pt, data_020ee674, NUM_ARROWS, &idx)) {
            menu->timers[idx] = ARROW_HIGHLIGHT_FRAMES;
            func_0207b5dc(menu, idx, 1);
            if (idx == 0) {
                func_0207b49c(menu, -1);
            } else {
                func_0207b49c(menu, 1);
            }
        }
    }
tail:
    if (menu->scroll != 0) {
        s32 s = menu->scroll;
        if ((s < 0 ? -s : s) <= SCROLL_STEP) {
            menu->items[(menu->top + 4) % NUM_SLOTS]->flags &= ~1;
            menu->scroll = 0;
            if (menu->queued_dir != 0) {
                func_0207b49c(menu, menu->queued_dir);
                menu->queued_dir = 0;
            }
        } else if (s < 0) {
            menu->scroll += SCROLL_STEP;
        } else {
            menu->scroll -= SCROLL_STEP;
        }
    }
    for (i = 0; i < NUM_ARROWS; i++) {
        if (menu->timers[i] != 0) {
            if (--menu->timers[i] == 0) {
                func_0207b5dc(menu, i, 0);
            }
        }
    }
    func_0207b540(menu);
}
