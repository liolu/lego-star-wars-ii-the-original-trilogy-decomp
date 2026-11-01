#include "m2c.h"

typedef struct {
    u32 hours;
    u32 minutes;
    u32 seconds;
} TimeStruct;

typedef struct VObj {
    void (**vtbl)(struct VObj *);
} VObj;

s32 func_0209084c(s32, s32);
s32 func_02053b14(s32 *);
void func_0208adb8(s32, u16 *, s8 *);
void func_020bb964(TimeStruct *, s32 *);
void MIi_CpuClearFast(s32, s32, s32);
int sprintf(char *, const char *, ...);

extern s32 data_0210930c[];
extern u32 data_02102988[];
extern s32 data_027e02a4;
typedef struct {
    u8 pad[0x4c8];
    s32 now; /* running counter, read as seconds here */
} GameTimer;
extern GameTimer data_02108d90;
extern char data_020ff460[];
extern char data_020ff468[];

enum {
    CLEAR_FILL         = 0x400040,
    TIME_RELATIVE_FLAG = 0x80000000, /* func_020bb964: negative input = flag + seconds */
    COUNTDOWN_SECONDS  = 180,
    REFRESH_INTERVAL   = 15,
    MAX_MINUTES        = 3,
    MAX_SECONDS        = 60,
};

/*
 * Refreshes a status display owned by p.
 * 1. Looks up the current value for slot p[0x86]; if it differs from the cached p[0x88],
 *    caches it. If it now equals p[0x87], clears 0x80 bytes at +0x580 of the u16 buffer from
 *    func_02053b14(p + 8), drops flags 0x60 of p+0x78, calls virtual slot 1 of the objects at
 *    p+0x5c/p+0x60, nulls both pointers and stops.
 * 2. Otherwise a changed value is printed (format data_020ff460) into that buffer at +0x59e.
 * 3. If flag 0x20 is set, a 3-minute countdown (180 s minus now - p+0x7c) is converted by
 *    func_020bb964 and printed as minutes/seconds (format data_020ff468) at +0x5b8; unless
 *    forced (arg1), only on every 15th tick of data_02102988[data_027e02a4].
 */
void func_0208c0c0(void *arg0, s32 arg1) {
    u8 *p       = (u8 *) arg0;
    s32 changed = 0;
    s32 reached = 0;
    s32 *entry;
    char buf[32];
    u32 remaining;
    TimeStruct clock;
    TimeStruct conv;
    VObj *o;

    entry = (s32 *) func_0209084c(*(s32 *) (data_0210930c + 0x68 / 4), p[0x86]);
    if (*entry != p[0x88]) {
        *(s8 *) (p + 0x88) = *entry;
        changed            = 1;
        if (p[0x88] == p[0x87]) {
            reached = 1;
        }
    }
    if (reached != 0) {
        MIi_CpuClearFast(CLEAR_FILL, func_02053b14((s32 *) (p + 8)) + 0x580, 0x80);
        *(s32 *) (p + 0x78) &= ~0x60;
        o = *(VObj **) (p + 0x5c);
        if (o != NULL) {
            o->vtbl[1](o);
        }
        *(s32 *) (p + 0x5c) = 0;
        o                   = *(VObj **) (p + 0x60);
        if (o != NULL) {
            o->vtbl[1](o);
        }
        *(s32 *) (p + 0x60) = 0;
        return;
    }
    if (changed != 0) {
        sprintf(buf, data_020ff460, p[0x88], p[0x87]);
        func_0208adb8((s32) p, (u16 *) (func_02053b14((s32 *) (p + 8)) + 0x9e + 0x500), (s8 *) buf);
    }
    if ((*(s32 *) (p + 0x78) & 0x20) == 0) {
        return;
    }
    if (arg1 == 0) {
        if (data_02102988[data_027e02a4] % REFRESH_INTERVAL != 0) {
            return;
        }
    }
    remaining = TIME_RELATIVE_FLAG;
    remaining += COUNTDOWN_SECONDS;
    remaining -= (s32) ((s64) data_02108d90.now - *(s32 *) (p + 0x7c));
    func_020bb964(&conv, (s32 *) &remaining);
    clock = conv;
    if (clock.minutes > MAX_MINUTES || clock.seconds > MAX_SECONDS) {
        clock.minutes = 0;
        clock.seconds = 0;
    }
    sprintf(buf, data_020ff468, clock.minutes, clock.seconds);
    func_0208adb8((s32) p, (u16 *) (func_02053b14((s32 *) (p + 8)) + 0x1b8 + 0x400), (s8 *) buf);
}
