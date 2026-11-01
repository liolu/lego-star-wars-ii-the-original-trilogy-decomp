/* compiler: 2.0/sp1p5 -O4,p -lang c99 -ipa file -inline on,noauto */
/* MBi_SendVolatBeacon from https://github.com/ctrl-cheeb-del/pokeheartgold lib/NitroSDK/src/mb/mb_beacon_volat.c (license:
 * none), reused by tools/donor_match.py */
typedef unsigned char u8;
typedef unsigned short int u16;
typedef unsigned long u32;
extern void MIi_CpuClear16(register u16 value, register u16 *dst, register u32 size);
extern void MIi_CpuCopy16(const register u16 *src, register u16 *dst, register u32 size);
typedef struct {
    u8 data[0x16];
} BeaconMember;
typedef struct {
    u8 pad0[0x358];
    u8 f358;
    u8 pad359;
    u16 f35a;
    u16 f35c;
    BeaconMember members[15];
    u8 pad[0x4a8 - 0x35e - 15 * 0x16];
    u8 f4a8[8];
    u16 f4b0;
    u8 pad4b2;
    u8 f4b3;
    u8 f4b4;
    u8 f4b5;
    u8 pad4b6[2];
    u32 f4b8;
} BeaconGameInfo;
typedef struct {
    u8 pad0[0x10];
    u32 mode;
    void (*callback)(u32);
    u8 pad18[4];
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
    u8 pad4b;
    u16 f4c;
    u16 f4e;
} BeaconState;
extern BeaconState sUnlockFunc;
extern u8 bsendBuff[];
extern u16 func_020daa20(void *buf, u32 len);
extern void MBi_InitSendVolatBeacon(void);
extern int WM_SetGameInfo(void *callback, void *buf, u16 size, u16 a, u32 b, u8 c);
extern void MBi_SendVolatBeacon(u16 a, u32 b, u32 c);
#pragma push
#pragma optimization_level 2
#pragma opt_common_subs off
void MBi_SendVolatBeacon(u16 a, u32 b, u32 c) {
    int i;
    u32 n;
    u16 diff;
    if (sUnlockFunc.f26 != sUnlockFunc.gameInfo->f4b4) {
        MBi_InitSendVolatBeacon();
    }
    sUnlockFunc.lo  = 2;
    sUnlockFunc.f45 = sUnlockFunc.gameInfo->f4b3;
    sUnlockFunc.f46 = sUnlockFunc.f26;
    sUnlockFunc.f40 = sUnlockFunc.gameInfo->f4b8;
    sUnlockFunc.hi  = sUnlockFunc.gameInfo->f4b5;
    sUnlockFunc.f47 = sUnlockFunc.counter++;
    sUnlockFunc.f4a = sUnlockFunc.gameInfo->f358;
    sUnlockFunc.f4c = sUnlockFunc.gameInfo->f35a;
    sUnlockFunc.f4e = sUnlockFunc.gameInfo->f35c;
    if (sUnlockFunc.mode == 0 && sUnlockFunc.callback != ((void *) 0)) {
        sUnlockFunc.callback(sUnlockFunc.gameInfo->f4b8);
    }
    for (i = 0; i < 8; i++) {
        (bsendBuff + 0x68)[i] = sUnlockFunc.gameInfo->f4a8[i];
    }
    MIi_CpuClear16(0, (u16 *) (bsendBuff + 0x10), 0x58);
    diff = sUnlockFunc.gameInfo->f4b0 ^ sUnlockFunc.gameInfo->f35a;
    n    = 0;
    for (i = 0; i < 15; i++) {
        if ((2 << i) & diff) {
            MIi_CpuCopy16((u16 *) &sUnlockFunc.gameInfo->members[i], (u16 *) (bsendBuff + 0x10 + n * 0x16), 0x16);
            n++;
            sUnlockFunc.gameInfo->f4b0 |= 2 << i;
            if (n == 4) {
                break;
            }
        }
    }
    if (n < 4) {
        bsendBuff[0x10 + n * 0x16] &= ~0xf0;
    }
    sUnlockFunc.sum = 0;
    sUnlockFunc.sum = func_020daa20(bsendBuff + 8, 0x68);
    if (sUnlockFunc.gameInfo->f4b0 == sUnlockFunc.gameInfo->f35a) {
        sUnlockFunc.state = 1;
    }
    WM_SetGameInfo(((void *) 0), bsendBuff, 0x70, a, b, c | 3);
    if (sUnlockFunc.mode == 1 && sUnlockFunc.callback != ((void *) 0)) {
        sUnlockFunc.callback(sUnlockFunc.gameInfo->f4b8);
    }
}
