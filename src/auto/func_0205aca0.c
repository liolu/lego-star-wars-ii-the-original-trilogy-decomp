#include "m2c.h"

typedef struct FSFile FSFile;

extern void func_02054e78(s32);
extern u32 func_02059efc(void *src, void *dst);
extern s32 func_0205abb4(u16 id, s32 *offset, s32 *size);
typedef struct MapNode {
    u8 links[0x10];
    void **value;
} MapNode;
typedef struct Map {
    s32 count;
    MapNode head;
} Map;
extern void func_0205af6c(MapNode **out, Map *map, u16 *key);
extern void func_0205b01c(Map *a0, u16 a1, s32 a2, s32 a3, void *a4);
extern void func_0205b1e0(Map *map, void **value, s32 a2);
extern void *func_0205e540(s32 first, s32 second, s32 value);
extern s32 func_0205e5b0(void *arg0);
extern void MIi_LockCard(void);
extern void MIi_UnlockCard(void);
extern void func_020d2cbc(void *file, void *buf, s32 size);
extern s32 FS_SeekFile(void *p_file, s32 offset, s32 origin);
extern u32 CARDi_ReadRomID(void);

extern u32 data_02101d7c;
extern Map *data_021027f4;
extern s32 data_027e02a4;
extern s32 data_027e02a8;
extern M2C_UNK data_027e0340;

enum {
    ROM_ID_CHECK_BIT  = 0x800,   /* data_027e02a8 bit: verify the card ROM ID */
    FILE_ACTIVE_BYTE  = 0x49,    /* byte in data_027e0340 set around file I/O */
    SEEK_FROM_START   = 0,       /* FS_SeekFile origin */
    PACKED_SIZE_SHIFT = 8,      /* packed header: unpacked size in bits 8..31 */
};

static inline s32 rom_id_mismatch(void) {
    u32 id;

    if ((data_027e02a8 & ROM_ID_CHECK_BIT) != 0) {
        MIi_LockCard();
        id = CARDi_ReadRomID();
        MIi_UnlockCard();
        if (id != data_02101d7c) {
            return 1;
        }
    }
    return 0;
}

static inline void toggle_flag(void) {
    if (data_027e02a4 == 0) {
        data_027e02a4 = 1;
    } else {
        data_027e02a4 = 0;
    }
}

/* Returns the data for resource `id`, loading it on a cache miss.
   The cache map (data_021027f4) is searched for id; on a hit the entry is
   passed to func_0205b1e0 with arg1 and its stored data pointer returned.
   On a miss func_0205abb4 gives the resource's offset and size, the shared
   file data_027e0340 is seeked there and `size` bytes are read into a
   buffer from func_0205e540. When `packed` is set the buffer is unpacked by
   func_02059efc into a second buffer sized from its header, the first
   buffer is released, and data_027e02a4 is toggled around the allocation.
   The result is inserted into the cache with id, arg1 and packed. Returns
   NULL when the lookup, seek or allocation fails; halts (func_02054e78)
   if the card ROM ID check fails after a failure or after the read. */
void *func_0205aca0(u16 id, s32 arg1, s32 packed) {
    s32 offset;
    s32 size;
    MapNode *it;
    u16 key;
    Map *cache;
    MapNode *entry;
    void *data;
    void *unpacked;

    cache = data_021027f4;
    key   = id;
    func_0205af6c(&it, cache, &key);
    data = it;
    if (data == &cache->head) {
        data = NULL;
    } else {
        entry = data;
        func_0205b1e0(cache, entry->value, arg1);
        data = *entry->value;
    }
    if (data == NULL) {
        if (func_0205abb4(id, &offset, &size) == 0) {
            if (rom_id_mismatch()) {
                func_02054e78(0);
                for (;;) {
                }
            }
            return NULL;
        }
        M2C_FIELD(&data_027e0340, s8 *, FILE_ACTIVE_BYTE) = 1;
        if (FS_SeekFile(&data_027e0340, offset, SEEK_FROM_START) == 0) {
            if (rom_id_mismatch()) {
                func_02054e78(0);
                for (;;) {
                }
            }
            return NULL;
        }
        if (packed != 0) {
            toggle_flag();
        }
        data = func_0205e540(size, 3, 4);
        if (packed != 0) {
            toggle_flag();
        }
        if (data != NULL) {
            func_020d2cbc(&data_027e0340, data, size);
            if (rom_id_mismatch()) {
                func_02054e78(0);
                for (;;) {
                }
            }
            if (packed != 0) {
                unpacked = func_0205e540(*(u32 *) data >> PACKED_SIZE_SHIFT, 3, 4);
                func_02059efc(data, unpacked);
                func_0205e5b0(data);
                data = unpacked;
            }
            M2C_FIELD(&data_027e0340, s8 *, FILE_ACTIVE_BYTE) = 0;
            func_0205b01c(data_021027f4, id, arg1, packed, data);
        } else if (rom_id_mismatch()) {
            func_02054e78(0);
            for (;;) {
            }
        }
    }
    return data;
}
