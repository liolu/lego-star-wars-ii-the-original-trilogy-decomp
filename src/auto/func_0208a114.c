#include "m2c.h"

/* Slot table size, end marker and byte offsets into Ctx used by the raw slot/range accesses below. */
enum {
    SLOT_COUNT      = 180,    /* slots[] / seen[] length */
    SLOT_VALUE_END  = -1,     /* slot value that ends a range scan */
    SLOT_SIZE       = 8,
    SLOT_KEY_A_OFS  = 0x22c0, /* slots[0].key_a */
    SLOT_KEY_B_OFS  = 0x22c1, /* slots[0].key_b */
    SLOT_VALUE_OFS  = 0x22c4, /* slots[0].value */
    ZONE_SIZE       = 10,
    RANGE_FIRST_OFS = 0x28c8,
    RANGE_END_OFS   = 0x28ca
};

typedef struct Slot {
    u8 key_a;
    u8 key_b;
    u8 pad[2];
    s32 value;
} Slot;

typedef struct Ctx {
    u8 pad0[0x22c0];
    Slot slots[SLOT_COUNT];
    s32 unk2860;
    s32 sel;
} Ctx;

typedef struct Entry {
    u8 pad[0x20];
    u8 key_a;
    u8 key_b;
    u8 pad2[2];
    s32 value;
} Entry;

typedef struct Node {
    Entry *entry;
    struct Node *next;
} Node;

typedef struct Inner2 {
    u8 pad[4];
    u8 *buf;
} Inner2;

typedef struct Inner1 {
    u8 pad[0x38c];
    Inner2 *sub;
} Inner1;

typedef struct Registry {
    s32 unk0;
    Node *head;
    u8 pad[0x60];
    Inner1 *ctx;
} Registry;

extern Registry data_0210930c;
extern s32 data_02109c70;

void *func_0205e624(void *dst, u32 value, u32 size);
void func_02067624(void *arg0, s32 arg1, u8 arg2);
void func_0209678c(void *list, void *item);

/* Copy *pv into the first list entry whose (key_a, key_b) equals the slot keys at row. */
static inline void apply_to_entry(u8 *row, s32 *pv) {
    Node *node;
    for (node = data_0210930c.head; node != NULL; node = node->next) {
        Entry *e = node->entry;
        if (e->key_a == row[SLOT_KEY_A_OFS] && e->key_b == row[SLOT_KEY_B_OFS]) {
            e->value = *pv;
            break;
        }
    }
}

/*
 * Called by func_02089e60 for a valid, not busy, active zone idx.
 * Clears seen[] (one byte per slot). For each node of data_0210930c.head: walk
 * the slots of the zone range [first, end) read at RANGE_FIRST_OFS/RANGE_END_OFS,
 * stopping at a slot whose value is SLOT_VALUE_END; mark each walked slot in
 * seen[]; on a (key_a, key_b) match copy the slot value into the entry and stop.
 * An entry with no match is passed to func_0209678c with &data_0210930c.
 * Then for every slot of the range not seen, with key_a != 0xff and key_b != 0:
 * call func_02067624 and copy the slot value into the first matching entry.
 * Finally publish ctx->sel to data_02109c70.
 */
void func_0208a114(Ctx *ctx, s32 idx) {
    u8 seen[SLOT_COUNT];
    Node *node;
    Entry *e;
    u16 i;
    s32 found;
    s32 j;
    u8 *base = (u8 *) ctx; // FAKE: byte view of ctx; keeps ctx + j * 8 in one register across func_02067624

    func_0205e624(seen, 0, sizeof(seen));
    if (idx != -1) {
        for (node = data_0210930c.head; node != NULL; node = node->next) {
            e     = node->entry;
            found = 0;
            for (i = *(u16 *) (base + idx * ZONE_SIZE + RANGE_FIRST_OFS);
                 i < *(s16 *) (base + idx * ZONE_SIZE + RANGE_END_OFS); i++) {
                // FAKE: slots[i].value read by byte offset while the keys use the struct; stops the ctx+0x22c4 hoist
                if (*(s32 *) (base + i * SLOT_SIZE + SLOT_VALUE_OFS) == SLOT_VALUE_END) {
                    break;
                }
                seen[i] = 1;
                if (e->key_a == ctx->slots[i].key_a && e->key_b == ctx->slots[i].key_b) {
                    e->value = *(s32 *) (base + i * SLOT_SIZE + SLOT_VALUE_OFS); // FAKE: byte-offset slots[i].value
                    found    = 1;
                    break;
                }
            }
            if (!found) {
                func_0209678c(&data_0210930c, e);
            }
        }
    }
    // FAKE: s32 j stepped as j = (u16)(j + 1) (a u16 counter in an int); a u16 j swaps the j/end-pointer registers
    for (j = *(u16 *) (base + idx * ZONE_SIZE + RANGE_FIRST_OFS); j < *(s16 *) (base + idx * ZONE_SIZE + RANGE_END_OFS);
         j = (u16) (j + 1)) {
        if (*(s32 *) (base + j * SLOT_SIZE + SLOT_VALUE_OFS) == SLOT_VALUE_END) { // FAKE: byte-offset slots[j].value
            break;
        }
        if (seen[j] == 0 && ctx->slots[j].key_a != 0xff && ctx->slots[j].key_b != 0) {
            func_02067624(data_0210930c.ctx->sub->buf + 0x28, ctx->slots[j].key_a, ctx->slots[j].key_b);
            // FAKE: slot row and value passed as byte offsets (keys above use the struct); fixes the r5/r6 swap
            apply_to_entry(base + j * SLOT_SIZE, (s32 *) (base + j * SLOT_SIZE + SLOT_VALUE_OFS));
        }
    }
    data_02109c70 = ctx->sel;
}
