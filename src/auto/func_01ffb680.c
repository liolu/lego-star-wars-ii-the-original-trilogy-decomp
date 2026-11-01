#include "m2c.h"

typedef struct Unk01ffb680 Unk01ffb680;

struct Unk01ffb680 {
    u8 pad0[0x8];
    u32 flags;                              /* 0x8: bit 0x20 set by this handler */
    u8 padC[0x4];
    void (*callback)(Unk01ffb680 *self);    /* 0x10: optional, gets the record */
};

enum {
    HANDLER_FLAG = 0x20,
};

/*
 * Pointer-table handler (slot 1 of data_02101498, between func_01ffb658 and
 * NNSi_G3dFuncSbc_NODE): runs the record's optional callback, passing the
 * record itself, then sets flag bit 0x20 whether or not a callback was set.
 */
void func_01ffb680(Unk01ffb680 *arg0) {
    if (arg0->callback != NULL) {
        arg0->callback(arg0);
    }
    arg0->flags |= HANDLER_FLAG;
}
