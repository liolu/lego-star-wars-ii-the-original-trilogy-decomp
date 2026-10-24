#include "m2c.h"

typedef struct {
    s32 year;
    s32 month;
    s32 day;
    s32 week;
} Date;

typedef struct {
    s32 hour;
    s32 minute;
    s32 second;
} Time;

typedef struct {
    u8 f0;
    u8 pad[3];
    s32 a;
    s32 b;
    s32 c;
    u32 *tbl;
    s32 g;
    s32 count;
    s32 seed;
} Rng;

extern M2C_UNK data_02104344;

s32 RTC_GetDateTime(Date *date, Time *time);
void func_02062b6c(s32 seed);

void func_020629c4(void) {
    Date date;
    Time time;
    Rng *r;
    s32 seed;

    RTC_GetDateTime(&date, &time);
    r       = (Rng *) &data_02104344;
    r->b    = -1;
    r->a    = -1;
    seed    = (time.hour << 24) + (time.minute << 16) + (time.second << 8) + ((date.day + date.month) << 4) + time.second;
    r->seed = seed;
    r->count += 1;
    func_02062b6c(seed);
    M2C_FIELD(&data_02104344, u8 *, 0) = 0;
}
