/* Fonctions du SDK qui parlent au processeur ARM7 : ici elles repondent directement.
 * Le jeu les appelle avec ses registres : un pointeur arrive en u32, adresse DS (DS() pour y acceder),
 * une fonction de rappel aussi (nds_call_target). */
#include "nitro.h"

typedef u64 (*Fn)();

typedef struct { u16 x, y, touch, validity; } TPData;

static TPData *auto_buf;
static u32 auto_size;

static void current_touch(TPData *d) {
    int down, x, y;
    present_touch(&down, &x, &y);
    d->x = (u16)x;
    d->y = (u16)y;
    d->touch = (u16)(down != 0);
    d->validity = 0;
}

void hle_frame(void) {
    if (auto_buf)
        for (u32 i = 0; i < auto_size; i++) current_touch(&auto_buf[i]);
}

void TP_Init(void) {}
u32 TP_GetUserInfo(u32 calibrate) {
    nds_zero(DS(calibrate), 12);
    return 1;
}
void TP_WaitBusy(u32 mask) {}
u32 TP_RequestSetStabilityAsync(u32 a, u32 b) { return 0; }
u32 TP_RequestSamplingAsync(void) { return 0; }
u32 TP_WaitCalibratedResult(u32 d) {
    current_touch((TPData *)DS(d));
    return 0;
}
void TP_GetCalibratedPoint(u32 disp, u32 raw) { *(TPData *)DS(disp) = *(const TPData *)DS(raw); }
u32 TP_RequestAutoSamplingStartAsync(u32 vcount, u32 freq, u32 buf, u32 size) {
    auto_buf = buf ? (TPData *)DS(buf) : 0;
    auto_size = size & 0xFFFF;
    return 0;
}
u32 TP_RequestAutoSamplingStopAsync(void) {
    auto_buf = 0;
    return 0;
}

/* alimentation */
static u32 backlight = 3;
u32 PM_SetBackLightAsync(u32 a, u32 b, u32 cb, u32 arg) {
    if (cb) ((Fn)nds_call_target(cb))(0, arg);
    return 0;
}
u32 PM_SetBackLight(u32 a, u32 b) { return 0; }
u32 PM_GetBackLight(u32 top, u32 bottom) {
    if (top) *(u32 *)DS(top) = backlight;
    if (bottom) *(u32 *)DS(bottom) = backlight;
    return 0;
}
u32 PM_SetLCDPower(u32 on) { return 1; }
u32 PM_GetLCDPower(void) { return 1; }
u32 PM_ForceToPowerOffAsync(u32 cb, u32 arg) { return 0; }
u32 PM_ForceToPowerOff(void) { os_exit(0); return 0; }

/* horloge */
typedef struct { u32 year, month, day, week; } RTCDate;
typedef struct { u32 hour, minute, second; } RTCTime;

void RTC_Init(void) {}
static void date_time(RTCDate *d, RTCTime *t) {
    OsTime s;
    os_local_time(&s);
    if (d) { d->year = s.year % 100; d->month = s.month; d->day = s.day; d->week = s.dow; }
    if (t) { t->hour = s.hour; t->minute = s.minute; t->second = s.second; }
}
u32 RTC_GetDateTime(u32 d, u32 t) {
    date_time(d ? (RTCDate *)DS(d) : 0, t ? (RTCTime *)DS(t) : 0);
    return 0;
}
u32 RTC_GetTime(u32 t) { return RTC_GetDateTime(0, t); }
u32 RTC_GetDate(u32 d) { return RTC_GetDateTime(d, 0); }
u32 RTC_GetDateTimeAsync(u32 d, u32 t, u32 cb, u32 arg) {
    RTC_GetDateTime(d, t);
    if (cb) ((Fn)nds_call_target(cb))(0, arg);
    return 0;
}

/* lecture de fichiers : la rom est deja en memoire, resultat immediat (FS_RESULT_SUCCESS = 0) */
u32 FSi_ReadRomCallback(u32 arc, u32 dst, u32 pos, u32 size) {
    if (pos + size > nds_rom_size) nds_fatal("lecture hors de la rom : %x + %x", pos, size);
    nds_copy(DS(dst), nds_rom + pos, size);
    if (nds_cb.rom_read) nds_mods_rom_read(pos, size, DS(dst));
    return 0;
}

/* OS_Terminate des assertions du jeu : autant le dire que tourner en rond sans image */
void nds_hook_game_halt(u32 r0) {
    extern u32 nds_pc, nds_r[6];
#ifdef NDS_INSTRUMENT
    extern u32 nds_ring[4096], nds_ring_pos;
    for (int i = 4096 - 200; i < 4096; i++) nds_log("  appel %8x", nds_ring[(nds_ring_pos + i) & 4095]);
#endif
    nds_fatal("le jeu s'est arrete sur une de ses assertions : arm=%x r0=%x r1=%x r2=%x r3=%x r4=%x r5=%x", nds_pc, nds_r[0], nds_r[1], nds_r[2], nds_r[3], nds_r[4], nds_r[5]);
}
