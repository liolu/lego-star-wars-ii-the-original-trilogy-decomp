/* Reconstructed from USA ARM9 at 0x0209ae80 (0xb8 bytes).
 *
 * Draws a decimal number (clamped to 999) right aligned into a two row tile
 * map held by the object: the digit glyphs live at 0x300 (upper half) and
 * 0x320 (lower half) in the same buffer.
 */
#include "m2c.h"

typedef struct {
    u8 pad[0xa4];
    u16 *map;
} Object;

void func_0209ae80(Object *self, u16 value, s32 y, s32 x) {
    u16 *dst;
    s32 offset;
    s32 digit;
    u8 i;

    if (value >= 1000) {
        value = 999;
    }
    offset = (s16) (y / 8) + ((s16) (x / 8) << 5);
    dst    = self->map + offset;
    i      = 0;
    do {
        digit         = value % 10;
        dst[2 - i]    = self->map[0x300 + digit];
        dst[0x22 - i] = self->map[0x320 + digit];
        value         = value / 10;
        i++;
    } while (value != 0);
}
