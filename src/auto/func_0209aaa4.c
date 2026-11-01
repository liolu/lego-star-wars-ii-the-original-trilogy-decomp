/* Reconstructed from USA ARM9 at 0x0209aaa4 (784 bytes). Caller: func_0209a3cc.
 *
 * Per-frame update of a four-slot ranking display. For every slot whose entry
 * in data_0210930c is set (read between func_02001688(0) and func_020016c0()):
 *
 * - Resting slot (flags[i] == 0): find its rank (position in order[1..4]) and
 *   draw its numbers with func_0209ae80 (decimal number into the object's tile
 *   map) at pixel row rank * 48 + offset: one number in mode 0, two otherwise.
 *   If the slot ranked just above is also resting and its func_0209bc04 value
 *   is lower, both slots start moving: each one's target becomes the other's
 *   current position word, both flags are set and their order entries swap.
 * - Moving slot: step the position words of both per-slot objects towards the
 *   saved target by 3.0 (fx32, converted by func_02007f24) and snap to the
 *   target, clearing the flag, once the distance is below one step.
 *
 * Every read of the manager (*data_02107860) is gated by bit 0x40 of
 * data_027e02a8; when it is clear the numbers and values read as 0.
 */
#include "m2c.h"

typedef struct Slot {
    u8 pad0[0x50];
    s32 value;
} Slot;

typedef struct Obj {
    u8 pad0[0x84];
    Slot *slotsA[4];   /* 0x84 */
    Slot *slotsB[4];   /* 0x94 */
    u8 pad1[0xc8 - 0xa4];
    u8 order[5];       /* 0xc8 */
    u8 flags[4];       /* 0xcd */
    u8 pad2[3];
    s32 saved[4];      /* 0xd4 */
    s32 mode;          /* 0xe4 */
} Obj;

typedef struct Tab {
    u8 pad0[0x4c];
    void *entries[4];
} Tab;

typedef struct Pad {
    u8 pad0[0x14];
    u16 a[4];     /* 0x14 */
    u16 b[4];     /* 0x1c */
    u16 c[4];     /* 0x24 */
} Pad;

enum {
    SLOT_COUNT = 4
};
enum {
    MANAGER_READ_FLAG = 0x40
};  /* bit of data_027e02a8 gating every manager read */
enum {
    FX_ONE = 0x1000
};           /* fx32 1.0 */
/* func_0209ae80 pixel coordinates: column, rank row pitch, row offsets. */
enum {
    NUMBER_COLUMN  = 0x40,
    RANK_ROW_PITCH = 0x30,
    SINGLE_ROW     = 0x10,
    FIRST_ROW      = 8,
    SECOND_ROW     = 0x18
};

extern Pad *data_02107860[];
extern Tab data_0210930c;
extern s32 data_027e02a8;

void func_02001688(s32 arg0);
void func_020016c0(void);
void func_02007f24(s32 *out, f64 value);
void func_0209ae80(void *self, u16 value, s32 y, s16 x);
s16 func_0209bc04(void *arg0, s32 arg1);

/* One-word fx32 wrapper, passed by value. */
typedef struct FxW {
    s32 v;
} FxW;
static inline s32 FxMulW(s32 x, FxW y) {
    return (s32) (((s64) x * y.v + 0x800) >> 12);
}
static inline void fx_set(FxW *p, s32 v) {
    p->v = v;
}

void func_0209aaa4(Obj *obj) {
    u8 i;
    for (i = 0; i < SLOT_COUNT; i++) {
        void *entry;
        u16 a, b, c;
        u8 rank;
        Pad **mgr = data_02107860;
        func_02001688(0);
        entry = data_0210930c.entries[i];
        func_020016c0();
        if (entry == 0) {
            continue;
        }
        b = 0;
        c = 0;
        a = 0;
        if (data_027e02a8 & MANAGER_READ_FLAG) {
            b = (*mgr)->b[i];
            c = (*mgr)->c[i];
            a = (*mgr)->a[i];
        }
        if (obj->flags[i] == 0) {
            s32 score;
            for (rank = 0; rank < SLOT_COUNT; rank++) {
                if (obj->order[rank + 1] == i) {
                    break;
                }
            }
            if (obj->mode == 0) {
                func_0209ae80(obj, a, NUMBER_COLUMN, rank * RANK_ROW_PITCH + SINGLE_ROW);
            } else {
                func_0209ae80(obj, b, NUMBER_COLUMN, rank * RANK_ROW_PITCH + FIRST_ROW);
                func_0209ae80(obj, c, NUMBER_COLUMN, rank * RANK_ROW_PITCH + SECOND_ROW);
            }
            score = 0;
            if (data_027e02a8 & MANAGER_READ_FLAG) {
                score = func_0209bc04(*mgr, i);
            }
            /* order[rank] is the slot ranked just above this one. */
            if (rank != 0 && obj->flags[obj->order[rank]] == 0) {
                s32 aboveScore = 0;
                if (data_027e02a8 & MANAGER_READ_FLAG) {
                    aboveScore = func_0209bc04(*mgr, obj->order[rank]);
                }
                if (aboveScore < score) {
                    u8 t;
                    obj->saved[i]                = obj->slotsB[obj->order[rank]]->value;
                    obj->saved[obj->order[rank]] = obj->slotsB[i]->value;
                    obj->flags[i]                = 1;
                    obj->flags[obj->order[rank]] = 1;
                    t                            = obj->order[rank + 1];
                    obj->order[rank + 1]         = obj->order[rank];
                    obj->order[rank]             = t;
                }
            }
        } else {
            s32 delta    = obj->slotsB[i]->value - obj->saved[i];
            s32 distance = delta;
            FxW m;
            s32 step1, step2, step3, step4, step5;
            if (delta < 0) {
                fx_set(&m, -FX_ONE);
                distance = FxMulW(delta, m);
            }
            func_02007f24(&step1, 3.0);
            if (distance < step1) {
                obj->slotsB[i]->value = obj->saved[i];
                obj->slotsA[i]->value = obj->saved[i];
                obj->flags[i]         = 0;
            } else if (delta != distance) {
                func_02007f24(&step2, 3.0);
                obj->slotsB[i]->value += step2;
                func_02007f24(&step3, 3.0);
                obj->slotsA[i]->value += step3;
            } else {
                func_02007f24(&step4, 3.0);
                obj->slotsB[i]->value -= step4;
                func_02007f24(&step5, 3.0);
                obj->slotsA[i]->value -= step5;
            }
        }
    }
}
