#include "m2c.h"

void *func_0205e624(void *dst, u32 value, u32 size);
void func_020b9d94(void);
void func_020ba104(u8 *arg0);
void DC_FlushRange(const void *addr, u32 size);
void SND_StartTimer(u32 chBitMask, u32 capBitMask, u32 alarmBitMask, u32 flags);
void SND_SetupCapture(s32 capture, s32 format, void *buffer, s32 length, s32 loop, s32 in, s32 out);
void SND_SetupAlarm(s32 alarmNo, u32 tick, u32 halfSamples, void (*handler)(void *), void *arg);
void SND_SetupChannelPcm(s32 chNo, s32 format, const void *dataAddr, s32 loop, s32 loopStart, s32 loopLen, s32 volume,
                         s32 shift, s32 timer, s32 pan);
void SND_WaitForFreeCommand(s32 count);
void func_020ba0a4(void *arg);

extern M2C_UNK data_0210dc20;
extern M2C_UNK data_0210e0bc;
extern M2C_UNK data_0210e0f4;
extern M2C_UNK data_0210e120;
extern M2C_UNK data_0210e124;

enum {
    FLAG_PCM16          = 0x4,
    FLAG_ROUTING_MASK   = 0x3,
    FLAG_STOPPED        = 0x20,
    MODE_NO_ALARM       = 7,
    CAPTURE_BUFFER_SIZE = 0x2000,
    CAPTURE_HALF_SIZE   = 0x1000,
};

/*
 * (Re)starts the two-channel sound capture loop described by the state block
 * at data_0210dc20. Does nothing while the flag word at +0x498 is zero.
 * Steps:
 *  - stop the running capture first (func_020b9d94 sets FLAG_STOPPED);
 *  - clear and flush the 0x2000-byte capture buffer at +0x528, one half per
 *    side (capture 0 / channel 1 use the first half, capture 1 / channel 3
 *    the second);
 *  - unless the mode byte at +0x49C is 7, reset the two per-half records
 *    (+0x4C8 and +0x4F8: buffer pointer, 0, sample count of one half);
 *  - FLAG_PCM16 picks PCM16 (capture format 0, wave format 1) over PCM8;
 *    the low two flag bits pick the capture in/out routing value;
 *  - set up both captures and their PCM playback channels (volume +0x53A,
 *    pan +0x53B mirrored as 0x7F - pan on channel 3, channel timer from
 *    data_0210e120+0x38);
 *  - unless the mode byte is 7, arm alarm 0 with handler func_020ba0a4;
 *  - start channels 1 and 3, captures 0 and 1, and the alarm if armed.
 */
