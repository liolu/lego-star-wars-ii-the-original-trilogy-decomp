#include "m2c.h"
typedef struct {
    u32 year : 8;
    u32 month : 5;
    u32 dummy0 : 3;
    u32 day : 6;
    u32 dummy1 : 2;
    u32 week : 3;
    u32 dummy2 : 5;
} RTCRawDate;
typedef struct {
    u32 hour : 6;
    u32 afternoon : 1;
    u32 dummy0 : 1;
    u32 minute : 7;
    u32 dummy1 : 1;
    u32 second : 7;
    u32 dummy2 : 9;
} RTCRawTime;
typedef struct {
    u16 intr1_mode : 4;
    u16 dummy0 : 2;
    u16 intr2_mode : 1;
    u16 dummy1 : 9;
} RTCRawStatus2;
typedef struct {
    u32 week : 3;
    u32 dummy0 : 4;
    u32 enable_week : 1;
    u32 hour : 6;
    u32 afternoon : 1;
    u32 enable_hour : 1;
    u32 minute : 7;
    u32 enable_minute : 1;
    u32 dummy1 : 8;
} RTCRawAlarm;
typedef union {
    struct {
        RTCRawDate date;
        RTCRawTime time;
    } t;
    struct {
        u16 status1;
        RTCRawStatus2 status2;
        RTCRawAlarm alarm;
    } a;
} RTCRawData;
typedef struct {
    u32 year;
    u32 month;
    u32 day;
    u32 week;
} RTCDate;
typedef struct {
    u32 hour;
    u32 minute;
    u32 second;
} RTCTime;
typedef struct {
    u32 week;
    u32 hour;
    u32 minute;
    u32 enable;
} RTCAlarmParam;
typedef void (*RTCCallback)(s32 result, void *arg);
typedef struct {
    u32 pad0;
    u32 lock;
    RTCCallback callback;
    void *buffer;
    void *buffer2;
    void *callbackArg;
    u32 command;
    u32 sequence;
    void (*interrupt)(void);
} RTCWork;
extern RTCWork RTCi_Bss;
s32 RtcBCD2HEX(u32);
s32 RTC_GetDayOfWeek(RTCDate *);
s32 RTCi_WriteRawStatus2Async(void);

