#include "m2c.h"

typedef struct State2070CA0 {
    s16 unknown0;
    u16 mask;
    s16 unknown4;
    u8 unknown6;
} State2070CA0;

typedef struct Entry2070CA0 {
    u8 name[0x18];
} Entry2070CA0;

typedef struct Table2070CA0 {
    u8 unknown00[0x51];
    Entry2070CA0 entries[1];
} Table2070CA0;

typedef struct Holder2070CA0 {
    Table2070CA0 *table;
} Holder2070CA0;

M2C_UNK func_02053c2c(void *, s32, s32, s32, s32, s32);   /* extern */
M2C_UNK func_02065600(void *, s32);                       /* extern */
M2C_UNK func_0206574c(void *, s32, s32);                  /* extern */
s32 func_02065be8(void *, s32, s32, s32, void *, s32);    /* extern */
s32 func_02073044(void);                                  /* extern */
extern void **data_021092f4;
extern State2070CA0 data_02104d2c;
extern Holder2070CA0 data_02107860;
extern s16 data_020ee008[];

enum {
    SLOT_ARG_FIRST = 0x3d, /* second func_02065be8 argument for slot 0 */
    SLOT_ARG_STEP  = 0x78, /* added per slot index */
};

/*
 * Redraw the per-slot name list on the object at arg0 + 0x5c, but only when the slot bitmap
 * changed (same scheme as func_02072250). The bitmap is the u16 at +0x66 of the object returned by
 * slot 0 of the function table of *data_021092f4; it is cached in data_02104d2c.mask.
 * For each set bit i below func_02073044(), the name record of slot i (*data_02107860 + 0x51 +
 * i * 0x18) is passed to func_02065be8 with its s16 pair from data_020ee008; an empty name clears
 * the cached mask so the next call rebuilds the list. Roles of the setup calls on arg0 + 0x28 /
 * arg0 + 0x5c and of the stepped argument are not established.
 */
void func_02070ca0(void *arg0) {
    u16 slot_arg;
    s32 i;
    u32 mask;

    slot_arg = SLOT_ARG_FIRST;
    mask     = M2C_FIELD(M2C_FIELD(*data_021092f4, void *(**) (void **, s32), 0)(data_021092f4, 0), u16 *, 0x66);
    if (mask == data_02104d2c.mask) {
        return;
    }
    data_02104d2c.mask = mask;
    func_02053c2c((u8 *) arg0 + 0x28, 0, 4, 0x20, 0xd, 0);
    func_02065600((u8 *) arg0 + 0x5c, 3);
    func_0206574c((u8 *) arg0 + 0x5c, 9, -2);
    i = 0;
    if (i < func_02073044()) {
        Holder2070CA0 *holder = &data_02107860;
        do {
            if (mask & (1 << i)) {
                Entry2070CA0 *e = &holder->table->entries[(u8) i];
                if (e->name[0] != 0) {
                    func_02065be8((u8 *) arg0 + 0x5c, slot_arg, data_020ee008[i * 2], data_020ee008[i * 2 + 1], e->name, 0);
                } else {
                    data_02104d2c.mask = 0;
                }
            }
            slot_arg += SLOT_ARG_STEP;
            i++;
        } while (i < func_02073044());
    }
}
