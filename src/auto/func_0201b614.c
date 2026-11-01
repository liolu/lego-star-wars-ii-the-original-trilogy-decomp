#include "m2c.h"

typedef struct Obj {
    u8 pad0[0x8];
    u32 flags;
    u8 pad1[0x4];
    u32 lo : 13;
    u32 wide : 1;
    u32 hi : 18;
    u32 tile : 10;
    u32 rest : 22;
    u8 pad2[0x2];
    u8 prev;
    u8 cur;
    u8 pad3[0x4];
    u16 *res;
    u8 pad4[0x1c];
    u16 x40;
    u16 slot;
    u8 pad5;
    u8 mode;
    u8 idx;
} Obj;

extern s32 data_027e02a4;
extern s32 data_027e02a8;
extern u8 *data_02101b1c[];
extern u32 data_02102988[];
u32 func_0201b368(Obj *obj);
s32 func_0201b44c(void *arg0);
u32 func_02059efc(void *src, void *dst);
u32 func_02064c2c(s32 arg0, u32 arg1, s32 arg2, s32 arg3);
void func_02064cac(M2C_UNK arg0, u32 arg1, s32 arg2, M2C_UNK arg3);

enum {
    NO_SLOT       = 0xffff,         /* func_02064c2c returns 0xffff when allocation fails */
    OBJ_VRAM_MAIN = 0x6400000, /* main-engine OBJ VRAM; +0x200000 per engine = sub OBJ VRAM */
};

/* Engine index (0 or 1) passed as the last argument of the allocator/free calls and
 * used to pick the OBJ VRAM bank of the upload destination. */
static inline s32 vram_flag(Obj *obj) {
    s32 flag;
    if (data_027e02a8 & 0x200) {
        flag = 0;
    } else if (obj->flags & 0x80000000) {
        flag = 0;
    } else {
        flag = 1;
    }
    return flag;
}

static inline s32 needs_slot(Obj *obj, u32 flags, int arg1) {
    if ((arg1 == 1 && (flags & 0x400042) == 0x400040) || obj->slot == NO_SLOT) {
        return 1;
    }
    return 0;
}

/* (Re)acquires the OBJ VRAM block for obj: frees the old block with func_02064cac,
 * allocates one of func_0201b368(obj) bytes with func_02064c2c (block index; NO_SLOT on
 * failure), or reuses a block shared through the table data_02101b1c[data_027e02a4]
 * (12-byte entries with use counts at +0xa). On a fresh allocation the graphics from
 * func_0201b44c are copied to VRAM at block * 32 bytes. Stores block and size in obj,
 * clears flag 0x40 and returns 1 when it uploaded, else 0. */
s32 func_0201b614(Obj *obj, int arg1) {
    u32 slot;
    s32 ok;
    u32 tile;
    u8 mode   = obj->mode;
    u32 flags = obj->flags;

    ok = 1;

    if (needs_slot(obj, flags, arg1)) {
        tile = (u16) func_0201b368(obj);
        if (mode & 3) {
            if (mode & 1) {
                s32 off  = (s16) obj->idx * 12;
                u8 *base = data_02101b1c[data_027e02a4];
                if ((*obj->res != *(u16 *) (base + off + 4) || *(base + off + 0xa) == 0 || obj->cur != *(base + off + 0xb)) &&
                    *(u32 *) ((u32) base + off) != data_02102988[data_027e02a4]) {
                    if (obj->slot != NO_SLOT) {
                        func_02064cac(obj->slot, obj->x40, (u8) obj->wide, vram_flag(obj));
                        obj->slot = NO_SLOT;
                    }
                    (data_02101b1c[data_027e02a4] + 0xa)[(s16) obj->idx * 12]++;
                    *(u16 *) (data_02101b1c[data_027e02a4] + obj->idx * 12 + 4) = obj->res[0];
                    *(u16 *) (data_02101b1c[data_027e02a4] + obj->idx * 12 + 6) = obj->res[1];
                    slot                                                  = (u16) func_02064c2c(0, tile, 0, vram_flag(obj));
                    *(data_02101b1c[data_027e02a4] + obj->idx * 12 + 0xb) = obj->cur;
                    *(u16 *) (data_02101b1c[data_027e02a4] + obj->idx * 12 + 8)   = slot;
                    *(u32 *) (data_02101b1c[data_027e02a4] + (s16) obj->idx * 12) = data_02102988[data_027e02a4];
                } else {
                    if (obj->slot == NO_SLOT) {
                        (data_02101b1c[data_027e02a4] + 0xa)[(s16) obj->idx * 12]++;
                    }
                    slot = *(u16 *) (data_02101b1c[data_027e02a4] + obj->idx * 12 + 8);
                    ok   = 0;
                }
            } else {
                u32 cur = obj->cur;
                if (*(data_02101b1c[data_027e02a4] + obj->idx * 12 + cur + 0xa) == 0) {
                    slot = (u16) func_02064c2c(0, tile, 0, vram_flag(obj));
                    *(u16 *) (data_02101b1c[data_027e02a4] + obj->idx * 12 + cur * 2 + 8) = slot;
                } else {
                    slot = *(u16 *) (data_02101b1c[data_027e02a4] + obj->idx * 12 + cur * 2 + 8);
                    ok   = 0;
                }
                (data_02101b1c[data_027e02a4] + 0xa + obj->idx * 12)[cur]++;
                if (cur != obj->prev && obj->slot != NO_SLOT) {
                    (data_02101b1c[data_027e02a4] + 0xa + obj->idx * 12)[obj->prev]--;
                    if (*(data_02101b1c[data_027e02a4] + obj->idx * 12 + obj->prev + 0xa) == 0) {
                        func_02064cac(obj->slot, obj->x40, (u8) obj->wide, vram_flag(obj));
                        obj->slot                                                                   = NO_SLOT;
                        *(u16 *) (data_02101b1c[data_027e02a4] + obj->idx * 12 + 8 + obj->prev * 2) = NO_SLOT;
                    }
                }
            }
        } else {
            if (obj->slot != NO_SLOT) {
                func_02064cac(obj->slot, obj->x40, (u8) obj->wide, vram_flag(obj));
            }
            slot = (u16) func_02064c2c(0, tile, obj->wide, vram_flag(obj));
        }
        if (slot == NO_SLOT) {
            slot = 0xffff; // FAKE: redundant self-assign steers moveq slot reload
            ok   = 0;
            tile = 0;
        }
        obj->slot = slot;
        obj->tile = slot;
        obj->x40  = tile;
        if (ok == 1) {
            func_02059efc((void *) func_0201b44c(obj), (void *) ((vram_flag(obj) << 21) + OBJ_VRAM_MAIN + (obj->slot << 5)));
        }
        obj->flags &= ~0x40;
    }
    obj->prev = obj->cur;
    return ok != 0;
}
