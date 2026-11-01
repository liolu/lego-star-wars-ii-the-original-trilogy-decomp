#include "m2c.h"

void SND_WaitForFreeCommand(s32);
void SND_SetupChannelPcm(s32, s32, const void *, s32, s32, s32, s32, s32, s32, s32);

/* Wave description passed by func_020bb4fc / func_020bb84c. */
typedef struct {
    void *data;
    s32 unk4;
    u8 flags;
    u8 pad9[3];
    s32 len;       /* end offset used when the wave does not loop */
    s32 loopStart;
    s32 loopEnd;
    u32 rate;      /* divisor of the sound timer clock */
} Wave;

enum {
    WAVE_FLAG_PCM16 = 0x02,
    WAVE_FLAG_ADPCM = 0x08,
    WAVE_FLAG_LOOP  = 0x10,
};

/* NitroSDK SNDWaveFormat / SNDChannelLoop values. */
enum {
    SND_WAVE_FORMAT_PCM8    = 0,
    SND_WAVE_FORMAT_PCM16   = 1,
    SND_WAVE_FORMAT_ADPCM   = 2,
    SND_CHANNEL_LOOP_REPEAT = 1,
    SND_CHANNEL_LOOP_1SHOT  = 2,
};

enum {
    SND_TIMER_CLOCK = 0xFFB0FF,
    CP_DIVCNT_BUSY  = 0x8000,
    PAN_CENTER      = 0x40,
};

/* Hardware divider: DIVCNT = 32/32 mode, numerator, 64-bit denominator. */
static inline void CP_SetDiv32_32(u32 numer, u32 denom) {
    *(volatile u16 *) 0x04000280 = 0;
    *(volatile u32 *) 0x04000290 = numer;
    *(volatile u64 *) 0x04000298 = denom;
}

static inline void CP_WaitDiv(void) {
    while (*(volatile u16 *) 0x04000280 & CP_DIVCNT_BUSY) {
    }
}

/*
 * Sets up PCM playback of a wave on a sound channel: picks the loop range
 * (whole wave as one-shot, or loopStart..loopEnd repeating), the sample
 * format from the wave flags, and the channel timer as
 * SND_TIMER_CLOCK / rate computed on the hardware divider. Volume and
 * shift are 0, pan is centred.
 */
void func_020bada8(s32 channel, Wave *wave) {
    s32 start;
    s32 end;
    s32 loop;
    s32 format;

    if (wave->flags & WAVE_FLAG_LOOP) {
        start = wave->loopStart;
        end   = wave->loopEnd;
        loop  = SND_CHANNEL_LOOP_REPEAT;
    } else {
        end   = wave->len;
        loop  = SND_CHANNEL_LOOP_1SHOT;
        start = 0;
    }
    if (wave->flags & WAVE_FLAG_ADPCM) {
        format = SND_WAVE_FORMAT_ADPCM;
    } else if (!(wave->flags & WAVE_FLAG_PCM16)) {
        format = SND_WAVE_FORMAT_PCM8;
    } else {
        format = SND_WAVE_FORMAT_PCM16;
    }
    SND_WaitForFreeCommand(1);
    CP_SetDiv32_32(SND_TIMER_CLOCK, wave->rate);
    CP_WaitDiv();
    SND_SetupChannelPcm(channel, format, wave->data, loop, start, end - start, 0, 0, *(volatile u32 *) 0x040002A0, PAN_CENTER);
}
