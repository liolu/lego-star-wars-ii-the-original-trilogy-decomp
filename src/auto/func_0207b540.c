/* Reconstructed from USA ARM9 at 0x0207b540 (156 bytes).
 *
 * Lays out the five visible list entries. Entry i shows item slot
 * (first + i) % 5; entries are spaced 42 units apart starting at 65,
 * offset by the current scroll value. While scrolling forward (scroll > 0)
 * the last entry is placed one row above the first (65 - 42 = 23) so it can
 * slide in. Positions are stored as 20.12 fixed point; the second
 * coordinate is a constant 86.
 */
#include "m2c.h"

typedef struct {
    u8 pad00[0x4c];
    s32 f4c;
    s32 f50;
} Item_0207b540;

typedef struct {
    u8 pad00[0x1c];
    Item_0207b540 *items[5];
    s32 f30;
    s32 f34;
} Unk_0207b540;

void func_0207b540(Unk_0207b540 *p) {
    s32 i;
    s32 y;
    s32 pos;
    Item_0207b540 *item;
    for (i = 0, y = 65; i < 5; i++) {
        s32 slot = (p->f30 + i) % 5;
        if (i >= 4 && (pos = p->f34) > 0) {
            pos       = (pos + 23) << 12;
            item      = p->items[slot];
            item->f4c = pos;
            item->f50 = 0x56000;
        } else {
            pos       = p->f34;
            pos       = (pos + y) << 12;
            item      = p->items[slot];
            item->f4c = pos;
            item->f50 = 0x56000;
        }
        y += 42;
    }
}
