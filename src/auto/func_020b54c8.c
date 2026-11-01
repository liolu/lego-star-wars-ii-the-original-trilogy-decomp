#include "m2c.h"

enum {
    NO_ENTRY          = 0xFFFF,
    CARD_ID_CHECK_BIT = 0x800,
    HDR_DATA_TABLE    = 4,  /* data_0210dbd0[] word: file offset of per-record data offset table */
    HDR_RECORD_TABLE  = 12, /* data_0210dbd0[] word: file offset of the Record array */
};

typedef struct Entry {
    void *data;
    u8 b4;
    u8 b5;
    u8 b6;
    u8 b7;
    s32 count;
    s32 fc;
    s32 f10;
    s32 f14;
    s32 f18;
    s32 f1c;
    u8 b20;
    u8 b21;
    u8 b22;
    u8 b23;
} Entry;

typedef struct Record {
    u16 id;
    u16 pad;
    u8 b4;
    u8 b5;
    u8 b6;
    u8 b7;
    s32 count;
    s32 fc;
    s32 f10;
    s32 f14;
    s32 f18;
    s32 f1c;
    u8 b20;
    u8 b21;
    u8 b22;
    u8 b23;
} Record;

typedef struct Obj {
    Entry *entries;
    u8 pad[0x30 - 4];
    u8 count;
} Obj;

typedef struct Pool {
    Obj *cur;
    u32 pad;
    u8 *base;
    u32 pad2[3];
    u32 used;
    u32 limit;
} Pool;

extern Pool data_0210daf0;
extern s32 data_027e02a8;
extern u32 data_02101d7c;
extern s32 data_0210dbd0[];

extern void func_02054e78(s32);
extern s32 func_0205e540(s32 first, s32 second, s32 value);
extern void *func_0205e624(void *dst, u32 value, u32 size);
extern void func_020ba778(M2C_UNK arg0, M2C_UNK arg1);
extern s32 func_020ba798(s32 arg0, M2C_UNK arg1);
extern u32 func_020ba7c8(void);
extern void MIi_LockCard(void);
extern void MIi_UnlockCard(void);
extern u32 CARDi_ReadRomID(void);

static inline s32 rom_id_mismatch(void) {
    u32 id;

    if ((data_027e02a8 & CARD_ID_CHECK_BIT) != 0) {
        MIi_LockCard();
        id = CARDi_ReadRomID();
        MIi_UnlockCard();
        if (id != data_02101d7c) {
            return 1;
        }
    }
    return 0;
}

/* Loads the entry table of the current object (data_0210daf0.cur) from the
   open archive file: reads cur->count u16 record ids at the current file
   position, then allocates cur->count + 1 Entry slots (heap when dynamic != 0,
   else from the static pool), zeroes them and fills slots 1..count. For each
   id (NO_ENTRY leaves the slot empty) it seeks to that Record, copies its
   fields, seeks via the data offset table to the entry's data and reads
   count words into a newly allocated buffer. Halts if allocation fails or
   the pool overflows and the card ROM ID check differs. Restores the file
   position on exit. */
void func_020b54c8(s32 dynamic) {
    u32 savedPos;
    s32 fileOfs;
    Record rec;
    u16 ids[100];
    s32 i;
    Entry *entry;

    func_020ba798((s32) ids, data_0210daf0.cur->count * 2);
    savedPos = func_020ba7c8();
    if (dynamic != 0) {
        data_0210daf0.cur->entries = (Entry *) func_0205e540((data_0210daf0.cur->count + 1) * sizeof(Entry), 3, 4);
        if (data_0210daf0.cur->entries == NULL) {
            if (rom_id_mismatch()) {
                func_02054e78(0);
                for (;;) {
                }
            }
        }
    } else {
        data_0210daf0.cur->entries = (Entry *) (data_0210daf0.base + data_0210daf0.used);
        data_0210daf0.used += ((data_0210daf0.cur->count + 1) * sizeof(Entry) + 3) & ~3;
        if (data_0210daf0.used >= data_0210daf0.limit) {
            if (rom_id_mismatch()) {
                func_02054e78(0);
                for (;;) {
                }
            }
        }
    }
    func_0205e624(data_0210daf0.cur->entries, 0, (data_0210daf0.cur->count + 1) * sizeof(Entry));

    for (i = 1; i < data_0210daf0.cur->count + 1; i++) {
        entry = &data_0210daf0.cur->entries[i];
        if (ids[i - 1] == NO_ENTRY) {
            entry->b4    = 0;
            entry->b5    = 0;
            entry->count = 0;
            entry->data  = NULL;
        } else {
            fileOfs = data_0210dbd0[HDR_RECORD_TABLE] + ids[i - 1] * sizeof(Record);
            func_020ba778(fileOfs, 0);
            func_020ba798((s32) &rec, sizeof(Record));
            entry->b4    = rec.b4;
            entry->b5    = rec.b5;
            entry->b6    = rec.b6;
            entry->b7    = rec.b7;
            entry->count = rec.count;
            entry->fc    = rec.fc;
            entry->f10   = rec.f10;
            entry->f14   = rec.f14;
            entry->f18   = rec.f18;
            entry->f1c   = rec.f1c;
            entry->b20   = rec.b20;
            entry->b21   = rec.b21;
            entry->b22   = rec.b22;
            entry->b23   = rec.b23;
            func_020ba778(data_0210dbd0[HDR_DATA_TABLE] + rec.id * 4, 0);
            func_020ba798((s32) &fileOfs, 4);
            func_020ba778(fileOfs, 0);
            if (dynamic != 0) {
                entry->data = (void *) func_0205e540(entry->count * 4, 3, 4);
                if (entry->data == NULL) {
                    if (rom_id_mismatch()) {
                        func_02054e78(0);
                        for (;;) {
                        }
                    }
                }
            } else {
                entry->data = data_0210daf0.base + data_0210daf0.used;
                data_0210daf0.used += (entry->count * 4 + 3) & ~3;
                if (data_0210daf0.used >= data_0210daf0.limit) {
                    if (rom_id_mismatch()) {
                        func_02054e78(0);
                        for (;;) {
                        }
                    }
                }
            }
            entry->data = (void *) func_020ba798((s32) entry->data, entry->count * 4);
        }
    }
    func_020ba778(savedPos, 0);
}
