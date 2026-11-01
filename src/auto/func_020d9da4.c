/* compiler: 2.0/sp1p5 -O4,p -lang c99 -ipa file -inline on,noauto */
typedef unsigned char u8;
typedef unsigned long u32;
typedef signed char s8;
typedef signed long s32;
typedef int BOOL;

typedef struct FSArchive {
    char name[4];
} FSArchive;

typedef struct FSFile {
    u32 link[2];
    FSArchive *arc;
    u32 stat[6];
    u32 top;
    u32 bottom;
    u32 pos;
    u32 rest[6];
} FSFile;

typedef struct RomHeader {
    u8 head[0x20];
    u32 main_rom_offset;
    u32 main_entry_address;
    u32 main_ram_address;
    u32 main_size;
    u32 sub_rom_offset;
    u32 sub_entry_address;
    u32 sub_ram_address;
    u32 sub_size;
    u8 pad40[0x20];
    u32 rom_ctrl;
    u8 pad64[0x1c];
    u32 own_size;
    u8 pad84[0xdc];
} RomHeader;

typedef struct MBiCacheInfo {
    u32 src;
    u32 len;
    u8 *ptr;
    u32 state;
} MBiCacheInfo;

typedef struct MBiCacheList {
    u32 lifetime;
    u32 recent;
    MBiCacheInfo *p_list;
    u32 size;
    char arc_name[4];
    u32 arc_name_len;
    FSArchive *arc_pointer;
    u8 reserved[20];
    MBiCacheInfo list[4];
} MBiCacheList;

typedef struct MbSegmentInfo {
    u32 reserved;
    u32 arm9_src;
    u32 arm7_src;
    u32 pad;
} MbSegmentInfo;

typedef struct SegmentContext {
    u32 rom_offset;
    u32 ram_delta;
    u32 buf_delta;
    u32 len;
} SegmentContext;

extern void DC_FlushRange(const void *start, u32 size);
extern void MI_CpuCopy8(const void *src, void *dest, u32 size);
extern FSArchive *FS_FindArchive(const char *name, s32 name_len);
extern void func_020d2898(FSFile *file);
extern s32 FS_OpenFileDirect(FSFile *file, FSArchive *arc, u32 top, u32 bottom, u32 id);
extern s32 func_020d2bc0(FSFile *file);
extern s32 func_020d2cbc(FSFile *file, void *dst, s32 len);
extern BOOL FS_SeekFile(FSFile *file, s32 offset, s32 origin);
extern void MBi_ReadSegmentHeader(SegmentContext *ctx, u32 start, u32 end, BOOL first);
extern void MBi_InitCache(MBiCacheList *pl);
extern void func_020dcec4(MBiCacheList *pl, u32 src, u32 len, void *ptr, u32 state);
extern void AutoloadCallback(void);
extern char data_02101568[];
extern const u32 *data_02101564; /* autoload table: {start, size} pairs, size 0 ends */

enum {
    HW_ROM_HEADER_BUF = 0x027ffe00, /* this binary's own ROM header in main RAM */
    ROM_HEADER_SIZE   = 0x160,
    CACHE_BLOCK_SIZE  = 0x4400,      /* partial ARM9 read: three blocks of this size */
    INSN_BX_LR        = 0xe12fff1e         /* ARM encoding of "bx lr" */
};

/*
 * NitroSDK MB_ReadSegment: fill buf with a download-play segment image.
 * Layout: ROM header (0x160), 0x88 bytes read after the ROM image (own_size),
 * an MBiCacheList (0x70), an MbSegmentInfo (0x10) with the ARM9/ARM7 source
 * offsets, then the ARM9+ARM7 binaries (or the first 3 ARM9 blocks).
 * file == NULL reads this program's own image through the "rom" archive; in
 * that case the cached ARM9 image is walked for its autoload segments and its
 * AutoloadCallback is patched to return at once. Returns 1 if binaries were read.
 */

