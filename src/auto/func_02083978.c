#include "m2c.h"

typedef struct {
    u8 pad0[5];
    u8 len : 8; // FAKE: full-width bitfield; keeps the -8 record step apart from the +5 field offset until late (plain u8
                // hoists differently)
    u8 pad6[2];
} Rec;

typedef struct {
    u8 pad0[0x2c];
    Rec *recs;
    u8 pad30[0x10];
    u16 count;
} Info;

typedef struct {
    u8 pad0[4];
    Info *info;
} Holder;

typedef struct {
    u8 pad0[0x38c];
    Holder *holder;
} Sys;

typedef struct {
    u8 pad0[0x68];
    Sys *sys;
} Global;

typedef struct {
    u8 pad0[0x54];
    u8 **bufs;
} Self;

extern Global data_0210930c;
extern void *func_0205e818(s32, s32, s32);

static inline s32 GetLen(u32 id) {
    Info *info = data_0210930c.sys->holder->info;
    return id != 0 ? (info->recs + id)[-1].len : 0;
}

enum {
    FILL_BYTE = 0xff
};

/* Builds one byte buffer per record of the current info table (count at
 * info+0x40, 8-byte records at info+0x2c, ids 1-based). self->bufs gets an
 * array of count pointers; each record with a nonzero len gets a len-byte
 * buffer filled with FILL_BYTE, a zero len leaves a NULL entry. A zero count
 * leaves self->bufs NULL. */
void func_02083978(Self *self) {
    s32 n = data_0210930c.sys->holder->info->count;
    s32 i;
    if (n == 0) {
        self->bufs = NULL;
        return;
    }
    self->bufs = func_0205e818(n * 4, 3, 4);
    for (i = 0; i < n; i++) {
        s32 len = GetLen(i + 1);
        s32 j;
        if (len == 0) {
            self->bufs[i] = NULL;
            continue;
        }
        self->bufs[i] = func_0205e818(len, 3, 4);
        for (j = 0; j < len; j++) {
            self->bufs[i][j] = FILL_BYTE;
        }
    }
}
