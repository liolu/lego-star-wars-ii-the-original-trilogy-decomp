#include "m2c.h"

enum {
    FIELD_INDEX_OFFSET = 0x84,
    LOW_TWO_BITS_MASK  = 0x3,
};

/* Returns nonzero when the s32 at offset 0x84 is 0 or 1: it builds the
 * bit (1 << value) and keeps only bits 0 and 1. The field's meaning is
 * unknown (single caller func_01ff8abc has no source). */
s32 func_01ff84a8(void *obj) {
    return (1 << M2C_FIELD(obj, s32 *, FIELD_INDEX_OFFSET)) & LOW_TWO_BITS_MASK;
}
