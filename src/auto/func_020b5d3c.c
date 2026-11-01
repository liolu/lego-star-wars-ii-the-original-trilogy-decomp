#include "m2c.h"

typedef struct {
    u8 note;
    u8 instr;
    u8 volcmd;
    u8 cmd;
    u8 param;
    u8 f05[0x1a];
    u8 lo1f : 4;
    u8 delay : 4;
    u8 f20[2];
    u8 flags;
    u8 f23;
} Voice;

typedef struct {
    u8 f00[0x14];
    u8 *ptr;
    u8 f18[0x0c];
    u8 numVoices;
    u8 f25[0x17];
    Voice voices[1];
} Snd;

extern Snd *data_0210daf0;

void func_020b601c(u8 arg0, u8 arg1);
void func_020b650c(u8 arg0);
void func_020b67f8(u8 index);

enum {
    NONE            = 0xff,
    KIND_SHIFT      = 6,
    CHANNEL_MASK    = 0x3f,
    NOTE_HAS_VOL    = 0x80,
    NOTE_MASK       = 0x7f,
    MASK_NOTE       = 0x20,
    MASK_INSTR      = 0x40,
    MASK_VOL        = 0x80,
    MASK_EFFECT     = 0x1f,
    CHANGED_NOTE    = 0x01,
    CHANGED_INSTR   = 0x02,
    CHANGED_VOL     = 0x04,
    CHANGED_EFFECT  = 0x08,
    CHANGED_DELAYED = 0x10,
    FX_TONE_PORTA   = 0x07,   /* IT effect G */
    FX_S            = 0x13,            /* IT effect S */
    FX_S_NOTE_DELAY = 0xd,  /* S-Dx */
    VOL_PORTA_FIRST = 0xc1,
    VOL_PORTA_LAST  = 0xca
};

/*
 * Decodes one packed pattern row (Impulse Tracker style) from the byte
 * stream at data_0210daf0->ptr. Every voice's change-flag byte is cleared
 * first. Each entry byte holds the channel in bits 0-5 and the entry kind
 * in bits 6-7: 1 = volume-column byte, 2 = note byte (bit 7 set: a volume
 * byte follows) then an instrument byte (0 = keep), 3 = mask byte (effect
 * in bits 0-4, bits 5/6/7 = note/instrument/volume follow) then the effect
 * parameter. Changed fields are stored in the voice with their flag bits.
 * Effect S-Dx (note delay) with x = 0 reuses the voice's stored delay
 * nibble; a nonzero delay marks the voice delayed and skips its update.
 * Other voices get the three per-voice updates; the first one is told
 * whether tone portamento applies (effect G, or volume column 193-202).
 * The row's 0 terminator is skipped on exit.
 */
void func_020b5d3c(void) {
    s32 i;

    for (i = data_0210daf0->numVoices - 1; i >= 0; i--) {
        data_0210daf0->voices[i].flags = 0;
    }

    while (*data_0210daf0->ptr != 0) {
        u32 c;
        u32 ch;
        u8 delayed;
        Voice *v;
        u8 note;
        u8 instr;
        u8 vol;
        u32 cmd;
        u8 param;
        c  = *data_0210daf0->ptr++;
        ch = c & CHANNEL_MASK;
        v  = &data_0210daf0->voices[ch];

        note    = NONE;
        instr   = NONE;
        vol     = NONE;
        cmd     = NONE;
        param   = NONE;
        delayed = 0;
        switch (c >> KIND_SHIFT) {
            case 0:
                break;
            case 1:
                vol = *data_0210daf0->ptr++;
                break;
            case 2:
                note = *data_0210daf0->ptr++;
                if (*data_0210daf0->ptr != 0) {
                    instr = *data_0210daf0->ptr;
                }
                data_0210daf0->ptr++;
                if (note & NOTE_HAS_VOL) {
                    note &= NOTE_MASK;
                    vol = *data_0210daf0->ptr++;
                }
                break;
            case 3:
                cmd   = *data_0210daf0->ptr++;
                param = *data_0210daf0->ptr++;
                if (cmd & MASK_NOTE) {
                    note = *data_0210daf0->ptr++;
                }
                if (cmd & MASK_INSTR) {
                    instr = *data_0210daf0->ptr++;
                }
                if (cmd & MASK_VOL) {
                    vol = *data_0210daf0->ptr++;
                }
                cmd &= MASK_EFFECT;
                break;
        }
        if (instr != NONE) {
            v->instr = instr;
            v->flags |= CHANGED_INSTR;
        }
        if (note != NONE) {
            v->note = note;
            v->flags |= CHANGED_NOTE;
        }
        if (vol != NONE) {
            v->volcmd = vol;
            v->flags |= CHANGED_VOL;
        }
        if (cmd != NONE) {
            v->cmd   = cmd;
            v->param = param;
            v->flags |= CHANGED_EFFECT;
        }
        if (v->flags & CHANGED_EFFECT) {
            if (v->cmd == FX_S && (v->param >> 4) == FX_S_NOTE_DELAY) {
                if ((v->param & 0xf) == 0) {
                    v->param |= v->delay;
                } else {
                    v->delay = v->param & 0xf;
                }
                if (v->param & 0xf) {
                    v->flags |= CHANGED_DELAYED;
                    delayed = 1;
                }
            }
        }
        if (!delayed) {
            func_020b601c(ch, cmd == FX_TONE_PORTA || (vol >= VOL_PORTA_FIRST && vol <= VOL_PORTA_LAST));
            func_020b650c(ch);
            func_020b67f8(ch);
        }
    }
    data_0210daf0->ptr++;
}
