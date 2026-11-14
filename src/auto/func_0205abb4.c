#include "m2c.h"

extern s32 FS_SeekFile(M2C_UNK *, s32, s32);
extern s32 func_020d2cbc(M2C_UNK *, M2C_UNK *, s32);
extern void func_02054e78(s32);
extern void MIi_LockCard(void);
extern void MIi_UnlockCard(void);
extern u32 CARDi_ReadRomID(void);
extern M2C_UNK data_027e0340;
extern s32 data_0210270c[];
extern s32 data_02102714[];
extern u32 data_027e02a8;
extern u32 data_02101d7c;

enum {
    ROM_ID_CHECK_BIT = 0x800, /* data_027e02a8 bit: verify the card ROM ID */
    FILE_ACTIVE_BYTE = 0x49,  /* byte in data_027e0340 set around file I/O */
    SEEK_FROM_START  = 0,     /* FS_SeekFile origin */
    TABLE_ENTRY_SIZE = 4,     /* bytes per offset-table word */
    TABLE_READ_SIZE  = 8,     /* this entry's word and the next one */
};

// FAKE: data_0210270c spelled as data_02102714 - 2 words (the next symbol, 8 bytes on); forces a second,
// separate literal-pool word for the read-buffer argument (the target pools data_0210270c twice; mwcc merges
// two uses of the same symbol into one word)
static inline s32 *header_base(void) {
    return data_02102714 - 2;
}

static inline s32 card_id_mismatch(void) {
    u32 id;
    if (data_027e02a8 & ROM_ID_CHECK_BIT) {
        MIi_LockCard();
        id = CARDi_ReadRomID();
        MIi_UnlockCard();
        if (id != data_02101d7c) {
            return 1;
        }
    }
    return 0;
}

/* Looks up resource `idx` in the offset table of the shared file data_027e0340:
   seeks to idx * 4, reads two consecutive words into data_0210270c, and returns
   the first as the offset and the difference as the size. Returns 0 if the seek
   or the 8-byte read fails, else 1. If the offset is -1 and the card ROM ID
   check fails, halts in func_02054e78. */
s32 func_0205abb4(s32 idx, s32 *out_off, s32 *out_size) {
    M2C_FIELD(&data_027e0340, s8 *, FILE_ACTIVE_BYTE) = 1;
    if (FS_SeekFile(&data_027e0340, idx * TABLE_ENTRY_SIZE, SEEK_FROM_START) == 0) {
        return 0;
    }
    if (func_020d2cbc(&data_027e0340, (M2C_UNK *) header_base(), TABLE_READ_SIZE) != TABLE_READ_SIZE) {
        return 0;
    }
    M2C_FIELD(&data_027e0340, s8 *, FILE_ACTIVE_BYTE) = 0;
    *out_off                                          = data_0210270c[0];
    *out_size                                         = data_0210270c[1] - *out_off;
    if (*out_off == -1) {
        if (card_id_mismatch() != 0) {
            func_02054e78(0);
            for (;;) {
            }
        }
    }
    return 1;
}
