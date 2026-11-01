/* Reconstructed from USA ARM9 at 0x020b82e8 (820 bytes).
 *
 * Sound channel volume and pan update (sibling of func_020b810c, which sets
 * the timer).  arg0 selects one of the 16 channel records at +0x4bc of the
 * sound object (data_0210daf0, 0x20 bytes each); record byte +0xa is the
 * voice slot index (0xff = none, return).  If record flag 0x80 is set and the
 * slot type is 9, the channel is simply silenced.
 *
 * Otherwise the volume is the product of a global byte, two sound object
 * bytes, the entry/group/slot volume bytes and the record's 16-bit volume.
 * When record flag 0x08 is set it is scaled by a linearly interpolated
 * envelope point (the hardware divider computes the slope); flag 0x10 does
 * the same for the pan, which starts from the record pan byte plus the
 * entry's (byte +4 - centre) * scale / 8 and is clamped to 0..64.  The
 * volume is stored (halved for flag 0x02) in data_0210dc34[arg0], split into
 * a volume and SND_SetChannelVolume shift (3..0), and both values are sent
 * to the channel.  Finally bit 1 of the 2-bit field at +0x11 is cleared.
 */
#include "m2c.h"

typedef struct {
    u8 flag : 2;
    u8 rest : 6;
} RecFlags;

typedef volatile u16 vu16;
typedef volatile u32 vu32;
typedef volatile s32 vs32;
typedef volatile u64 vu64;

enum {
    NO_VOICE_SLOT  = 0xff,
    REG_DIVCNT     = 0x4000280,
    REG_DIV_NUMER  = 0x4000290,
    REG_DIV_DENOM  = 0x4000298,
    REG_DIV_RESULT = 0x40002a0,
    DIVCNT_BUSY    = 0x8000,
};

typedef struct {
    u16 *xs;
    s8 *ys;
} Curve;

typedef struct {
    u8 pad0[6];
    s8 pan_scale;
    u8 pan_center;
    u8 volume;
    u8 pad9[0x14 - 9];
    Curve *vol_curve;
    Curve *pan_curve;
    u8 pad1c[4];
} Ent;

typedef struct {
    u8 pad0[4];
    u8 pan_in;
    u8 ent;
    u8 vol_scale;
    u8 pan_base;
    u8 pad8;
    u8 grp;
    u8 slot;
    u8 flags;
    u8 padc[4];
    u8 flags2;
    RecFlags bits;
    u16 volume;
    u16 vol_x;
    u8 vol_i;
    u8 pad17;
    u16 pan_x;
    u8 pan_i;
    u8 pad1b[5];
} Rec;

typedef struct {
    u8 pad0[3];
    u8 type;
    u8 pad4[2];
    u8 volume;
    u8 pad7[0x24 - 7];
} Slot;

typedef struct {
    u8 pad0[4];
    u8 volume;
    u8 pad5[0x24 - 5];
} Grp;

typedef struct {
    Grp *grps;
    Ent *ents;
    u8 pad8[0x2a - 8];
    u8 vol_a;
    u8 pad2b[0x36 - 0x2b];
    u8 vol_b;
    u8 pad37[0x3c - 0x37];
    Slot slots[32];
    Rec recs[16];
} Snd;

extern Snd *data_0210daf0;
extern u8 data_0210dc20[];
extern u8 data_0210dc34[];

void SND_SetChannelVolume(s32, s32, s32);
void SND_SetChannelPan(s32, s32);

static inline void div_start(s32 num, s32 den) {
    *(vu16 *) REG_DIVCNT    = 0;
    *(vs32 *) REG_DIV_NUMER = num;
    *(vu64 *) REG_DIV_DENOM = (u32) den;
}

static inline s32 div_result(void) {
    while (*(vu16 *) REG_DIVCNT & DIVCNT_BUSY) {
    }
    return *(vs32 *) REG_DIV_RESULT;
}

void func_020b82e8(u8 arg0) {
    Snd *obj;
    Rec *rec;
    Slot *slot;
    s32 pt;
    Curve *curve;
    u32 ent_idx;
    Ent *ents;
    u8 slot_idx;
    u8 flags;
    s32 vol;
    s32 pan;
    s32 shift;
    s32 chan_mask;

    curve    = 0;
    pt       = 0;
    obj      = data_0210daf0;
    rec      = &obj->recs[arg0];
    slot_idx = rec->slot;
    slot     = &obj->slots[slot_idx];
    if (slot_idx == NO_VOICE_SLOT) {
        return;
    }

    if ((rec->flags2 & 0x80) && slot->type == 9) {
        SND_SetChannelVolume(1 << arg0, 0, 0);
        return;
    }

    if (rec->flags & 8) {
        u16 *xs;
        s8 *ys;
        s32 x0;
        curve = obj->ents[rec->ent].vol_curve;
        pt    = rec->vol_i;
        xs    = curve->xs;
        ys    = curve->ys;
        x0    = xs[pt];
        div_start(((rec->vol_x - x0) * (ys[pt + 1] - ys[pt])) << 8, xs[pt + 1] - x0);
    }

    obj     = data_0210daf0;
    ent_idx = rec->ent;
    ents    = obj->ents;
    vol     = ((data_0210dc20[0x24] * obj->vol_a) * ents[ent_idx].volume * obj->grps[rec->grp].volume) >> 19;
    vol     = (vol * (rec->volume * (obj->vol_b * slot->volume))) >> 23;
    flags   = rec->flags;
    if (flags & 8) {
        vol *= div_result() + (curve->ys[pt] << 8);
        vol = (vol * rec->vol_scale) >> 4;
    } else {
        vol = vol * (rec->vol_scale << 10);
    }

    if (flags & 0x10) {
        u16 *xs;
        s8 *ys;
        s32 x0;
        curve = ents[ent_idx].pan_curve;
        pt    = rec->pan_i;
        xs    = curve->xs;
        ys    = curve->ys;
        x0    = xs[pt];
        div_start((rec->pan_x - x0) * (ys[pt + 1] - ys[pt]), xs[pt + 1] - x0);
    }

    pan = rec->pan_base +
          (((rec->pan_in - data_0210daf0->ents[rec->ent].pan_center) * data_0210daf0->ents[rec->ent].pan_scale) >> 3);
    if (rec->flags & 0x10) {
        s32 env;
        env = div_result();
        env += curve->ys[pt];
        if (rec->pan_base < 0x20) {
            env = env * rec->pan_base;
        } else {
            env = env * (0x40 - rec->pan_base);
        }
        pan += env >> 5;
    }
    if (pan < 0) {
        pan = 0;
    } else if (pan > 0x40) {
        pan = 0x40;
    }

    vol                 = (vol >> 13) - (vol >> 20);
    data_0210dc34[arg0] = vol >> 4;
    if (rec->flags & 2) {
        data_0210dc34[arg0] = data_0210dc34[arg0] >> 1;
    }
    if (vol < 0x80) {
        shift = 3;
    } else if (vol < 0x200) {
        vol >>= 2;
        shift = 2;
    } else if (vol < 0x400) {
        vol >>= 3;
        shift = 1;
    } else {
        vol >>= 4;
        shift = 0;
    }
    chan_mask = 1 << arg0;
    SND_SetChannelVolume(chan_mask, vol, shift);
    SND_SetChannelPan(chan_mask, (pan << 1) - (pan >> 6));
    rec->bits.flag &= ~2;
}
