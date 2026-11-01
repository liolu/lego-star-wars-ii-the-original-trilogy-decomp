/* Reconstructed from USA ARM9 at 0x0205627c (entry in pointer table data_020ed928).
 * Looks up an object by record bytes 6 and 7 (func_02067080; NULL = no object, nothing done).
 * Record halfwords 0, 2, 4 are signed integer x, y, z; they are scaled to 20.12 fixed point
 * and, if they differ from the object's vector at +0xb0, copied there and dirty byte +0xd1 is set.
 * Record byte 8 indexes func_02098440's degree table, giving a 16-bit binary angle; if it differs
 * from the object's halfword at +0xca it is stored there and dirty byte +0xd0 is set.
 * Sibling func_020561e8 updates the same vector/+0xd1 pair from a Vec3 table.
 */
#include "m2c.h"

typedef struct {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

typedef struct {
    u16 value;
} Halfword;

enum {
    FX32_SHIFT = 12,
};

M2C_UNK func_02067080(M2C_UNK, u8, u8);
void func_02098440(u16 *, u32);

static inline void Vec3_set(Vec3 *v, s32 x, s32 y, s32 z) {
    v->x = x;
    v->y = y;
    v->z = z;
}

void func_0205627c(M2C_UNK arg0, M2C_UNK arg1) {
    u8 *keys = (u8 *) arg1;
    M2C_UNK obj;
    Vec3 value;
    Vec3 *slot;
    s32 changed;
    Halfword angle;
    Halfword angle_copy;

    obj = func_02067080(arg0, keys[6], keys[7]);
    if (obj == 0) {
        return;
    }
    Vec3_set(&value, (s32) * (s16 *) (keys + 0) << FX32_SHIFT, (s32) * (s16 *) (keys + 2) << FX32_SHIFT,
             (s32) * (s16 *) (keys + 4) << FX32_SHIFT);
    slot = &value;
    if (M2C_FIELD(obj, s32 *, 0xb0) != slot->x || M2C_FIELD(obj, s32 *, 0xb4) != slot->y ||
        M2C_FIELD(obj, s32 *, 0xb8) != slot->z) {
        changed = 1;
    } else {
        changed = 0;
    }
    if (changed != 0) {
        if ((s32 *) ((u8 *) obj + 0xb0) != (s32 *) slot) {
            M2C_FIELD(obj, s32 *, 0xb0) = slot->x;
            M2C_FIELD(obj, s32 *, 0xb4) = slot->y;
            M2C_FIELD(obj, s32 *, 0xb8) = slot->z;
        }
        M2C_FIELD(obj, u8 *, 0xd1) = 1;
    }
    func_02098440(&angle.value, keys[8]);
    angle_copy = angle;
    if (M2C_FIELD(obj, u16 *, 0xca) != angle_copy.value) {
        M2C_FIELD(obj, u16 *, 0xca) = angle_copy.value;
        M2C_FIELD(obj, u8 *, 0xd0)  = 1;
    }
}
