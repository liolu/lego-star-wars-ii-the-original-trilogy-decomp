#include "m2c.h"

typedef struct {
    u8 language;
    u8 favoriteColour;
    u8 birthday[2];
    u16 nickName[10];
    u16 nickNameLength;
    u16 comment[26];
    u16 commentLength;
} OSOwnerInfo;

typedef struct {
    u8 pad0[2];
    u16 mask;  /* 0x2: cached slot bitmap */
    u8 pad4[2];
    u8 count;  /* 0x6: number of set bits seen on the last rebuild */
} State;

typedef struct {
    u8 pad[0x66];
    u16 mask;  /* 0x66: slot bitmap */
} Source;

typedef struct Provider {
    Source *(**vtbl)(struct Provider *, s32);
} Provider;

typedef struct {
    u8 pad[0x51];
    u8 names[1][0x18];  /* 0x51: one 0x18-byte name record per slot */
} Table;

extern void OS_GetOwnerInfo(OSOwnerInfo *info);
extern void func_02053c2c(void *a, s32 b, s32 c, s32 d, s32 e, s32 f);
extern s32 func_02073044(void);
extern void func_02065be8(void *a, s32 b, s32 c, s32 d, void *e, void *f);

extern State data_02104d2c;
extern Provider *data_021092f4;
extern Table *data_02107860;
extern const s16 data_020ee008[][2];

enum {
    SLOT_ARG_STEP = 0x78, /* added to the second func_02065be8 argument per slot index */
};

/*
 * Rebuild the per-slot name list on self + 0x5c when the slot bitmap changed, and return the
 * number of set bits. Same scheme as func_02070ca0. The bitmap is the u16 at +0x66 of the object
 * returned by slot 0 of the function table of *data_021092f4; it is cached in data_02104d2c.mask.
 * When it equals the cache, the count cached in data_02104d2c.count is returned unchanged.
 * Otherwise func_02053c2c fills a block on self + 0x28 with 0, and for each set bit i below
 * func_02073044() the name record of slot i (*data_02107860 + 0x51 + i * 0x18) is passed to
 * func_02065be8 with its s16 pair from data_020ee008; an empty name clears the cached mask so the
 * next call rebuilds the list. Every set bit is counted (empty names included), and the count is
 * stored in data_02104d2c.count, which the caller func_0206f848 reads after this call.
 * The owner info read by OS_GetOwnerInfo is not used. The roles of the stepped argument (it
 * starts at the low half of self + 0x98) and of the setup on self + 0x28 are not established.
 */
u8 func_0206fbf4(void *self) {
    OSOwnerInfo owner;
    u16 slot_arg;
    u8 count;
    s32 i;
    u32 mask;
    Table **table;

    OS_GetOwnerInfo(&owner);
    slot_arg = *(s32 *) ((u8 *) self + 0x98);
    mask     = data_021092f4->vtbl[0](data_021092f4, 0)->mask;
    if (mask == data_02104d2c.mask) {
        return data_02104d2c.count;
    }
    data_02104d2c.mask = mask;
    func_02053c2c((u8 *) self + 0x28, 0, 4, 0x20, 0xd, 0);
    count = 0;
    i     = 0;
    if (func_02073044() > 0) {
        table = &data_02107860;
        do {
            if (mask & (1 << i)) {
                u8 *name = (*table)->names[(u8) i];
                if (*name != 0) {
                    func_02065be8((u8 *) self + 0x5c, slot_arg, data_020ee008[i][0], data_020ee008[i][1], name, 0);
                } else {
                    data_02104d2c.mask = 0;
                }
                count++;
            }
            slot_arg += SLOT_ARG_STEP;
            i++;
        } while (i < func_02073044());
    }
    return data_02104d2c.count = count;
}
