/* compiler: 2.0/sp1p5 -O4,p -lang c99 -ipa file -inline on,noauto */
/* MBi_SendFixedBeacon from https://github.com/ctrl-cheeb-del/pokeheartgold lib/NitroSDK/src/mb/mb_beacon_fixed.c (license:
 * none), reused by tools/donor_match.py */
typedef unsigned char u8;
typedef unsigned short int u16;
typedef unsigned long u32;
extern void MIi_CpuClear16(register u16 value, register u16 *dst, register u32 size);
extern void MIi_CpuCopy16(const register u16 *src, register u16 *dst, register u32 size);
typedef struct {
    u8 pad0[0x4b2];
    u8 f4b2;
    u8 f4b3;
    u8 pad1;
    u8 f4b5;
    u8 pad2[2];
    u32 f4b8;
} BeaconGameInfo;
typedef struct {
    u8 pad0[0x1c];
    BeaconGameInfo *gameInfo;
    u8 *src;
    u8 state;
    u8 pad25;
    u8 f26;
    u8 seqNo;
    u8 seqMax;
    u8 counter;
    u8 pad2a[0x16];
    u32 f40;
    u8 lo : 2;
    u8 hi : 6;
    u8 f45;
    u8 f46;
    u8 f47;
    u16 sum;
    u8 f4a;
    u8 f4b;
    u8 len;
} BeaconState;
extern BeaconState sUnlockFunc;
extern u8 bsendBuff[];
extern u16 func_020daa20(void *buf, u32 len);
extern int WM_SetGameInfo(void *callback, void *buf, u16 size, u16 a, u32 b, u8 c);
extern void MBi_SendFixedBeacon(u16 a, u32 b, u32 c);
#pragma push
#pragma optimization_level 2
#pragma opt_common_subs off
void MBi_SendFixedBeacon(u16 a, u32 b, u32 c) {
    u32 end = (u32) sUnlockFunc.gameInfo + 0x358;
    u32 len = (u32) sUnlockFunc.src + 0x62;
    if (len <= end) {
        sUnlockFunc.len = 0x62;
    } else {
        len             = end - (u32) sUnlockFunc.src;
        sUnlockFunc.len = len;
        {
            u8 n = sUnlockFunc.len;
            MIi_CpuClear16(0, (u16 *) (bsendBuff + 0xE + n), 0x62 - n);
        }
    }
    MIi_CpuCopy16((u16 *) sUnlockFunc.src, (u16 *) (bsendBuff + 0xE), sUnlockFunc.len);
    sUnlockFunc.f4a = sUnlockFunc.seqNo;
    sUnlockFunc.f4b = sUnlockFunc.seqMax;
    sUnlockFunc.lo  = sUnlockFunc.gameInfo->f4b2;
    sUnlockFunc.f45 = sUnlockFunc.gameInfo->f4b3;
    sUnlockFunc.f46 = sUnlockFunc.f26;
    sUnlockFunc.f40 = sUnlockFunc.gameInfo->f4b8;
    sUnlockFunc.hi  = sUnlockFunc.gameInfo->f4b5;
    sUnlockFunc.f47 = sUnlockFunc.counter++;
    sUnlockFunc.sum = 0;
    sUnlockFunc.sum = func_020daa20(bsendBuff + 8, 0x68);
    if (++sUnlockFunc.seqNo < sUnlockFunc.seqMax) {
        sUnlockFunc.src += 0x62;
    } else {
        sUnlockFunc.state = 4;
    }
    WM_SetGameInfo(((void *) 0), bsendBuff, 0x70, a, b, c | 3);
}
