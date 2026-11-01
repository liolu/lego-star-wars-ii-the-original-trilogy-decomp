#include "m2c.h"

/* Box shape point test: slot 0x08 of the shape vtable data_020fd128
   (func_0201c6c8 calls it as (shape, obj, obj->0x8c, obj->0x90)).
   The shape holds a box (min x/y at 0x08/0x0c, max x/y at 0x10/0x14,
   set by the constructor func_0201c478) relative to the owner position
   at owner+0x4c/0x50. Returns 1 when (x, y) lies inside the box moved to
   that position (bounds inclusive), else 0.
   The function is variadic like its circle sibling func_0201c560, so x
   and y are read back from the argument home area. The offsets are u32:
   "box = offset + box" then stays a plain add instead of being folded
   into "box += offset". */

typedef struct {
    s32 x, y;
} Point;
typedef struct {
    u8 pad[8];
    s32 min_x, min_y, max_x, max_y;
} BoxShape;
typedef struct {
    u8 pad[0x4c];
    Point pos;
} Owner;

s32 func_0201c5e0(BoxShape *shape, Owner *owner, s32 x, s32 y, ...) {
    u32 off_x, off_y;
    s32 min_x, max_x, min_y, max_y;
    off_x = owner->pos.x;
    min_x = shape->min_x;
    min_x = off_x + min_x;
    max_x = shape->max_x;
    max_x = off_x + max_x;
    off_y = owner->pos.y;
    min_y = shape->min_y;
    min_y = off_y + min_y;
    max_y = shape->max_y;
    max_y = off_y + max_y;
    if (*(s32 *) &x >= min_x && *(s32 *) &x <= max_x && *(s32 *) &y >= min_y && *(s32 *) &y <= max_y) {
        return 1;
    }
    return 0;
}
