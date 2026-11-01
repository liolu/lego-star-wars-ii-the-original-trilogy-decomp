#include "m2c.h"

/* Per-type item size in bytes, indexed by the list type byte. */
extern const u8 data_020edc28[];

/* A list: word 0 is the item array, byte 8 the type (selects the item size). */
typedef struct {
    u8 *items;
    u8 unk_04[4];
    u8 type;
} List;

/* Returns the address of item `index` in `list`; the stride comes from the
   per-type size table. */
void *func_01ffa478(List *list, s32 index) {
    return list->items + index * data_020edc28[list->type];
}
