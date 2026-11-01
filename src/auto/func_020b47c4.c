#include "m2c.h"

enum {
    NO_ENTRY          = 0xFFFF,
    HEADER_SIZE       = 12,
    MAX_INSTRUMENTS   = 100,
    FIELD30_MAX       = 100,
    MAX_CHANNELS      = 64,
    CARD_ID_CHECK_BIT = 0x800,
};

typedef struct Ent {
    u8 pad0[2];
    u8 a;
    u8 b;
    u8 pad1[0x24 - 4];
} Ent;

typedef struct Chan {
    u8 pad0[8];
    u8 *seq;
    u8 pad1[0x1C - 0x0C];
    u16 f1c;
    u8 pad2[0x22 - 0x1E];
    u16 f22;
    u8 channelCount;
    u8 pad3[0x2A - 0x25];
    u8 f2a;
    u8 f2b;
    u8 f2c;
    u8 f2d;
    u8 seqLength;
    u8 instrumentCount;
    u8 f30;
    u8 f31;
    u8 pad4[0x36 - 0x32];
    u8 f36;
    u8 f37;
    u8 f38;
    u8 pad5[0x40 - 0x39];
    Ent ent[1];
} Chan;

typedef struct Pool {
    Chan *cur;
    u32 pad;
    u8 *base;
    Chan *slots[3];
    u32 used;
    u32 limit;
} Pool;

typedef struct Hdr {
    u8 b[8];
    u16 h;
    u8 channelCount;
    u8 pad;
} Hdr;

extern Pool data_0210daf0;
extern s32 data_0210dbd0;
extern s32 data_027e02a8;
extern u32 data_02101d7c;

extern void func_02054e78(s32);
extern s32 func_0205e540(s32 first, s32 second, s32 value);
extern void func_020b4b30(s32 heap);
extern void func_020b54c8(s32 heap);
extern void func_020b58a8(s32 arg0);
extern void func_020ba778(s32 position, s32 origin);
extern s32 func_020ba798(s32 buffer, s32 size);
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

/* Loads the header of stream entry `entry` into the current record: seeks
   through the u32 offset table at data_0210dbd0 to the entry, reads a 12-byte
   header (8 byte fields, a u16, the channel count), clamps the instrument count
   and field 0x30 to 100 and the channel count to 64, reads two per-channel byte
   lists, then reads seqLength bytes of sequence data into a block taken from the
   heap (heap != 0) or the static arena, and loads instruments (func_020b4b30) and
   the two following tables. Halts if allocation fails/overflows and the card
   ROM ID changed. No-op when entry == NO_ENTRY. */
void func_020b47c4(s32 entry, s32 heap) {
    Hdr hdr;
    u8 chanA[MAX_CHANNELS];
    u8 chanB[MAX_CHANNELS];
    s32 headerOffset;
    s32 i;

    if (entry == NO_ENTRY) {
        return;
    }
    data_0210daf0.cur->f1c = entry;
    func_020ba778(data_0210dbd0 + entry * 4, 0);
    func_020ba798((s32) &headerOffset, 4);
    func_020ba778(headerOffset, 0);
    func_020ba798((s32) &hdr, HEADER_SIZE);
    data_0210daf0.cur->f2a             = hdr.b[0];
    data_0210daf0.cur->f2b             = hdr.b[1];
    data_0210daf0.cur->f2c             = hdr.b[2];
    data_0210daf0.cur->f2d             = hdr.b[3];
    data_0210daf0.cur->seqLength       = hdr.b[4];
    data_0210daf0.cur->instrumentCount = hdr.b[5];
    data_0210daf0.cur->f30             = hdr.b[6];
    data_0210daf0.cur->f31             = hdr.b[7];
    data_0210daf0.cur->f22             = hdr.h;
    data_0210daf0.cur->channelCount    = hdr.channelCount;
    data_0210daf0.cur->f36             = 0x80;
    data_0210daf0.cur->f38             = 0;
    data_0210daf0.cur->f37             = 0;
    if (data_0210daf0.cur->instrumentCount > MAX_INSTRUMENTS) {
        data_0210daf0.cur->instrumentCount = MAX_INSTRUMENTS;
    }
    if (data_0210daf0.cur->f30 > FIELD30_MAX) {
        data_0210daf0.cur->f30 = FIELD30_MAX;
    }
    if (data_0210daf0.cur->channelCount > MAX_CHANNELS) {
        data_0210daf0.cur->channelCount = MAX_CHANNELS;
    }
    func_020ba798((s32) chanA, data_0210daf0.cur->channelCount);
    func_020ba798((s32) chanB, data_0210daf0.cur->channelCount);
    for (i = 0; i < data_0210daf0.cur->channelCount; i++) {
        data_0210daf0.cur->ent[i].a = chanA[i];
        data_0210daf0.cur->ent[i].b = chanB[i];
    }
    if (heap != 0) {
        data_0210daf0.cur->seq = (u8 *) func_0205e540(data_0210daf0.cur->seqLength, 3, 4);
        if (data_0210daf0.cur->seq == NULL) {
            if (rom_id_mismatch()) {
                func_02054e78(0);
                while (1) {
                }
            }
        }
    } else {
        data_0210daf0.cur->seq = data_0210daf0.base + data_0210daf0.used;
        data_0210daf0.used     = data_0210daf0.used + ((data_0210daf0.cur->seqLength + 3) & ~3);
        if (data_0210daf0.used >= data_0210daf0.limit) {
            if (rom_id_mismatch()) {
                func_02054e78(0);
                while (1) {
                }
            }
        }
    }
    data_0210daf0.cur->seq = (u8 *) func_020ba798((s32) data_0210daf0.cur->seq, data_0210daf0.cur->seqLength);
    func_020b4b30(heap);
    func_020b54c8(heap);
    func_020b58a8(heap);
    if (rom_id_mismatch()) {
        func_02054e78(0);
        while (1) {
        }
    }
}