void func_020b9de4(void) {
    s32 capFormat;
    s32 pcmFormat;
    s32 routing;
    s32 alarmMask;
    u8 is16Bit;
    s32 sampleBytes;
    u32 halfSamples;
    s32 step;
    s32 alarmSamples;
    s32 timer;

    alarmMask = 0;
    if (M2C_FIELD(&data_0210dc20, s32 *, 0x498) == 0) {
        return;
    }
    if (!(M2C_FIELD(&data_0210dc20, s32 *, 0x498) & FLAG_STOPPED)) {
        func_020b9d94();
    }
    func_0205e624(M2C_FIELD(&data_0210dc20, u8 **, 0x528), 0, CAPTURE_BUFFER_SIZE);
    DC_FlushRange(M2C_FIELD(&data_0210dc20, u8 **, 0x528), CAPTURE_BUFFER_SIZE);
    if (M2C_FIELD(&data_0210dc20, u8 *, 0x49C) != MODE_NO_ALARM) {
        is16Bit     = (M2C_FIELD(&data_0210dc20, s32 *, 0x498) & FLAG_PCM16) ? 1 : 0;
        sampleBytes = (is16Bit == 1) ? 2 : 1;
        func_020ba104((u8 *) &data_0210e0bc);
        M2C_FIELD(&data_0210dc20, u8 *, 0x4F4)  = is16Bit;
        M2C_FIELD(&data_0210dc20, u8 **, 0x4C8) = M2C_FIELD(&data_0210dc20, u8 **, 0x528);
        M2C_FIELD(&data_0210dc20, s32 *, 0x4CC) = 0;
        halfSamples                             = CAPTURE_HALF_SIZE / (u32) sampleBytes;
        M2C_FIELD(&data_0210dc20, u32 *, 0x4D0) = halfSamples;
        func_0205e624(&data_0210e0f4, 0, 0x20);
        M2C_FIELD(&data_0210dc20, u8 *, 0x524)  = is16Bit;
        M2C_FIELD(&data_0210dc20, u8 **, 0x4F8) = M2C_FIELD(&data_0210dc20, u8 **, 0x528) + CAPTURE_HALF_SIZE;
        M2C_FIELD(&data_0210dc20, s32 *, 0x4FC) = 0;
        M2C_FIELD(&data_0210dc20, u32 *, 0x500) = halfSamples;
        func_0205e624(&data_0210e124, 0, 0x20);
    }
    if (M2C_FIELD(&data_0210dc20, s32 *, 0x498) & FLAG_PCM16) {
        capFormat = 0;
        pcmFormat = 1;
    } else {
        capFormat = 1;
        pcmFormat = 0;
    }
    switch (M2C_FIELD(&data_0210dc20, s32 *, 0x498) & FLAG_ROUTING_MASK) {
        case 1:
            routing = 1;
            break;
        case 2:
            routing = 0;
            break;
    }
    SND_WaitForFreeCommand(6);
    SND_SetupCapture(0, capFormat, M2C_FIELD(&data_0210dc20, u8 **, 0x528),
                     M2C_FIELD(&data_0210dc20, s32 *, 0x52C) / sizeof(u32), 1, routing, routing);
    SND_SetupChannelPcm(1, pcmFormat, M2C_FIELD(&data_0210dc20, u8 **, 0x528), 1, 0,
                        M2C_FIELD(&data_0210dc20, s32 *, 0x52C) / 4, M2C_FIELD(&data_0210dc20, u8 *, 0x53A), 0,
                        M2C_FIELD(&data_0210e120, u16 *, 0x38), M2C_FIELD(&data_0210dc20, u8 *, 0x53B));
    SND_SetupCapture(1, capFormat, M2C_FIELD(&data_0210dc20, u8 **, 0x528) + CAPTURE_HALF_SIZE,
                     M2C_FIELD(&data_0210dc20, s32 *, 0x52C) / sizeof(u32), 1, routing, routing);
    SND_SetupChannelPcm(3, pcmFormat, M2C_FIELD(&data_0210dc20, u8 **, 0x528) + CAPTURE_HALF_SIZE, 1, 0,
                        M2C_FIELD(&data_0210dc20, s32 *, 0x52C) / 4, M2C_FIELD(&data_0210dc20, u8 *, 0x53A), 0,
                        M2C_FIELD(&data_0210e120, u16 *, 0x38), 0x7F - M2C_FIELD(&data_0210dc20, u8 *, 0x53B));
    if (M2C_FIELD(&data_0210dc20, u8 *, 0x49C) != MODE_NO_ALARM) {
        alarmSamples = M2C_FIELD(&data_0210dc20, s32 *, 0x52C) / sizeof(u16);
        step         = 32;
        if (M2C_FIELD(&data_0210dc20, s32 *, 0x498) & FLAG_PCM16) {
            alarmSamples >>= 1;
            step >>= 1;
        }
        timer     = M2C_FIELD(&data_0210e120, u16 *, 0x38);
        alarmMask = 1;
        SND_SetupAlarm(0, timer + step * (timer >> 5), (u32) (timer * alarmSamples) >> 5, func_020ba0a4, 0);
    }
    SND_StartTimer(10, 3, alarmMask, 0);
}