BOOL func_020d9da4(FSFile *file, void *buf, u32 len) {
    BOOL ret = 0;

    if (len >= ROM_HEADER_SIZE + 4) {
        BOOL is_own_binary = 0;
        u32 own_offset;
        u32 own_size;
        u32 rest              = len;
        u8 *bufp              = (u8 *) buf;
        MBiCacheList *p_cache = 0;
        MbSegmentInfo *seg    = 0;
        RomHeader *hdr        = (RomHeader *) buf;
        FSFile own_file;
        bufp += ROM_HEADER_SIZE;
        rest -= ROM_HEADER_SIZE;

        if (file != 0) {
            own_offset = file->pos - file->top;
            if (func_020d2cbc(file, buf, ROM_HEADER_SIZE) < ROM_HEADER_SIZE) {
                rest = 0;
            }
            own_size = hdr->own_size;
            if (own_size == 0) {
                own_size = 0x1000000;
            }
        } else {
            {
                const RomHeader *own_hdr = (const RomHeader *) HW_ROM_HEADER_BUF;
                own_size                 = own_hdr->own_size;
            }
            if (own_size == 0) {
                own_size = 0x1000000;
            }
            is_own_binary = 1;
            func_020d2898(&own_file);
            FS_OpenFileDirect(&own_file, FS_FindArchive(data_02101568, 3), 0, own_size + 0x88, (u32) -1);
            file       = &own_file;
            own_offset = file->pos - file->top;
            MI_CpuCopy8((void *) HW_ROM_HEADER_BUF, buf, ROM_HEADER_SIZE);
            hdr->rom_ctrl |= 0x406000;
        }

        if (rest >= 0x88) {
            FS_SeekFile(file, own_offset + own_size, 0);
            func_020d2cbc(file, bufp, 0x88);
            bufp += 0x88;
            rest -= 0x88;
        } else {
            rest = 0;
        }

        if (rest >= 0x70) {
            p_cache = (MBiCacheList *) bufp;
            MBi_InitCache(p_cache);
            bufp += 0x70;
            rest -= 0x70;
            func_020dcec4(p_cache, 0, ROM_HEADER_SIZE, buf, 3);
            {
                const char *arc_name = file->arc->name;
                s32 i;
                for (i = 0; i < 3 && arc_name[i] != 0; ++i) {
                }
                MI_CpuCopy8(arc_name, p_cache->arc_name, i);
                p_cache->arc_name_len = i;
            }
        } else {
            rest = 0;
        }

        if (rest >= 0x10) {
            MbSegmentInfo *p = (MbSegmentInfo *) bufp;
            p->reserved      = 0;
            p->arm9_src      = hdr->main_rom_offset + (own_offset + file->top);
            p->arm7_src      = hdr->sub_rom_offset + (own_offset + file->top);
            seg              = p;
            bufp += 0x10;
            rest -= 0x10;
        } else {
            rest = 0;
        }

        if (rest >= hdr->main_size + hdr->sub_size) {
            u32 top = file->top;
            u32 main_size;
            FS_SeekFile(file, seg->arm9_src - top, 0);
            func_020d2cbc(file, bufp, hdr->main_size);
            func_020dcec4(p_cache, seg->arm9_src, hdr->main_size, bufp, 3);
            main_size = hdr->main_size;
            FS_SeekFile(file, seg->arm7_src - top, 0);
            func_020d2cbc(file, bufp + main_size, hdr->sub_size);
            func_020dcec4(p_cache, seg->arm7_src, hdr->sub_size, bufp + main_size, 3);
            ret = 1;
        } else if (rest >= 3 * CACHE_BLOCK_SIZE) {
            u32 top = file->top;
            u32 src = seg->arm9_src;
            FS_SeekFile(file, src - top, 0);
            func_020d2cbc(file, bufp, CACHE_BLOCK_SIZE);
            func_020dcec4(p_cache, src, CACHE_BLOCK_SIZE, bufp, 3);
            FS_SeekFile(file, src + CACHE_BLOCK_SIZE - top, 0);
            func_020d2cbc(file, bufp + CACHE_BLOCK_SIZE, CACHE_BLOCK_SIZE);
            func_020dcec4(p_cache, src + CACHE_BLOCK_SIZE, CACHE_BLOCK_SIZE, bufp + CACHE_BLOCK_SIZE, 2);
            FS_SeekFile(file, src + 2 * CACHE_BLOCK_SIZE - top, 0);
            func_020d2cbc(file, bufp + 2 * CACHE_BLOCK_SIZE, CACHE_BLOCK_SIZE);
            func_020dcec4(p_cache, src + 2 * CACHE_BLOCK_SIZE, CACHE_BLOCK_SIZE, bufp + 2 * CACHE_BLOCK_SIZE, 2);
            ret = 1;
        }

        FS_SeekFile(file, own_offset, 0);

        if (is_own_binary) {
            func_020d2bc0(&own_file);
            if (ret) {
                SegmentContext ctx;
                const u32 *e;
                ctx.rom_offset = hdr->main_rom_offset;
                e              = data_02101564;
                ctx.ram_delta  = hdr->main_ram_address - hdr->main_rom_offset;
                ctx.buf_delta  = (u32) p_cache->list[1].ptr - hdr->main_rom_offset;
                ctx.len        = len;
                MBi_ReadSegmentHeader(&ctx, 0x4000, 0x8000, 1);
                for (; e[1] != 0; e += 2) {
                    MBi_ReadSegmentHeader(&ctx, e[0], e[0] + e[1], 0);
                }
                {
                    u32 *p = (u32 *) p_cache->list[1].ptr;
                    p      = (u32 *) ((u8 *) p + ((u32) AutoloadCallback - hdr->main_ram_address));
                    *p     = INSN_BX_LR;
                }
            }
        }
        if (ret) {
            DC_FlushRange(buf, len);
        }
    }
    return ret;
}
