#include "m2c.h"

extern u32 data_0210930c[];
extern s32 data_021019d8;
extern void func_0204f9bc(void *result, void *world, void *from, void *to, s32 flags);
extern M2C_UNK func_0200a6c4(void *, void *, void *, s32, s32, s32);

typedef struct Vec3 {
    s32 x;
    s32 y;
    s32 z;
} Vec3;

/* Candidate record (object, distance); the caller func_0202d044 gets the list from func_0202d2bc. */
typedef struct Entry {
    void *obj;
    s32 dist;
} Entry;

enum {
    LIFT_Y        = 0x2000, /* probe points are raised by 2.0 (fixed point 20.12) */
    MIN_CLEARANCE = 0x2000, /* same threshold as func_02027124 / func_0202db94 */
};

/* Copies the position at +0xb0 of an object and raises it by LIFT_Y. */
static inline void get_lifted_pos(Vec3 *out, u8 *o) {
    out->x = *(s32 *) (o + 0xb0);
    out->y = *(s32 *) (o + 0xb4);
    out->z = *(s32 *) (o + 0xb8);
    out->y += LIFT_Y;
}

/* Returns the first of the `count` candidates in `list` that passes two checks from `obj`, else NULL
 * (arg0 is not used). For each candidate func_0204f9bc probes from obj's lifted position towards the
 * candidate's (it hands to - from on to func_01ff97c4). When the result distance is at least
 * MIN_CLEARANCE, field +0x28 of both objects is set to data_021019d8 + 1 and
 * func_0200a6c4(from, to, {0, 0}, 2, 7, 1) is called; the candidate is returned when that yields 0. */
void *func_0202d444(void *arg0, u8 *obj, Entry *list, u32 count) {
    Entry result;
    Vec3 from;
    Vec3 to;
    u32 i;

    get_lifted_pos(&from, obj);

    for (i = 0; i < count; i++) {
        get_lifted_pos(&to, list[i].obj);
        func_0204f9bc(&result, M2C_FIELD(M2C_FIELD(&data_0210930c, void **, 0x68), void **, 0x384), &from, &to, 0);
        if (result.dist >= MIN_CLEARANCE) {
            s32 zeros[2]                         = {0, 0};
            *(s32 *) (obj + 0x28)                = data_021019d8 + 1;
            *(s32 *) ((u8 *) list[i].obj + 0x28) = data_021019d8 + 1;
            if (func_0200a6c4(&from, &to, zeros, 2, 7, 1) == 0) {
                return list[i].obj;
            }
        }
    }
    return 0;
}
