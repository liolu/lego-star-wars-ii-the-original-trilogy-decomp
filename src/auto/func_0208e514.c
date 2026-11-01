/* Reconstructed from USA ARM9 at 0x0208e514 (716 bytes).
 *
 * func_0208e514: per-frame update of the text menu whose glyph list is built
 * by func_0208e874 (same UnkFile layout).
 *
 * Steps:
 *  1. Bail out while func_02060198(&data_02102900) reports non-zero.
 *  2. flag_e3 == 1: hand over to func_0208ea58 and stop.
 *  3. flag_e0 set: once flags bit 2 is raised, clear it and flag_e0, rebuild
 *     the list (func_0208e874(self, 1)), set bit 0 of menu->flags, centre the
 *     text horizontally ((0x6c - width / 2) << 12, fx32) and re-register
 *     with data_02102900.
 *  4. Otherwise, unless data_027e02a8 & 0x30, hit-test the stored position
 *     (data_021044a4.f2a/f2c, only when bit b2 is set) against each of the
 *     `count` entries: an enabled entry already selected (or one whose
 *     halfword at +2 is >= 10) plays 0xb4 and calls func_0208ea58; another
 *     enabled entry becomes `sel`, plays 0xb6 and rebuilds the list.
 *  5. With no hit: func_02093bd8 -> 0xb4, sel = 0, func_0208ea58;
 *     func_0205cfe8 moved `sel` within [0, count) -> 0xb6 and rebuild;
 *     func_0205d0d0(.., 1, 4) -> 0xb4 and func_0208ea58.
 */
#include "m2c.h"

typedef struct {
    u8 b0 : 1;
    u8 b1 : 1;
    u8 b2 : 1;
    u8 b3 : 1;
    u8 b4 : 1;
    u8 b5 : 1;
    u8 b6 : 1;
    u8 b7 : 1;
    u8 pad[0x29];
    u16 f2a;
    u16 f2c;
} Data;

typedef struct {
    s16 x;
    s16 y;
} Pos;

typedef struct {
    u8 unk_00[8];
    u32 flags;
    u8 unk_0c[0x40];
    s32 unk_4c;
} UnkMenu;

typedef struct {
    u8 unk_00;
    u8 enabled;
    u16 unk_02;
    u8 unk_04[8];
} UnkEntry;

typedef struct {
    u8 unk_00[0x20];
    u32 flags;
    u8 unk_24[4];
    u8 unk_28[8];
    UnkMenu *menu;
    u8 unk_34[8];
    char text[0x5c];
    UnkEntry entries[6];
    u8 flag_e0;
    u8 flag_e1;
    u8 flag_e2;
    u8 flag_e3;
    u32 handle;
    u32 count;
    s32 sel;
    u32 unk_f0;
} UnkFile;

extern s32 func_02060198(void *);
extern void func_0208ea58(UnkFile *file);
extern void func_02072f94(void);
extern void func_0205365c(void *a0, s32 a1);
extern void func_0208e874(UnkFile *file, s32 flag);
extern u8 *func_02066708(s32 a);
extern s32 func_02065840(void *a, u8 *b);
extern void func_020601f4(void *arg0, void *arg1, s32 arg2);
extern s32 func_020601d8(void *unused, void *arg1, s32 arg2, s32 arg3, u8 arg4);
extern s32 func_02064a58(UnkEntry *self, Pos *pos);
extern void func_020b8f70(s32 arg0, s32 arg1);
extern s32 func_02093bd8(void *arg0);
extern s32 func_0205cfe8(void *table, s32 *counter, s32 lo, s32 hi, s32 idx);
extern s32 func_0205d0d0(void *arg0, s32 arg1, s32 arg2);

extern M2C_UNK data_02102900;
extern M2C_UNK data_021027f8;
extern M2C_UNK data_0210930c;
extern u32 data_027e02a8;
extern Data data_021044a4;

void func_0208e514(UnkFile *self) {
    Pos pos;
    s32 hit;
    u32 i;

    if (func_02060198(&data_02102900) != 0) {
        return;
    }
    if (self->flag_e3 == 1) {
        func_0208ea58(self);
        return;
    }
    func_02072f94();
    if (self->flag_e0 != 0) {
        if (!(self->flags & 4)) {
            return;
        }
        self->flags &= ~4;
        self->flag_e0 = 0;
        func_0205365c(self->unk_34, 0);
        func_0208e874(self, 1);
        self->menu->flags |= 1;
        self->menu->unk_4c = (0x6c - func_02065840(self->text, func_02066708(0x3c)) / 2) << 12;
        {
            void *base = self;
            if (self != NULL) {
                base = self->unk_28;
            }
            func_020601f4(&data_02102900, base, 1);
        }
        func_020601d8(&data_02102900, self->unk_34, 1, 5, 0);
        return;
    }
    if (data_027e02a8 & 0x30) {
        return;
    }
    hit   = 0;
    pos.x = 0;
    pos.y = 0;
    if (data_021044a4.b2) {
        pos.x = data_021044a4.f2a;
        pos.y = data_021044a4.f2c;
    }
    for (i = 0; i < self->count; i++) {
        if (func_02064a58(&self->entries[i], &pos) != 0) {
            if ((self->sel == i && self->entries[i].enabled != 0) || (s32) self->entries[i].unk_02 >= 10) {
                func_020b8f70(0xb4, 0);
                func_0208ea58(self);
            } else if (self->sel != i && self->entries[i].enabled != 0) {
                self->sel = i;
                func_020b8f70(0xb6, 0);
                func_0208e874(self, 0);
            }
            hit = 1;
            break;
        }
    }
    if (hit) {
        return;
    }
    if (func_02093bd8(&data_0210930c) != 0) {
        func_020b8f70(0xb4, 0);
        self->sel = 0;
        func_0208ea58(self);
        return;
    }
    if (func_0205cfe8(&data_021027f8, &self->sel, 0, self->count, 4) != 0) {
        func_020b8f70(0xb6, 0);
        func_0208e874(self, 0);
        return;
    }
    if (func_0205d0d0(&data_021027f8, 1, 4) != 0) {
        func_020b8f70(0xb4, 0);
        func_0208ea58(self);
    }
}
