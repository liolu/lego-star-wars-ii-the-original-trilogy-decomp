#include "m2c.h"

/* func_02060084 (0x02060084, 0x6c bytes) -- MATCHES the USA ARM9 bytes exactly.
 *
 * Sibling of func_020600f0: same object layout, but the extra words are read
 * from a va_list-style cursor (called from the variadic wrapper func_02060158).
 *
 *   +0x94 : Row rows[2]      (32-byte rows, 8 s32 per row)
 *   +0xd4 : s32  counts[2]    (one counter per row)
 *
 * It stores one packed word  a2 | (a3 << 16)  at rows[a1].v[count], then a3
 * further words taken from the argument list, bumping the row counter for each.
 * The cursor is advanced before the load (ap += 4, then read ap[-4]), which is
 * the shape of a va_arg fetch.
 */

typedef struct {
    s32 v[8];
} Row;

typedef struct {
    u8 pad[0x94];
    Row rows[2];
    s32 counts[2];
} Obj;

void func_02060084(Obj *o, s32 a1, s32 a2, s32 a3, char *ap) {
    s32 *cur = (s32 *) ((char *) o + a1 * 4);
    s32 i;
    s32 j;
    u32 v;

    i = o->counts[a1];
    j = cur[53];
    o->counts[a1]++;
    o->rows[a1].v[j] = a2 | (a3 << 16);
    while (a3--) {
        v = *(u32 *) ((ap += 4) - 4);
        i = cur[53];
        cur[53]++;
        o->rows[a1].v[i] = v;
    }
}
