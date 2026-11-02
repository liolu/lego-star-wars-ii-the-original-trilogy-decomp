#include "m2c.h"

/* Per-screen state: an 8-byte-per-entry OAM shadow at offset 0, plus the
 * number of entries used this frame and in the previous frame. */
typedef struct {
    u8 pad[0x721];
    u8 cur;
    u8 prev;
    u8 rest[0x1];
} Screen;

/* VBlank counters; the object is volatile because every store to ticks/b is reloaded. */
typedef struct {
    u8 a;
    u8 b;
    u8 ticks;
    u8 pad[0x11];
    u32 vblanks;
} Counter;

enum {
    DTCM_IRQ_CHECK_OFFSET   = 0x3ff8, /* OS IRQ check flag word in DTCM */
    IRQ_CHECK_VBLANK        = 0x1,
    TICKS_PER_PERIOD        = 0x3c,        /* 60 VBlanks */
    FLAG_SKIP_VRAM_TRANSFER = 0x80,
    FLAG_SHARED_OAM         = 0x200,
    SCREEN_COUNT            = 2,
    OAM_MAIN                = 0x7000000,
    OAM_SCREEN_SHIFT        = 10,          /* 0x400 bytes of OAM per screen */
    OAM_ENTRY_SHIFT         = 3,            /* 8 bytes per OAM entry */
    COPY_ARG                = 0x20,
    SCREEN_DATA_STRIDE      = 0x20,
};

extern volatile Counter data_02102824;
extern s32 data_027e02a4;
extern u32 data_027e02a8;
extern u8 data_0210283c[];
extern Screen data_02103374[];
extern u8 data_02101b78[];
extern u8 data_021042d4[];
extern u8 data_021029e0[];
extern u32 SDK_AUTOLOAD_DTCM_START[];

void NNS_GfdDoVramTransfer(void);
void func_02053d90(void *);
void func_02055c94(u32);
void func_0205e724(s32, void *, s32, s32, s32);
void func_02060a2c(s32);
void func_020627d4(void *, s32);
void func_02064e64(s32);
void func_0206cdbc(void *);
void func_020b8e58(void);

/* VBlank IRQ handler (installed by func_0205d158 via OS_SetIrqFunction(1, ...)).
 * 1. Acknowledge the VBlank in the DTCM IRQ check flag.
 * 2. Count VBlanks; every 60 ticks latch b into a and clear b.
 * 3. Run the pending NNS VRAM transfers unless flag 0x80 is set.
 * 4. For each screen with its dirty byte set (screen index published in
 *    data_027e02a4 meanwhile): copy max(cur, prev) OAM entries from the
 *    shadow to OAM (both to main OAM when flag 0x200 is set), roll the entry
 *    counts and run the per-screen updates; screen 0 also calls func_0206cdbc.
 * 5. Restore the screen index and call func_020b8e58. */
void func_0205d1a0(void) {
    s32 saved_screen;
    u32 screen;
    u32 *irq_check = (u32 *) ((u8 *) SDK_AUTOLOAD_DTCM_START + DTCM_IRQ_CHECK_OFFSET);

    *irq_check |= IRQ_CHECK_VBLANK;
    saved_screen = data_027e02a4;
    data_02102824.vblanks++;
    data_02102824.ticks++;
    if (data_02102824.ticks >= TICKS_PER_PERIOD) {
        data_02102824.a     = data_02102824.b;
        data_02102824.b     = 0;
        data_02102824.ticks = data_02102824.b;
    }
    if ((data_027e02a8 & FLAG_SKIP_VRAM_TRANSFER) != FLAG_SKIP_VRAM_TRANSFER) {
        NNS_GfdDoVramTransfer();
    }
    func_020627d4(data_021042d4, 1);
    for (screen = 0; (s32) screen < SCREEN_COUNT;
         screen++) { // FAKE: signed compare on an unsigned index sets the loop register colouring
        data_027e02a4 = screen;
        if (data_0210283c[screen] != 0) {
            Screen *s = &data_02103374[screen];
            u32 entries;
            if (data_027e02a8 & FLAG_SHARED_OAM) {
                entries = data_02103374[screen ^ 1].prev;
                if (s->cur >= entries) {
                    entries = s->cur;
                }
                func_0205e724(0, s, OAM_MAIN, entries << OAM_ENTRY_SHIFT, COPY_ARG);
            } else {
                entries = s->prev;
                if (s->cur >= entries) {
                    entries = s->cur;
                }
                func_0205e724(0, s, OAM_MAIN + (screen << OAM_SCREEN_SHIFT), entries << OAM_ENTRY_SHIFT, COPY_ARG);
            }
            s->prev = s->cur;
            s->cur  = 0;
            func_02060a2c(screen);
            func_02055c94(screen);
            func_02053d90(data_02101b78 + screen * SCREEN_DATA_STRIDE);
            func_02064e64(screen);
            data_0210283c[screen] = 0;
            if (screen == 0) {
                func_0206cdbc(data_021029e0);
            }
        }
    }
    data_027e02a4 = saved_screen;
    func_020b8e58();
}
