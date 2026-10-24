/* Reconstructed from USA ARM9 at 0x02068d74 (192 bytes). */
#include "m2c.h"

typedef struct Item {
    u8 pad0[8];
    u32 flags;
} Item;

typedef struct Group {
    u8 pad0[8];
    Item *items[8];
} Group;

typedef struct Screen {
    u8 pad0[0xC];
    Group *group;
    u8 pad10[4];
    void *f14;
    void *f18;
    u8 pad1C[4];
    u8 *f20;
    u8 pad24[0x11];
    u8 f35;
    u8 f36;
} Screen;

void func_0205365c(void *, s32);                                  /* extern */
void func_020536a8(void *, s32);                                  /* extern */
void func_0206574c(void *, s32, s32);                             /* extern */
s32 func_02066708(s32);                                           /* extern */
void func_02065be8(void *, s32, s32, s32, s32, s32);              /* extern */
void func_02068fbc(Screen *);                                     /* extern */

void func_02068d74(Screen *self) {
    u32 i;
    s32 id;

    func_0205365c(self->f18, 0);
    func_020536a8(self->f18, 0);
    func_0206574c(self->f14, 9, 0);
    id = (s32) self->f20 + 0x231;
    if (self->f35 != 0) {
        id = 0x23F;
    }
    func_02065be8(self->f14, 0x8C, 0x80, 0x1E, func_02066708(id), 1);
    self->f36 = 0xFF;
    for (i = 0; i < 8; i++) {
        self->group->items[i]->flags &= ~1;
    }
    func_02068fbc(self);
    func_020536a8(self->f18, 1);
}
