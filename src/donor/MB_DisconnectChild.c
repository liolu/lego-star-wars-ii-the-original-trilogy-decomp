/* compiler: 2.0/sp1p5 -O4,p -lang c99 -ipa file -inline on,noauto */
/* MB_DisconnectChild from https://github.com/ctrl-cheeb-del/pokeheartgold lib/NitroSDK/src/mb/mb_disconnect.c (license: none),
 * reused by tools/donor_match.py */
typedef unsigned char u8;
typedef unsigned short int u16;
typedef unsigned long u32;
typedef signed char s8;
extern void MI_CpuFill8(void *dest, u8 data, u32 size);
typedef struct {
    u8 data[22];
} ChildPiece;
typedef struct {
    u8 data[0x5c4];
    u16 f5c4, f5c6, f5c8, f5ca, f5cc, f5ce, f5d0, f5d2;
} Game;
typedef struct {
    u8 prefix[0x1340];
    ChildPiece piece[15];
    u16 state[15];
    u32 four[15];
    u32 callback;
    u32 request[15];
    u8 unknown[2];
    s8 game[15];
    u8 connectedCount;
    u16 connected;
    u8 buffer[0x21c];
    u16 status[15];
    u8 pad[22];
    Game games[16];
} Parent;
typedef struct {
    u32 pad[3];
    Parent *parent;
} MbCtx;
extern MbCtx WM_DMA_NO;
extern void MBi_ParentCallback(void *);
extern int WM_Disconnect(void (*)(void *), u16);
extern void MBi_ClearParentPieceBuffer(u16);
extern void MB_DisconnectChild(u16 aid);
typedef struct {
    u8 pad0[0x1d4c];
    u16 f4c;
    u16 f4e;
    u16 f50;
} GameSlot;
void MB_DisconnectChild(u16 aid) {
    WM_Disconnect(MBi_ParentCallback, aid);
    if (aid == 0 || aid >= 16) {
        return;
    }
    (WM_DMA_NO.parent)->state[aid - 1] = 0;
    MI_CpuFill8(&(WM_DMA_NO.parent)->four[aid - 1], 0, 4);
    MI_CpuFill8(&(WM_DMA_NO.parent)->piece[aid - 1], 0, sizeof(ChildPiece));
    MBi_ClearParentPieceBuffer(aid);
    (WM_DMA_NO.parent)->status[aid - 1] = 0;
    if ((WM_DMA_NO.parent)->game[aid - 1] != -1) {
        u8 slot = (WM_DMA_NO.parent)->game[aid - 1];
        ((GameSlot *) ((u8 *) (WM_DMA_NO.parent) + (slot) * 0x5d4))->f4e &= ~(1 << aid);
        ((GameSlot *) ((u8 *) (WM_DMA_NO.parent) + (slot) * 0x5d4))->f50 |= 1 << aid;
        (WM_DMA_NO.parent)->game[aid - 1] = -1;
        ((GameSlot *) ((u8 *) (WM_DMA_NO.parent) + (slot) * 0x5d4))->f4c &= ~(1 << aid);
    }
    if ((WM_DMA_NO.parent)->connected & (1 << aid)) {
        (WM_DMA_NO.parent)->connectedCount--;
        (WM_DMA_NO.parent)->connected &= ~(1 << aid);
    }
    (WM_DMA_NO.parent)->request[aid - 1] = 0;
}
