#include "m2c.h"

typedef struct {
    u8 pad0[0x64];
    s32 min[4];   /* 0x64 */
    s32 max[4];   /* 0x74 */
    s32 avg[4];   /* 0x84 */
    u16 count[4]; /* 0x94 */
    s32 last[4];  /* 0x9c */
} Stats;

extern s32 data_02109258;

// FAKE: identity accessor for &data_02109258; only used to hold the global's address in a local (below)
static inline s32 *clock_ptr(void) {
    return &data_02109258;
}

/* Per-slot timing statistics: the time since the slot's previous sample
 * (64-bit difference) updates the minimum, maximum and running mean,
 * then the sample count is bumped and the current time stored. */
void func_0209cf60(Stats *s, u8 i) {
    // FAKE: global address held in a local; keeps &data_02109258 in r4 across the _ll_sdiv call, so the
    // final *clock re-read is ldr r0,[r4] (forms without the local judge 17-20)
    s32 *clock = clock_ptr();
    s32 now    = *clock;
    s64 elapsed;
    s32 total;

    elapsed = (s64) now - s->last[i];
    if (elapsed < s->min[i]) {
        s->min[i] = elapsed;
    }
    elapsed = (s64) now - s->last[i];
    if (s->max[i] < elapsed) {
        s->max[i] = elapsed;
    }
    total     = s->avg[i] * s->count[i];
    elapsed   = (s64) now - s->last[i];
    s->avg[i] = (total + elapsed) / (s->count[i] + 1);
    s->count[i]++;
    s->last[i] = *clock;
}
