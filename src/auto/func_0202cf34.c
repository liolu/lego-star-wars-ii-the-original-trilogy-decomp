#include "m2c.h"

typedef struct {
    s32 range;
    u8 gain;
    u8 loss;
    u8 pad[2];
} RangeEntry;

extern RangeEntry data_020ff7d8[];
typedef struct {
    u8 v;
    u8 pad[7];
} ByteEntry;
extern ByteEntry data_020ff7dc[];
extern ByteEntry data_020ff7dd[];

/* Distance between the three-word vectors at a1 and a2 (hardware square root). */
void func_0202d884(u32 *out, s32 *a1, s32 *a2);

enum {
    VEC_OFFSET = 0xb0, /* offset of the vector passed to func_0202d884 in arg1/arg2 */
};

/* Called by func_0202d044 with (self, info, target). Indexes the 8-byte table
 * entries by obj[1]: word at +0 is a range, byte at +4 a gain, byte at +5 a loss.
 * Stores the info-target distance at obj+8. Inside the range, picks a band:
 * nearest quarter adds gain * 4 to the s16 at obj+4 and sets obj[3] to 0;
 * up to three quarters adds gain and caps obj[3] at 1; up to the range adds
 * gain / 2 and caps obj[3] at 2. Out of range, subtracts the loss (halved when
 * obj[2] >= 2) from the s16 at obj+4, clamped at 0. Returns 1 when in range. */
s32 func_0202cf34(void *arg0, void *arg1, void *arg2) {
    u8 *obj = arg0;
    u32 dist;
    s32 in_range = 0;
    s32 increase = 0;
    u32 gain;
    s32 sdist;
    s32 range;
    range = data_020ff7d8[obj[1]].range;
    gain  = data_020ff7dc[obj[1]].v;

    func_0202d884(&dist, (s32 *) ((u8 *) arg1 + VEC_OFFSET), (s32 *) ((u8 *) arg2 + VEC_OFFSET));
    *(u32 *) (obj + 8) = dist;
    /* signed view of the distance for the outer two band tests */
    sdist = *(
        s32 *) &dist; // FAKE: type-punned s32 re-read of dist (plain sdist = dist differs in codegen) for the outer band tests
    if ((s32) dist < (range >> 2)) {
        obj[3]   = 0;
        increase = gain * 4;
        in_range = 1;
    } else if (sdist < range - (range >> 2)) {
        in_range = 1;
        increase = gain;
        if (obj[3] > 1) {
            obj[3] = 1;
        }
    } else if (sdist < range) {
        increase = gain >> 1;
        in_range = 1;
        if (obj[3] > 2) {
            obj[3] = 2;
        }
    }
    if (!in_range) {
        s16 loss = data_020ff7dd[obj[1]].v;
        if (obj[2] >= 2) {
            loss >>= 1;
        }
        *(s16 *) (obj + 4) -= loss;
        if (*(s16 *) (obj + 4) < 0) {
            *(s16 *) (obj + 4) = 0;
        }
    } else {
        *(s16 *) (obj + 4) += increase;
    }
    return in_range;
}