void RtcCommonCallback(s32 tag, u32 data, s32 err) {
    s32 result;
    u8 command;
    u8 pxiResult;
    RTCCallback cb;

    if (err) {
        if (RTCi_Bss.sequence) {
            RTCi_Bss.sequence = 0;
        }
        if (RTCi_Bss.lock) {
            RTCi_Bss.lock = 0;
        }
        if (RTCi_Bss.callback) {
            cb                = RTCi_Bss.callback;
            RTCi_Bss.callback = 0;
            cb(6, RTCi_Bss.callbackArg);
        }
        return;
    }
    command   = (u8) ((data & 0x7F00) >> 8);
    pxiResult = (u8) (data & 0xFF);
    if (command == 0x30) {
        if (RTCi_Bss.interrupt) {
            RTCi_Bss.interrupt();
        }
        return;
    }
    if (pxiResult == 0) {
        result = 0;
        switch (RTCi_Bss.command) {
            case 0: {
                RTCDate *pDate = (RTCDate *) RTCi_Bss.buffer;
                pDate->year    = RtcBCD2HEX(((RTCRawData *) 0x027FFDE8)->t.date.year);
                pDate->month   = RtcBCD2HEX(((RTCRawData *) 0x027FFDE8)->t.date.month);
                pDate->day     = RtcBCD2HEX(((RTCRawData *) 0x027FFDE8)->t.date.day);
                pDate->week    = RTC_GetDayOfWeek(pDate);
                break;
            }
            case 1: {
                RTCTime *pTime = (RTCTime *) RTCi_Bss.buffer;
                pTime->hour    = RtcBCD2HEX(((RTCRawData *) 0x027FFDE8)->t.time.hour);
                pTime->minute  = RtcBCD2HEX(((RTCRawData *) 0x027FFDE8)->t.time.minute);
                pTime->second  = RtcBCD2HEX(((RTCRawData *) 0x027FFDE8)->t.time.second);
                break;
            }
            case 2: {
                RTCDate *pDate = (RTCDate *) RTCi_Bss.buffer;
                RTCTime *pTime;
                pDate->year   = RtcBCD2HEX(*(u32 *) 0x027FFDE8 & 0xFF);
                pDate->month  = RtcBCD2HEX(((RTCRawData *) 0x027FFDE8)->t.date.month);
                pDate->day    = RtcBCD2HEX(((RTCRawData *) 0x027FFDE8)->t.date.day);
                pDate->week   = RTC_GetDayOfWeek(pDate);
                pTime         = (RTCTime *) RTCi_Bss.buffer2;
                pTime->hour   = RtcBCD2HEX(((RTCRawData *) 0x027FFDE8)->t.time.hour);
                pTime->minute = RtcBCD2HEX(((RTCRawData *) 0x027FFDE8)->t.time.minute);
                pTime->second = RtcBCD2HEX(((RTCRawData *) 0x027FFDE8)->t.time.second);
                break;
            }
            case 6: {
                u32 *pEnable = (u32 *) RTCi_Bss.buffer;
                switch (((RTCRawData *) 0x027FFDE8)->a.status2.intr1_mode) {
                    case 4:
                        *pEnable = 1;
                        break;
                    default:
                        *pEnable = 0;
                        break;
                }
                break;
            }
            case 7: {
                u32 *pEnable = (u32 *) RTCi_Bss.buffer;
                if (((RTCRawData *) 0x027FFDE8)->a.status2.intr2_mode) {
                    *pEnable = 1;
                } else {
                    *pEnable = 0;
                }
                break;
            }
            case 8: {
                RTCAlarmParam *pAlarm = (RTCAlarmParam *) RTCi_Bss.buffer;
                pAlarm->week          = ((RTCRawData *) 0x027FFDE8)->a.alarm.week;
                pAlarm->hour          = RtcBCD2HEX(((RTCRawData *) 0x027FFDE8)->a.alarm.hour);
                pAlarm->minute        = RtcBCD2HEX(((RTCRawData *) 0x027FFDE8)->a.alarm.minute);
                pAlarm->enable        = 0;
                if (((RTCRawData *) 0x027FFDE8)->a.alarm.enable_week) {
                    pAlarm->enable += 1;
                }
                if (((RTCRawData *) 0x027FFDE8)->a.alarm.enable_hour) {
                    pAlarm->enable += 2;
                }
                if (((RTCRawData *) 0x027FFDE8)->a.alarm.enable_minute) {
                    pAlarm->enable += 4;
                }
                break;
            }
            case 9:
                if (RTCi_Bss.sequence == 0) {
                    if (*(u32 *) RTCi_Bss.buffer == 1) {
                        if (((RTCRawData *) 0x027FFDE8)->a.status2.intr1_mode != 4) {
                            RTCi_Bss.sequence++;
                            ((RTCRawData *) 0x027FFDE8)->a.status2.intr1_mode = 4;
                            if (!RTCi_WriteRawStatus2Async()) {
                                RTCi_Bss.sequence = 0;
                                result            = 3;
                            }
                        }
                    } else {
                        if (((RTCRawData *) 0x027FFDE8)->a.status2.intr1_mode != 0) {
                            RTCi_Bss.sequence++;
                            ((RTCRawData *) 0x027FFDE8)->a.status2.intr1_mode = 0;
                            if (!RTCi_WriteRawStatus2Async()) {
                                RTCi_Bss.sequence = 0;
                                result            = 3;
                            }
                        }
                    }
                } else {
                    RTCi_Bss.sequence = 0;
                }
                break;
            case 10:
                if (RTCi_Bss.sequence == 0) {
                    if (*(u32 *) RTCi_Bss.buffer == 1) {
                        if (((RTCRawData *) 0x027FFDE8)->a.status2.intr2_mode == 0) {
                            RTCi_Bss.sequence++;
                            ((RTCRawData *) 0x027FFDE8)->a.status2.intr2_mode = 1;
                            if (!RTCi_WriteRawStatus2Async()) {
                                RTCi_Bss.sequence = 0;
                                result            = 3;
                            }
                        }
                    } else {
                        if (((RTCRawData *) 0x027FFDE8)->a.status2.intr2_mode != 0) {
                            RTCi_Bss.sequence++;
                            ((RTCRawData *) 0x027FFDE8)->a.status2.intr2_mode = 0;
                            if (!RTCi_WriteRawStatus2Async()) {
                                RTCi_Bss.sequence = 0;
                                result            = 3;
                            }
                        }
                    }
                } else {
                    RTCi_Bss.sequence = 0;
                }
                break;
            case 3:
            case 4:
            case 5:
            case 11:
            case 12:
            case 13:
            case 14:
            case 15:
                break;
            default:
                RTCi_Bss.sequence = 0;
                result            = 4;
                break;
        }
    } else {
        RTCi_Bss.sequence = 0;
        switch (pxiResult) {
            case 1:
                result = 4;
                break;
            case 2:
                result = 5;
                break;
            case 3:
                result = 1;
                break;
            case 4:
            default:
                result = 6;
                break;
        }
    }
    if (RTCi_Bss.sequence == 0) {
        if (RTCi_Bss.lock) {
            RTCi_Bss.lock = 0;
        }
        if (RTCi_Bss.callback) {
            cb                = RTCi_Bss.callback;
            RTCi_Bss.callback = 0;
            cb(result, RTCi_Bss.callbackArg);
        }
    }
}
