/* Reconstructed from USA ARM9 at 0x0208b064 (84 bytes).
 *
 * Writes one 2x2 block of halfword entries into a row-major halfword grid
 * whose rows are 0x20 entries apart (callers func_0208afd8 and
 * func_0208b0b8 pass a row pointer and a small variant: 0/3, or the
 * bouncing 0..2 pulse). The two base values are copied from the halfword
 * pair at data_020f0278+4 (0x1042, 0x104a); each is offset by variant * 2
 * and written with its successor: top row gets top, top + 1; the next row
 * gets bottom, bottom + 1. The first argument is unused.
 */
#include "m2c.h"

enum {
    ROW_STRIDE = 0x20,
};

typedef struct {
    u16 top;
    u16 bottom;
} TilePair;

typedef struct {
    u8 frames[4];
    TilePair tiles;
} TileTable;

extern const TileTable data_020f0278;

void func_0208b064(void *arg0, u16 *dst, s32 variant) {
    TilePair base = data_020f0278.tiles;
    u16 top;
    u16 bottom;

    top                 = base.top + variant * 2;
    dst[0]              = top;
    dst[1]              = top + 1;
    bottom              = base.bottom + variant * 2;
    dst[ROW_STRIDE]     = bottom;
    dst[ROW_STRIDE + 1] = bottom + 1;
}
