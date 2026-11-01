/* compiler: 2.0/sp1p5 -O4,p -lang c99 -ipa file -inline on,noauto */
/* MB_RegisterFile from https://github.com/ctrl-cheeb-del/pokeheartgold lib/NitroSDK/src/mb/mb_download_info.c (license: none),
 * reused by tools/donor_match.py */
typedef unsigned char u8;
typedef unsigned short int u16;
typedef unsigned long u32;
typedef signed short int s16;
typedef enum {
    OS_INTRMODE_DISABLE_IRQ = 0x80,
    OS_INTRMODE_DISABLE_FIQ = 0x40,
    OS_INTRMODE_ENABLE      = 0
} OSIntrMode;
extern OSIntrMode OS_DisableInterrupts(void);
extern OSIntrMode OS_RestoreInterrupts(OSIntrMode state);
extern void MI_CpuCopy8(const void *src, void *dst, u32 size);
typedef struct Info {
    u32 a, b, pad;
    char segments[48], tail[136];
} Info;
typedef struct SegmentInfo {
    u32 destination, load, size, extra;
} SegmentInfo;
typedef struct Download {
    u32 fields[3];
    SegmentInfo segments[3];
} Download;
typedef struct BlockTable {
    u32 offsets[3];
    u16 boundaries[3], count;
} BlockTable;
extern char *mbc;
typedef struct W36016Cache {
    u8 bytes[0x6c];
    u32 available;
} W36016Cache;
typedef struct W36016Data {
    u8 prefix[0x1e8];
    W36016Cache part1;
    u8 part2[];
} W36016Data;
extern u8 data_02118944;
typedef struct GameInfo {
    u8 prefix[0x4b3];
    u8 number, padding, index;
    u8 gap4b6[6];
    struct GameInfo *next;
} GameInfo;
typedef struct {
    u8 download[0xe4];
    GameInfo game;
    u8 blocks[20];
    void *file;
    W36016Data *data;
    u8 gap5c0[6];
    u16 active;
    u8 gap5c8[2], used, gap5cb;
    W36016Cache *cache;
    u8 *payload;
} Slot;
typedef struct {
    u8 prefix[0x1300];
    u8 gameUser[0x224];
    u8 count;
    u8 gap1525[0x263];
    Slot slots[16];
    u8 gap74c8[24];
    u8 taskThread[0x800];
    u8 taskInfo[0x20];
} Context;
extern void MBi_InitTaskInfo(void *p);
typedef struct {
    u32 x;
    const u16 *name, *description;
    void *icon1, *icon2;
    u32 game;
    u8 flag;
} Entry;
extern inline Slot *GetSlot(u32 n);
extern int MB_RegisterFile(register void *file, W36016Data *data);
extern void func_020da508(Info *out, char *header);
extern int MBi_MakeBlockInfoTable(BlockTable *out, Download *info);
extern void MBi_MakeGameInfo(void *info, Entry *e, const void *extra);
extern void MB_AddGameInfo(void *p);
static inline Slot *GetSlot(u32 n) {
    return (Slot *) (mbc + 0x1788 + n * 0x5d4);
}
extern int MBi_IsStarted(void);
extern int MBi_IsTaskAvailable(void);
extern void func_020dd0dc(void *, u32);
int MB_RegisterFile(register void *file, W36016Data *data) {
    u8 free = 255;
    OSIntrMode flags;
    u8 i;
    u32 offset;
    W36016Cache *cache;
    void *info;
    flags = OS_DisableInterrupts();
    if (!MBi_IsStarted()) {
        OS_RestoreInterrupts(flags);
        return 0;
    }
    if ((*((u8 *) (mbc) + (0x1524))) + 1 > 16) {
        OS_RestoreInterrupts(flags);
        return 0;
    }
    for (i = 0; i < 16; i++) {
        u8 *cursor = (u8 *) (Context *) mbc + i * 0x5d4;
        if (*(void **) (cursor + 0x1d40) == file) {
            OS_RestoreInterrupts(flags);
            return 0;
        }
        if (!cursor[0x1d52]) {
            free = i;
            break;
        }
    }
    if (i == 16) {
        OS_RestoreInterrupts(flags);
        return 0;
    }
    offset              = free * 0x5d4;
    GetSlot(free)->file = file;
    info                = GetSlot(free)->download;
    func_020da508(info, (char *) data);
    MI_CpuCopy8((u8 *) file + 0x1c, (u8 *) info + 0xc4, 32);
    if (!MBi_MakeBlockInfoTable((BlockTable *) (mbc + 0x1d2c + offset), info)) {
        OS_RestoreInterrupts(flags);
        return 0;
    }
    MBi_MakeGameInfo((GameInfo *) (mbc + 0x186c + offset), file, mbc + 0x1300);
    GetSlot(free)->game.index = free;
    MB_AddGameInfo((GameInfo *) (mbc + 0x186c + offset));
    {
        s16 number;
        number                     = data_02118944;
        GetSlot(free)->game.number = number;
        GetSlot(free)->active      = 1;
        {
            cache                  = &data->part1;
            GetSlot(free)->data    = data;
            u8 *payload            = data->part2;
            GetSlot(free)->cache   = cache;
            number                 = number + 1;
            GetSlot(free)->payload = payload;
            data_02118944          = number;
        }
    }
    if (GetSlot(free)->cache->available && !MBi_IsTaskAvailable()) {
        MBi_InitTaskInfo(mbc + 0x7ce0);
        func_020dd0dc(mbc + 0x74e0, 0x800);
    }
    GetSlot(free)->used = 1;
    (*((u8 *) (mbc) + (0x1524)))++;
    OS_RestoreInterrupts(flags);
    return 1;
}
void func_020da508(Info *out, char *header);
int MBi_MakeBlockInfoTable(BlockTable *out, Download *info);
void MBi_MakeGameInfo(void *info, Entry *e, const void *extra);
void MB_AddGameInfo(void *p);
