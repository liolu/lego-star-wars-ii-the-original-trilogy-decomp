#include "m2c.h"

typedef struct Slot {
    u8 pad0[0x14];
    u32 pad14 : 12;
    u32 lvl : 4;
} Slot;

typedef struct SlotList {
    u8 pad[8];
    Slot *s[4];
} SlotList;

typedef struct Owner {
    u8 pad0[0xC];
    SlotList *list;
    u8 pad1[0x20];
    u8 st[4];
} Owner;

typedef struct Plr {
    u8 pad0[8];
    u32 flags;
    u8 pad1[0x5CA];
    u8 b5d6;
    u8 pad2[0xE4];
    u8 b6bb : 8; /* FAKE: full-width bitfield in a byte gives the load a consumer, which fixes its schedule slot */
} Plr;

typedef struct Game {
    u8 pad[0x4C];
    Plr *players[4];
} Game;
typedef struct Sel {
    u8 pad[0x2A];
    u8 cur;
} Sel;

extern Game data_0210930c;
extern Sel data_021027f8;
typedef struct {
    s32 v;
} WordArg;
extern s32 func_0200aef8(void *, void *, WordArg);

enum {
    STATE_UNSET = 0x64
};      /* state recorded for a slot with no qualifying player */
enum {
    PLR_FLAGS_MASK = 0x803
};  /* (flags & mask) == 1: bit 0 set, bits 1 and 11 clear */
enum {
    LVL_KEPT = 0xD
};          /* slot level that is not overwritten with LVL_DEFAULT */
enum {
    LVL_DEFAULT = 0xF
};
enum {
    LVL_ALT = 0xE
};
enum {
    DIST_LIMIT = 0x32000
};    /* passed to the +0xb0 position check func_0200aef8 */

/* For each of the 4 player slots, works out the byte state (Plr byte 0x6bb, or
 * STATE_UNSET) and updates the 4-bit level field of self->list->s[i]:
 * - (flags & 0x803) == 1: level LVL_DEFAULT unless it is LVL_KEPT; then LVL_ALT
 *   if func_0200aef8 returns 0 for the current player (data_021027f8.cur) and
 *   this one (skipped for slot 0) or the current player's byte 0x5d6 is set;
 * - otherwise, if flags bit 0 is set: level LVL_ALT.
 * mode != 0 (func_020682c4 passes 1) stores every state in self->st[i]
 * (STATE_UNSET if the player has flags bit 0 clear) and returns 0.
 * mode == 0 stores a differing state only if the slot has no player or flags
 * bit 0 set, and returns 1 if any was stored (func_02068598 tests for 0). */

s32 func_020691b4(Owner *self, s32 mode) {
    s32 changed = 0;
    u32 i;
    for (i = 0; i < 4; i++) {
        u32 state = STATE_UNSET;
        Plr *p    = data_0210930c.players[i];
        if (p != NULL && (p->flags & PLR_FLAGS_MASK) == 1) {
            s32 flag = 1;
            state    = p->b6bb;
            if (i != 0) {
                WordArg radius;
                radius.v = DIST_LIMIT;
                flag     = func_0200aef8((u8 *) data_0210930c.players[data_021027f8.cur] + 0xb0, (u8 *) p + 0xb0, radius);
            }
            if (self->list->s[i]->lvl != LVL_KEPT) {
                self->list->s[i]->lvl = LVL_DEFAULT;
            }
            if (flag == 0 || data_0210930c.players[data_021027f8.cur]->b5d6 != 0) {
                self->list->s[i]->lvl = LVL_ALT;
            }
        } else if (p != NULL && (p->flags & 1) == 1) {
            state                 = p->b6bb;
            self->list->s[i]->lvl = LVL_ALT;
        }
        if (mode != 0) {
            if (state != STATE_UNSET && p != NULL && (p->flags & 1) != 1) {
                state = STATE_UNSET;
            }
            self->st[i] = state;
        } else if (state != self->st[i]) {
            if (p == NULL || (p != NULL && (p->flags & 1) == 1)) {
                self->st[i] = state;
                changed     = 1;
            }
        }
    }
    return changed;
}
