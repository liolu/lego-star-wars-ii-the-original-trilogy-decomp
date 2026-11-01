#include "m2c.h"

typedef struct {
    s32 x, y, z;
} Vec3;

typedef struct {
    Vec3 pos;
    s32 id;
    u8 key0;
    u8 key1;
    u8 mode;
    u8 arg;
    u32 pad;
} Entry;

typedef struct {
    u8 pad0[0x18];
    s32 order;
    u8 pad1c[0x40 - 0x1c];
} Slot;

typedef struct {
    u16 first;
    s16 end;
    u8 pad4[4];
    s8 id;
    u8 active;
} Zone;

typedef struct {
    Entry entries[360];
    Slot slots[4];
    u8 pad22c0[0x2860 - 0x22c0];
    s32 counter;
    u8 pad2864[4];
    s32 f_2868;
    u8 pad286c[0x28c4 - 0x286c];
    Zone zones[7];
    s8 cur;
    u8 pad290b[0x2a68 - 0x290b];
    u8 busy;
} Self;

typedef struct Node {
    s32 value;
    struct Node *next;
} Node;

typedef struct {
    u8 pad0[0x394];
    Node *list;
} Level;

typedef struct {
    u8 pad0[0x1b4];
    s32 order;
} Ent;

typedef struct {
    u8 pad0[0x4c];
    Ent *ents[4];
    u8 pad5c[0x68 - 0x5c];
    Level *level;
} Globals;

typedef struct {
    u8 pad0[4];
    Node *list;
    u8 pad8[0x30];
} Row;

extern Globals data_0210930c;
extern M2C_UNK data_02104d94;
extern s32 data_027e02a4;
extern Row data_021019f4[];
extern s32 data_02101ad4;

void func_02001688(s32 arg0);
void func_020016c0(void);
void *func_0205e624(void *dst, u32 value, u32 size);
void func_02085638(void *arg0, s32 arg1);
void func_0208a114(Self *self, u8 zone);
void func_0208a31c(Self *self, u8 player, s32 zone);
s8 func_0208a5b4(Self *self);
void func_0208a60c(Self *self, s32 index);
void func_0208a738(Self *self, s32 zone, s32 value, u8 *seen);

/* Byte offsets into Self used by the raw slot/zone accesses below. */
enum {
    SLOT_SIZE      = 0x40,         /* sizeof(Slot) */
    SLOT_ORDER_OFS = 0x21d8,  /* slots[0].order */
    ZONE_SIZE      = 10,           /* sizeof(Zone) */
    ZONE_FIRST_OFS = 0x28c4,  /* zones[0].first */
    ZONE_END_OFS   = 0x28c6     /* zones[0].end */
};

/*
 * Per-frame update, bracketed by func_02001688(0) / func_020016c0().
 * With no current zone (cur == -1) only slots 0 and 1 are refreshed with
 * zone -1. Otherwise: pick a zone (func_0208a5b4) and, if it is valid, not
 * busy and active, enter it (func_0208a114); refresh all 4 slots; give each
 * slot that has an entity an order number from self->counter the first time
 * (order == -1) and copy it to the entity. If the zone is still usable, mark
 * the entries reached from the two node lists in seen[] (func_0208a738) and
 * call func_0208a60c for every entry index in the zone's [first, end) range
 * that was not seen; otherwise clear busy. Finally publish self->counter to
 * data_02101ad4.
 */
void func_02089e60(Self *self) {
    u8 seen[0x168];
    Node *node;
    s8 zone;
    u8 i;
    u16 k;

    if (self->cur == -1) {
        func_0208a31c(self, 0, -1);
        func_0208a31c(self, 1, -1);
        return;
    }
    func_02001688(0);
    zone = func_0208a5b4(self);
    if (zone != -1 && self->busy == 0 && self->zones[zone].active != 0) {
        func_0208a114(self, zone);
    }
    func_02085638(&data_02104d94, self->f_2868);
    for (i = 0; i < 4; i++) {
        func_0208a31c(self, i, zone);
    }
    for (i = 0; i < 4; i++) {
        if (data_0210930c.ents[i] != NULL) {
            if (*(s32 *) ((u8 *) self + (s32) i * SLOT_SIZE + SLOT_ORDER_OFS) == -1) {
                *(s32 *) ((u8 *) self + (s32) i * SLOT_SIZE + SLOT_ORDER_OFS) = self->counter;
                self->counter++;
            }
            data_0210930c.ents[i]->order = *(s32 *) ((u8 *) self + (s32) i * SLOT_SIZE + SLOT_ORDER_OFS);
        }
    }
    if (zone == -1 || self->busy != 0 || self->zones[zone].active == 0) {
        func_020016c0();
        self->busy = 0;
        return;
    }
    func_0205e624(seen, 0, sizeof(seen));
    if (zone != -1) {
        for (node = *(Node **) ((u8 *) &data_021019f4[data_027e02a4] + 4); node != NULL; node = node->next) {
            func_0208a738(self, zone, node->value, seen);
        }
        for (node = data_0210930c.level->list; node != NULL; node = node->next) {
            func_0208a738(self, zone, node->value, seen);
        }
        for (k = *(u16 *) ((u8 *) self + (s32) zone * ZONE_SIZE + ZONE_FIRST_OFS);
             k < *(s16 *) ((u8 *) self + (s32) zone * ZONE_SIZE + ZONE_END_OFS); k++) {
            if (seen[k] == 0) {
                func_0208a60c(self, k);
            }
        }
    }
    data_02101ad4 = self->counter;
    func_020016c0();
}
