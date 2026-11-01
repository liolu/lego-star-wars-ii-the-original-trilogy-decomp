#include "m2c.h"

typedef struct {
    u8 kind;
    s32 b;
    s32 c;
} S0;

typedef struct {
    u8 f00;
    u8 f01;
    u8 f02;
    u8 f03;
    u8 f04;
    u8 f05;
    u8 f06 : 1;
    u8 f06b : 7;
    u8 f07;
    s32 f08;
    s32 f0c;
    s32 f10;
    s32 f14;
    s32 f18;
} SndCmd;

typedef struct {
    void *ptr;
    u16 id;
    s8 refs;
    u8 f07;
    u8 flags;
    u8 f09;
    u8 f0a;
    u8 pad0b;
    s32 count;
    s32 f10;
    s32 f14;
    s32 f18;
} SndSlot;

typedef struct {
    u8 pad00[0x14];
    u8 chanPrio[0x14];
    SndCmd cmds[12];
    u8 pad178[0x178 - 0x28 - 12 * 0x1C];
    SndSlot slots[1];
} SndMgr;

extern SndMgr data_0210dc20;
extern s32 data_0210db84;

extern s32 func_020ba9f0(u16 id, S0 arg);
extern void func_020ba9b0(void *arg0);
extern s32 func_020bb388(s32 arg0, s32 arg1);
extern void func_020bb3f8(s32 arg0, s32 arg1);
extern void func_020bada8(M2C_UNK, M2C_UNK);
extern void SND_StartTimer(M2C_UNK arg0, M2C_UNK arg1, M2C_UNK arg2, s32 arg3);
extern u32 SND_GetCurrentCommandTag(void);

enum {
    SND_ID_NONE      = 0xFFFF, /* id meaning "no sound": nothing is started */
    SND_NONE         = 0xFF,   /* no free command / slot / channel */
    SND_CMD_LAST     = 11,     /* the 12 commands are scanned from 11 down to 0 */
    CMD_STATE_FREE   = 0,
    CMD_STATE_ACTIVE = 3,      /* polled by func_020bb26c until its tag finishes */
    SND_OWNER_CMD    = 0x20,   /* channel owner class for commands (| index) */
    SLOT_FLAG_10     = 0x10,   /* slot flag: channel requested at priority 0xFF */
    SND_PRIO_BIAS    = 0xBE,   /* added to cmd->f03 to form the channel priority */
    SND_SERIAL_SHIFT = 8,      /* handle = index | serial << 8 */
};

/*
 * Starts sound `id` on a free command entry. Loads the sound into a slot
 * (func_020ba9f0), fills the command (f03/f04/f08 default to the slot's
 * f09/f0a/f18 when passed as 0xFF/0xFF/0), stamps it with the next serial
 * from data_0210db84, then allocates a channel with priority prio
 * (func_020bb388), claims it and starts its timer. Returns the handle
 * index | serial << 8, or -1 if no command, slot or channel is available
 * (on channel failure the slot is released and the command freed again).
 */
s32 func_020bb4fc(s32 id, s32 param, s32 a, s32 b, S0 arg, s32 arg2) {
    s32 idx;
    SndCmd *cmd;
    SndSlot *snd;
    s32 i;
    s32 prio;

    if (id == SND_ID_NONE) {
        return -1;
    }
    idx = SND_NONE;
    for (i = SND_CMD_LAST; i >= 0; i--) {
        if (data_0210dc20.cmds[i].f01 == CMD_STATE_FREE) {
            idx = i;
            break;
        }
    }
    if (idx == SND_NONE) {
        return -1;
    }
    cmd      = &data_0210dc20.cmds[idx];
    cmd->f00 = func_020ba9f0(id, arg);
    if (cmd->f00 == SND_NONE) {
        return -1;
    }
    snd      = &data_0210dc20.slots[cmd->f00];
    cmd->f01 = CMD_STATE_ACTIVE;
    cmd->f14 = arg2;
    cmd->f10 = data_0210db84++;
    if (a == SND_NONE) {
        a = snd->f09;
    }
    cmd->f03 = a;
    if (b == SND_NONE) {
        b = snd->f0a;
    }
    cmd->f04 = b;
    if (param == 0) {
        param = snd->f18;
    }
    cmd->f08 = param;
    cmd->f0c = param;
    cmd->f05 = 0;
    cmd->f06 = 1;
    cmd->f18 = 0;
    if (snd->flags & SLOT_FLAG_10) {
        prio = SND_NONE;
    } else {
        prio = cmd->f03 + SND_PRIO_BIAS;
    }
    cmd->f02 = func_020bb388(idx | SND_OWNER_CMD, prio);
    if (cmd->f02 != SND_NONE) {
        func_020bb3f8(cmd->f02, idx | SND_OWNER_CMD);
        data_0210dc20.chanPrio[cmd->f02] = prio;
        func_020bada8(cmd->f02, snd);
        SND_StartTimer(1 << cmd->f02, 0, 0, 0);
        cmd->f18 = SND_GetCurrentCommandTag();
        return idx | (cmd->f10 << SND_SERIAL_SHIFT);
    }
    func_020ba9b0(snd);
    cmd->f01 = CMD_STATE_FREE;
    return -1;
}
