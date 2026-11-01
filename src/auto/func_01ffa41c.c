#include "m2c.h"

/* Reads the next 32-bit word from a stream cursor and advances the cursor by one word
 * (callers such as func_0205bec8 read successive fields this way). */
s32 func_01ffa41c(s32 **cursor) {
    s32 *p    = *cursor;
    s32 value = *p++;
    *cursor   = p;
    return value;
}
