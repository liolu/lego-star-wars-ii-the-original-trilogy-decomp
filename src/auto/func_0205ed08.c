#include "m2c.h"

typedef struct {
    s32 v;
} UnkVal;

typedef struct Ctx {
    u8 pad0[0x88];
    u32 counter[2];
    u8 pad90[0xdc - 0x90];
    u8 busy[2];
} Ctx;

typedef struct Obj18 {
    s32 pad0;
    s32 kind;
} Obj18;
typedef struct ObjA {
    u8 pad[0x28];
    u8 f28;
    u8 f29;
    u8 f2a;
} ObjA;

extern void func_020016f0(void);
extern void func_0205ce64(void *);
extern void func_0206468c(void *);
extern void func_020627d4(void *, s32);
extern void func_0206c670(void *);
extern void func_020a0440(void *, UnkVal, u8);
extern u8 *func_020aaea8(void);
extern u8 *func_020aaeb4(void);
extern u32 func_020ac6a8(void *, void *);
extern void func_020ac7bc(void *);
extern void func_020b0410(void *);
extern void func_020b8e40(void);
extern void func_0205ab98(s32);
extern s32 func_0205ef84(void *, s32, u8 *);
extern void func_0205f168(void *, s32);
extern void func_0205f1f0(void *, u8 *);
extern void func_0205f45c(void *, s32);
extern void func_0205f520(void *, s32);

extern ObjA data_021027f8;
extern u8 data_021044a4[];
extern u8 data_021042d4[];
extern u8 data_021029e0[];
extern struct {
    u8 pad[0x18];
    Obj18 *p;
} data_021028e8;
extern s32 data_027e02a4;
extern s32 data_027e02a8;
extern UnkVal data_020ff9a0;
extern u8 data_02109c88[];
extern void *data_021092f4;
extern u8 data_0210794c[];
extern u8 data_027e0874[];

/* Depth-buffer selection bit (bit 1) of the 3D engine's SWAP_BUFFERS register. */
typedef enum {
    DEPTH_Z = 0,
    DEPTH_W = 1
} BufMode;

enum {
    REG_SWAP_BUFFERS = 0x4000540,
    SORT_MANUAL      = 1, /* bit 0: translucent polygons keep submission order */
};

/* Write the 3D SWAP_BUFFERS register: sort mode in bit 0, depth mode in bit 1. */
static inline void swap_buffers(s32 am, BufMode zw) {
    *(volatile u32 *) REG_SWAP_BUFFERS = (u32) ((am) | ((zw) << 1)); // FAKE: volatile local cast for MMIO register store
}

/* Per-frame step called from func_020013a4 with &data_02102900 before its
 * frame-pacing wait loop. Runs a fixed sequence of subsystem updates, then two
 * passes over the context's two slots (data_027e02a4 holds the slot being
 * processed); func_0205ef84 records a per-slot byte flag in loc.flags. Unless
 * bit 0x200 of data_027e02a8 is set, slots whose flag is set and whose busy
 * byte is clear get func_0205f168, the remaining idle slots get func_0205f520,
 * and the frame ends by swapping the 3D buffers in manual-sort mode with
 * Z or W depth buffering chosen by byte 0x1b0 of the func_020aaea8 state.
 * With bit 0x200 set, func_0205f1f0 handles the flags and the swap is skipped.
 */

void func_0205ed08(Ctx *ctx) {
    struct {
        u16 saved;
        u8 flags[2];
    } loc;
    s32 first = 0;
    u32 i;
    u8 *fp;
    s32 skip;

    loc.flags[0] = 0;
    loc.flags[1] = 0;
    func_020016f0();
    func_0205ce64(&data_021027f8);
    func_0206468c(data_021044a4);
    func_020627d4(data_021042d4, 0);
    func_0206c670(data_021029e0);
    if ((data_021028e8.p != NULL ? data_021028e8.p->kind : 0) == 4) {
        if ((data_027e02a8 & 3) == 0) {
            func_020a0440(data_02109c88, data_020ff9a0, data_021027f8.f2a);
        }
        if (data_027e02a8 & 0x40) {
            loc.saved = *(u16 *) (func_020aaeb4() + 0x42);
            {
                void *o             = data_021092f4;
                u8 *r               = ((u8 * (**) (void *) )(*(void **) o))[1](o);
                *(u16 *) (r + 0x6a) = loc.saved;
            }
        }
    }
    for (i = 0; i < 2; i++) {
        data_027e02a4   = i;
        ctx->counter[i] = ctx->counter[i] + 1;
        {
            s32 r = func_0205ef84(ctx, i, &loc.flags[i]);
            if (first == 0) {
                first = r;
            }
        }
    }
    if (data_021027f8.f29 > 1) {
        func_020b0410(data_0210794c);
    }
    func_0205ab98(first);
    for (i = 0; i < 2; i++) {
        data_027e02a4 = i;
        func_0205f45c(ctx, i);
    }
    skip = (data_027e02a8 & 0x200) != 0;
    if (!skip) {
        func_020ac6a8(data_027e0874, func_020aaea8() + 0x4c);
        for (i = 0, fp = loc.flags; i < 2; i++, fp++) {
            if (*fp != 0 && ctx->busy[i] == 0) {
                data_027e02a4 = i;
                func_0205f168(ctx, i);
            }
        }
    } else {
        func_0205f1f0(ctx, loc.flags);
    }
    func_020b8e40();
    if (skip) {
        return;
    }
    for (i = 0; i < 2; i++) {
        if (ctx->busy[i] == 0) {
            data_027e02a4 = i;
            func_0205f520(ctx, i);
        }
    }
    func_020ac7bc(data_027e0874);
    swap_buffers(SORT_MANUAL, func_020aaea8()[0x1b0] ? DEPTH_Z : DEPTH_W);
}
