#include "m2c.h"

typedef struct {
    s32 x, y, z;
} Vec3;

/* Box record: centre and half-size per axis (func_0204c5fc builds centre - half / centre + half). */
typedef struct {
    s32 pad0;     /* 0x00 */
    Vec3 center;  /* 0x04 */
    s32 pad1;     /* 0x10 */
    Vec3 half;    /* 0x14 */
} Bounds;

typedef struct {
    u8 pad0[0x20];
    Bounds *bounds;           /* 0x20 */
    u8 pad1[0xB0 - 0x24];
    Vec3 ref;                 /* 0xB0: point the bounds centre is measured from */
} Outer;

extern s32 *func_0204c5fc(s32 *, Vec3 *, Vec3 *);
extern s32 func_0204c8a8(void *, s32 *);
extern M2C_UNK data_0210930c;

/* Moves obj's bounds box so that its centre keeps the same offset from obj->ref but relative to
 * `point` (centre = point + (bounds->center - obj->ref)), expands it with the bounds half-size into
 * a min/max box (func_0204c5fc: {x0, x1, y0, y1, z0, z1}) and returns func_0204c8a8 on that box and
 * the value read from data_0210930c (+0x68, then +0x384). What that call tests or returns is not
 * established here. */
s32 func_02042fa8(Outer *obj, Vec3 *point) {
    s32 box[6];
    Vec3 center;
    Bounds *bounds = obj->bounds;
    /* int (not s32) temporaries and the z, y, x per-axis order are what the compiler needs
     * for the register assignment and add operand order. */
    int x, y, z;
    z = bounds->center.z;
    z -= obj->ref.z;
    y = bounds->center.y;
    y -= obj->ref.y;
    x = bounds->center.x;
    x -= obj->ref.x;
    z        = point->z + z;
    y        = point->y + y;
    x        = point->x + x;
    center.x = x;
    center.y = y;
    center.z = z;
    func_0204c5fc(box, &center, &bounds->half);
    return func_0204c8a8(M2C_FIELD(M2C_FIELD(&data_0210930c, void **, 0x68), void **, 0x384), box);
}
