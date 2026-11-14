#include "m2c.h"

typedef struct Node {
    void *data;
    struct Node *next;
} Node;

typedef struct Item {
    u8 pad00[0x20];
    u8 f20;
    u8 f21;
    u8 pad22[2];
    u32 f24;
    u32 flags28;
} Item;

typedef struct Level {
    u8 pad000[0x394];
    Node *objs;
} Level;

typedef struct Game {
    u8 pad00[4];
    Node *items;
    u8 pad08[0x68 - 0x08];
    Level *level;
} Game;

typedef struct Room {
    u8 pad00[4];
    Node *objs;
    u8 pad08[0x38 - 0x08];
} Room;

typedef struct Src8 {
    u32 w;
    u8 b[4];
} Src8;

typedef struct Mark {
    u8 a;
    u8 b;
    u8 pad[2];
    u32 val;
} Mark;

typedef struct Slot {
    u16 f0;
    u16 f2;
    u16 f4;
    u16 f6;
    u8 f8;
    u8 f9;
} Slot;

typedef struct Rec {
    u32 w;
    u8 b[4];
} Rec;

typedef struct State {
    u8 pad0000[0x22c0];
    Mark marks[(0x2860 - 0x22c0) / 8];
    s32 f2860;
    s32 f2864;
    s32 f2868;
    u8 f286c;
    u8 pad286d[3];
    u32 bits[2];
    u8 flags[57];
    u8 vals[18];
    u8 pad28c3[1];
    Slot slots[35];
    u8 pad2a22[6];
    Rec recs[8];
} State;

extern Game data_0210930c;
extern u8 data_02104d90[];
extern u8 data_02104d94[];
extern s32 data_027e02a4;
extern Room data_021019f4[];
extern s32 data_02109c70;
extern s32 data_02101ad4;

extern void func_02001688(s32 arg0);
extern void func_020016c0(void);
extern s32 func_0208559c(void *arg0);
extern s32 func_02086204(void *arg0, s32 arg1);
extern u8 func_02086244(void *arg0, s32 arg1);
extern s32 func_020867ec(void *arg0, s32 arg1);
extern s32 func_02088eb4(State *arg0, s32 arg1);
extern s32 func_02088f00(State *arg0);
extern void func_02088f44(State *arg0, s32 arg1);
extern void func_02089078(State *arg0, s32 arg1);
extern s32 func_0208926c(State *arg0);
extern s32 func_0208933c(State *arg0, s32 arg1);
extern void func_02089388(State *arg0, s32 arg1);
extern s8 func_0208960c(State *arg0, s32 arg1);
extern void func_02089648(State *arg0, s32 arg1);
extern void func_020896c4(State *arg0, u16 *count, void *obj);
extern Src8 *func_0209084c(Level *arg0, s32 arg1);

static inline void set_bit(u32 *bits, s32 n, s32 v) {
    bits[n >> 5] &= ~(1 << (n & 31));
    bits[n >> 5] |= v << (n & 31);
}

static inline void add_mark(State *st, Item *it, u16 *count) {
    if ((it->flags28 & 1) != 1) {
        u16 k = *count;
        Mark *m;
        Mark *marks = st->marks;
        // FAKE: same element spelled two ways (st->marks[k] and marks[k]); forces the val store through a
        // separate st+0x2000 base and the a/b byte stores through a second marks-array base register.
        st->marks[k].val = it->f24;
        m                = &marks[k];
        m->a             = it->f20;
        m->b             = it->f21;
        (*count)++;
    }
}

static inline void visit(State *st, u16 *count, Node *n) {
    // FAKE: always-true null test (callers loop while n != NULL); forces the second cmp/beq inside both
    // object-list loops. Possibly an inlined helper in the original.
    if (n != NULL) {
        func_020896c4(st, count, n->data);
    }
}

/* Loop counts; each equals the length of the State array it fills. */
enum {
    REC_COUNT  = 8,
    FLAG_COUNT = 57,
    VAL_COUNT  = 18
};

/*
 * Fills the state block for range `idx` (the block func_020887fc initialises; func_02088f44
 * shifts the 8-byte marks at 0x22c0 between ranges): eight recs from the level, the range's
 * mode byte, 57 bits, 57 flags and 18 values read from data_02104d90, the room and level
 * object lists through func_020896c4 (appends at *count), then one mark per item with bit 0 clear.
 */
void func_02088b84(State *st, u8 mode) {
    s32 idx;
    u8 i;
    u16 j;
    Node *n;
    Node *item;
    Room *room;
    u16 count;

    func_02001688(0);
    func_02089648(st, mode);
    idx = func_0208960c(st, mode);
    if (data_0210930c.level != NULL) {
        i = 0;
        do {
            Src8 *src        = func_0209084c(data_0210930c.level, i);
            st->recs[i].w    = src->w;
            st->recs[i].b[0] = src->b[0];
            st->recs[i].b[1] = src->b[1];
            st->recs[i].b[2] = src->b[2];
            st->recs[i].b[3] = src->b[3];
            i++;
        } while (i < REC_COUNT);
    }
    st->slots[idx].f8 = mode;
    st->slots[idx].f9 = 1;
    func_02089388(st, idx);
    for (j = 0; j < FLAG_COUNT; j++) {
        set_bit(st->bits, j, func_02086204(data_02104d90, j));
    }
    for (j = 0; j < FLAG_COUNT; j++) {
        st->flags[j] = func_02086244(data_02104d90, j);
    }
    for (j = 0; j < VAL_COUNT; j++) {
        st->vals[j] = func_020867ec(data_02104d90, j);
    }
    st->f2868         = func_0208559c(data_02104d94);
    count             = 0; // FAKE: dead store, forces the mov #0 / strh to the count stack slot before the call
    count             = func_0208933c(st, idx);
    st->slots[idx].f2 = count + func_0208926c(st);
    func_02089078(st, idx);
    room = &data_021019f4[data_027e02a4];
    for (n = room->objs; n != NULL; n = n->next) {
        visit(st, &count, n);
    }
    for (n = data_0210930c.level->objs; n != NULL; n = n->next) {
        visit(st, &count, n);
    }
    count             = func_02088eb4(st, idx);
    st->slots[idx].f6 = count + func_02088f00(st);
    func_02088f44(st, idx);
    for (item = data_0210930c.items; item != NULL; item = item->next) {
        add_mark(st, item->data, &count);
    }
    st->f2864 = data_02109c70;
    st->f2860 = data_02101ad4;
    st->f286c = mode;
    func_020016c0();
}
