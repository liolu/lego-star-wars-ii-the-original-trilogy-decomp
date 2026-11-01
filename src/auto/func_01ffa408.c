#include "m2c.h"

/* Reads the next halfword from *cursor and advances the cursor past it. */
u16 func_01ffa408(u16 **cursor) {
    u16 *p    = *cursor;
    u16 value = *p++;
    *cursor   = p;
    return value;
}
